#include <gtest/gtest.h>
#include "Headers/Manager/ShaderManager.h"
#include "Headers/Manager/TextureManager.h"
#include "Headers/RayMarching/ConemapGenerator.h"
#include "Headers/RayMarching/LinearSearch.h"
#include "Headers/RayMarching/ConeStepMapping.h"
#include "Headers/Model/RayMarchedModel.h"
#include "Headers/Model/Mesh.h"
#include "Headers/Material/Material.h"
#include "Headers/RendererVisitor/OpenGLRendererVisitor.h"
#include "Headers/Debug/RayMarchDebugState.h"
#include "Utils/Camera.h"
#include "Utils/GLUtils.hpp"
#include "Utils/ProgramBuilder.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <vector>

// Previously untested: the entire RayMarching module (ConemapGenerator,
// LinearSearch, ConeStepMapping) was 0% covered because nothing ever linked
// a real compiled ray marching shader program. These tests compile the
// production shaders via the real ShaderManager, exercise the CPU-side
// wrapper classes, and — for the GPU-side ray marching algorithm itself,
// which has no C++ surface at all — use the debug SSBOs as an observation
// channel: render a primitive with debug capture on, then read back
// stepCount / hit UV exactly like MyApp::ExportDebugLog() does.

namespace {

// Writes a small, perfectly flat 24-bit grayscale BMP heightmap (no row
// padding needed for a 4-pixel-wide image: 4*3 = 12 bytes/row).
void WriteFlatGrayBMP(const std::filesystem::path& p, uint8_t gray, int size = 4) {
    const int rowSize        = size * 3;
    const int pixelDataSize  = rowSize * size;
    const uint32_t fileSize  = 54 + static_cast<uint32_t>(pixelDataSize);

    std::vector<uint8_t> buf(54 + pixelDataSize, 0);
    buf[0] = 'B'; buf[1] = 'M';
    auto put32 = [&](int off, uint32_t v) { std::memcpy(&buf[off], &v, 4); };
    auto put16 = [&](int off, uint16_t v) { std::memcpy(&buf[off], &v, 2); };

    put32(2, fileSize);
    put32(10, 54);                                   // pixel data offset
    put32(14, 40);                                   // BITMAPINFOHEADER size
    put32(18, static_cast<uint32_t>(size));           // width
    put32(22, static_cast<uint32_t>(size));           // height
    put16(26, 1);                                     // planes
    put16(28, 24);                                     // bpp
    put32(34, static_cast<uint32_t>(pixelDataSize));

    for (int i = 0; i < size * size; ++i) {
        buf[54 + i * 3 + 0] = gray; // B
        buf[54 + i * 3 + 1] = gray; // G
        buf[54 + i * 3 + 2] = gray; // R
    }
    std::ofstream ofs(p, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
}

std::shared_ptr<Mesh> MakeFlatQuadMesh() {
    MeshObject<VertexMergedNorm> quad;
    quad.vertexArray = {
        { { -2.5f, 0.f, -2.5f }, { 0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f } },
        { {  2.5f, 0.f, -2.5f }, { 0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }, { 1.f, 0.f } },
        { {  2.5f, 0.f,  2.5f }, { 0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }, { 1.f, 1.f } },
        { { -2.5f, 0.f,  2.5f }, { 0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 1.f } },
    };
    quad.indexArray = { 0, 1, 2, 0, 2, 3 };
    auto mesh = std::make_shared<Mesh>();
    mesh->Build<VertexMergedNorm>(std::move(quad));
    // Mesh::Render() hard-exits without a material (mirrors production usage).
    auto mat = std::make_shared<Material>();
    mat->SetDiffuseColor(glm::vec3(0.8f));
    mesh->SetMaterial(mat);
    return mesh;
}

// Mirrors MyApp::InitDebugSSBOs() layout (see docs/Debug.md / Debug.cpp).
struct DebugSSBOs {
    GLuint visual = 0, numerical = 0;

    void Init() {
        glCreateBuffers(1, &visual);
        glNamedBufferData(visual, (9 + 512) * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);

        glCreateBuffers(1, &numerical);
        glNamedBufferData(numerical,
            2 * sizeof(glm::uvec4) + (23 + 512 * 2) * sizeof(glm::vec4),
            nullptr, GL_DYNAMIC_DRAW);

        struct DrawCmd { GLuint count = 0, instanceCount = 1, first = 0, baseInstance = 0; };
        DrawCmd initCmds[2];
        glNamedBufferSubData(numerical, 0, sizeof(initCmds), initCmds);
    }

    ~DebugSSBOs() {
        if (visual)    glDeleteBuffers(1, &visual);
        if (numerical) glDeleteBuffers(1, &numerical);
    }
};

class RayMarchingFixture : public ::testing::Test {
protected:
    std::filesystem::path heightmapPath;
    ShaderManager shaderMgr;
    TextureManager texMgr;

    GLuint conemapOriginal     = 0;
    GLuint conemapConservative = 0;
    GLuint lsProgram           = 0;
    GLuint csmProgram          = 0;

    void SetUp() override {
        heightmapPath = std::filesystem::temp_directory_path() / "csm_rm_heightmap.bmp";
        WriteFlatGrayBMP(heightmapPath, 128, 4);

        conemapOriginal = shaderMgr.Load("conemap_original_test",
            { { GL_COMPUTE_SHADER, "src/Shaders/Conemap/Comp_Conemap.comp" } });
        conemapConservative = shaderMgr.Load("conemap_conservative_test",
            { { GL_COMPUTE_SHADER, "src/Shaders/Conemap/Comp_Conemap.comp" } }, { "CONSERVATIVE" });

        lsProgram = shaderMgr.Load("ls_test", {
            { GL_VERTEX_SHADER,   "src/Shaders/RayMarching/Vert_RM.vert" },
            { GL_GEOMETRY_SHADER, "src/Shaders/RayMarching/Geom_RM_abcd.geom" },
            { GL_FRAGMENT_SHADER, "src/Shaders/RayMarching/Frag_LinearSearch.frag" }
        });
        csmProgram = shaderMgr.Load("csm_test", {
            { GL_VERTEX_SHADER,   "src/Shaders/RayMarching/Vert_RM.vert" },
            { GL_GEOMETRY_SHADER, "src/Shaders/RayMarching/Geom_RM_abcd.geom" },
            { GL_FRAGMENT_SHADER, "src/Shaders/RayMarching/Frag_ConeStepMapping.frag" }
        });
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove(heightmapPath, ec);
    }
};

} // namespace

// -- ConemapGenerator ----------------------------------------------------------

TEST_F(RayMarchingFixture, RealShadersCompileAndLink) {
    EXPECT_NE(conemapOriginal, 0u);
    EXPECT_NE(conemapConservative, 0u);
    EXPECT_NE(lsProgram, 0u);
    EXPECT_NE(csmProgram, 0u);
}

// -- ShaderManager / ProgramBuilder ---------------------------------------------

TEST_F(RayMarchingFixture, GetReturnsZeroForUnknownProgramName) {
    EXPECT_EQ(shaderMgr.Get("no_such_program_was_ever_loaded"), 0u);
}

TEST_F(RayMarchingFixture, LoadWithEmptyDefinesDelegatesToTwoArgOverload) {
    // The 3-arg Load(name, stages, defines) with an empty defines vector
    // must behave identically to the 2-arg overload (it forwards to it).
    GLuint prog = shaderMgr.Load("ls_empty_defines_test", {
        { GL_VERTEX_SHADER,   "src/Shaders/RayMarching/Vert_RM.vert" },
        { GL_GEOMETRY_SHADER, "src/Shaders/RayMarching/Geom_RM_abcd.geom" },
        { GL_FRAGMENT_SHADER, "src/Shaders/RayMarching/Frag_LinearSearch.frag" }
    }, {});
    EXPECT_NE(prog, 0u);
    EXPECT_EQ(shaderMgr.Get("ls_empty_defines_test"), prog);
}

TEST_F(RayMarchingFixture, ReloadAllKeepsTheSameProgramIDAndValidLink) {
    // The whole point of in-place relinking: classes caching the GLuint from
    // Load() must stay valid after a Ctrl+F5-style reload.
    shaderMgr.ReloadAll();

    EXPECT_EQ(shaderMgr.Get("csm_test"), csmProgram);
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(csmProgram, GL_LINK_STATUS, &linkStatus);
    EXPECT_EQ(linkStatus, GL_TRUE);
}

TEST_F(RayMarchingFixture, ProgramBuilderWithZeroIDLogsWithoutCrashing) {
    ProgramBuilder builder(0);
    SUCCEED();
}

TEST_F(RayMarchingFixture, GenerateProducesValidConemapMatchingHeightmapSize) {
    auto heightmap = texMgr.GetOrLoad(heightmapPath, false);
    ASSERT_TRUE(heightmap->IsValid());

    ConemapGenerator gen(conemapOriginal, conemapConservative);
    auto conemap = gen.Generate(*heightmap);

    ASSERT_NE(conemap, nullptr);
    EXPECT_TRUE(conemap->IsValid());
    EXPECT_EQ(gen.GetLastConemapID(), conemap->GetID());

    GLint w = 0, h = 0;
    glGetTextureLevelParameteriv(conemap->GetID(), 0, GL_TEXTURE_WIDTH,  &w);
    glGetTextureLevelParameteriv(conemap->GetID(), 0, GL_TEXTURE_HEIGHT, &h);
    EXPECT_EQ(w, 4);
    EXPECT_EQ(h, 4);
}

TEST_F(RayMarchingFixture, GeneratedConemapHeightChannelMatchesFlatInput) {
    auto heightmap = texMgr.GetOrLoad(heightmapPath, false);
    ConemapGenerator gen(conemapOriginal, conemapConservative);
    auto conemap = gen.Generate(*heightmap);
    ASSERT_NE(conemap, nullptr);

    std::vector<uint8_t> pixels(4 * 4 * 4);
    glGetTextureImage(conemap->GetID(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
        static_cast<GLsizei>(pixels.size()), pixels.data());

    // R channel = height. For a perfectly flat input every texel should
    // read back the same value the heightmap was filled with.
    for (int i = 0; i < 4 * 4; ++i)
        EXPECT_NEAR(pixels[i * 4 + 0], 128, 2) << "texel " << i;
}

TEST_F(RayMarchingFixture, ConservativeFlagSelectsTheOtherProgram) {
    auto heightmap = texMgr.GetOrLoad(heightmapPath, false);
    ConemapGenerator gen(conemapOriginal, conemapConservative);
    EXPECT_FALSE(gen.IsConservative());

    gen.SetConservative(true);
    EXPECT_TRUE(gen.IsConservative());

    auto conemap = gen.Generate(*heightmap);
    ASSERT_NE(conemap, nullptr);
    EXPECT_TRUE(conemap->IsValid());
}

TEST_F(RayMarchingFixture, GenerateReturnsNullForInvalidSizeHeightmap) {
    // A texture that failed to load (non-existent file) has GL id 0 and
    // reports width/height 0 via glGetTextureLevelParameteriv -- Generate()
    // must reject it instead of dispatching a compute shader on a 0x0 image.
    auto missing = texMgr.GetOrLoad(
        std::filesystem::temp_directory_path() / "csm_rm_no_such_heightmap.bmp", false);
    ASSERT_FALSE(missing->IsValid());

    ConemapGenerator gen(conemapOriginal, conemapConservative);
    auto conemap = gen.Generate(*missing);
    EXPECT_EQ(conemap, nullptr);
}

// -- LinearSearch / ConeStepMapping wrapper classes -----------------------------

TEST_F(RayMarchingFixture, LinearSearchReportsIdentity) {
    LinearSearch tech(lsProgram);
    EXPECT_EQ(tech.GetProgramID(), lsProgram);
    EXPECT_EQ(tech.GetName(), "Linear Search");
    EXPECT_EQ(tech.GetTechniqueID(), 0);
}

TEST_F(RayMarchingFixture, ConeStepMappingReportsIdentity) {
    ConeStepMapping tech(csmProgram);
    EXPECT_EQ(tech.GetProgramID(), csmProgram);
    EXPECT_EQ(tech.GetName(), "Cone Step Mapping");
    EXPECT_EQ(tech.GetTechniqueID(), 1);
}

TEST_F(RayMarchingFixture, SetUniformsRunsWithoutGLErrorForBothTechniques) {
    auto heightmap = texMgr.GetOrLoad(heightmapPath, false);
    ConemapGenerator gen(conemapOriginal, conemapConservative);

    RayMarchedModel model("test_surface");
    model.SetHeightmap(heightmap, &gen);
    ASSERT_NE(model.GetConemap(), nullptr);

    for (GLuint prog : { lsProgram, csmProgram }) {
        glUseProgram(prog);
        while (glGetError() != GL_NO_ERROR) {}
        if (prog == lsProgram) {
            LinearSearch(prog).SetUniforms(model);
        } else {
            ConeStepMapping(prog).SetUniforms(model);
        }
        EXPECT_EQ(glGetError(), GL_NO_ERROR);
    }
    glUseProgram(0);
}

TEST_F(RayMarchingFixture, ConeStepMappingSetUniformsLogsWithoutCrashWhenConemapIsNull) {
    // A RayMarchedModel that never had SetHeightmap() called has no conemap;
    // SetUniforms() must log and continue rather than dereference a null Texture.
    RayMarchedModel model("no_conemap");
    glUseProgram(csmProgram);
    while (glGetError() != GL_NO_ERROR) {}
    EXPECT_NO_THROW(ConeStepMapping(csmProgram).SetUniforms(model));
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    glUseProgram(0);
}

TEST_F(RayMarchingFixture, GetProgramIDReturnsZeroWithoutTechnique) {
    RayMarchedModel model("no_technique");
    EXPECT_EQ(model.GetProgramID(), 0u);
}

TEST_F(RayMarchingFixture, ConemapGeneratorErrorLoggedWithoutCrashOnNullGenerator) {
    // RayMarchedModel::SetHeightmap with gen == nullptr must be a no-op —
    // covered here alongside the real-generator path for contrast.
    RayMarchedModel model("no_gen");
    auto heightmap = texMgr.GetOrLoad(heightmapPath, false);
    EXPECT_NO_THROW(model.SetHeightmap(heightmap, nullptr));
    EXPECT_EQ(model.GetConemap(), nullptr);
}

// -- Full pipeline + debug SSBO readback (GPU-side algorithm verification) -----

TEST_F(RayMarchingFixture, DebugSSBOReportsHitForStraightDownRayOnFlatSurface) {
    auto heightmap = texMgr.GetOrLoad(heightmapPath, false);
    ConemapGenerator gen(conemapOriginal, conemapConservative);

    auto model = std::make_shared<RayMarchedModel>("debug_test_surface");
    model->SetTechnique(std::make_shared<ConeStepMapping>(csmProgram));
    model->SetHeightmap(heightmap, &gen);
    ASSERT_NE(model->GetConemap(), nullptr);
    model->AddMesh(MakeFlatQuadMesh());

    DebugSSBOs debug;
    debug.Init();

    RayMarchDebugState state;
    state.debugVisualSSBO    = debug.visual;
    state.debugNumericalSSBO = debug.numerical;
    state.config.showDebug   = true;
    state.config.primitiveID = -1; // inspect whichever primitive the ray hits
    state.dirty = true;
    // Debug ray: straight down through the quad's center, through the
    // displaced prism (extends +Y by normalMult, default 0.5, above the quad).
    state.debugCamera.SetView(
        glm::vec3(0.f, 3.f, 0.f), glm::vec3(0.f, -3.f, 0.f), glm::vec3(0.f, 0.f, -1.f));

    Camera renderCam;
    renderCam.SetView(glm::vec3(0.f, 5.f, 5.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
    renderCam.SetAspect(1.0f);

    OpenGLRendererVisitor visitor(renderCam);
    visitor.SetDebugState(&state);

    while (glGetError() != GL_NO_ERROR) {}
    visitor.Visit(*model);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    // Read back exactly like MyApp::ExportDebugLog() / the ImGui Values tab.
    const GLintptr kNumericalOffset = 2 * static_cast<GLintptr>(sizeof(glm::uvec4));
    glm::vec4 hdr[23];
    glGetNamedBufferSubData(debug.numerical, kNumericalOffset, sizeof(hdr), hdr);

    const int  stepCount = static_cast<int>(hdr[0].x + 0.5f);
    const bool wasHit    = hdr[16].w > 0.5f;

    EXPECT_TRUE(wasHit);
    EXPECT_GT(stepCount, 0);
    EXPECT_LE(stepCount, model->GetMaxSteps());
    if (wasHit) {
        EXPECT_NEAR(hdr[16].x, 0.5f, 0.2f) << "hit UV.x should land near quad center";
        EXPECT_NEAR(hdr[16].y, 0.5f, 0.2f) << "hit UV.y should land near quad center";
    }
}
