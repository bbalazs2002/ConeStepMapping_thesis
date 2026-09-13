#include <gtest/gtest.h>
#include "Headers/Manager/ShaderManager.h"
#include "Headers/RendererVisitor/OpenGLRendererVisitor.h"
#include "Headers/Model/Model.h"
#include "Headers/Model/RayMarchedModel.h"
#include "Headers/Model/Mesh.h"
#include "Headers/Material/Material.h"
#include "Headers/RayMarching/ConeStepMapping.h"
#include "Headers/RayMarching/ConemapGenerator.h"
#include "Headers/Manager/TextureManager.h"
#include "Utils/Camera.h"
#include "Utils/GLUtils.hpp"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <cstdint>
#include <vector>

// Previously untested: OpenGLRendererVisitor::Visit(const Model&) — the
// entire regular (non-RayMarched) model rendering path, including wireframe
// mode and the "selected object gets a wireframe overlay" branch, which is
// also present (and equally untested) in Visit(const RayMarchedModel&).

namespace {

std::shared_ptr<Mesh> MakeTriangleMesh() {
    MeshObject<Vertex> tri;
    tri.vertexArray = {
        { { -1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f } },
        { {  1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, { 1.f, 0.f } },
        { {  0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.5f, 1.f } },
    };
    tri.indexArray = { 0, 1, 2 };
    auto mesh = std::make_shared<Mesh>();
    mesh->Build<Vertex>(std::move(tri));

    auto mat = std::make_shared<Material>();
    mat->SetDiffuseColor(glm::vec3(0.7f));
    mesh->SetMaterial(mat);
    return mesh;
}

class ModelRenderFixture : public ::testing::Test {
protected:
    ShaderManager shaderMgr;
    GLuint modelProgram         = 0;
    GLuint modelSelectedProgram = 0;
    Camera cam;

    void SetUp() override {
        modelProgram = shaderMgr.Load("model_render_test", {
            { GL_VERTEX_SHADER,   "src/Shaders/Models/Vert_Model.vert" },
            { GL_FRAGMENT_SHADER, "src/Shaders/Models/Frag_Model.frag" }
        });
        modelSelectedProgram = shaderMgr.Load("model_selected_render_test", {
            { GL_VERTEX_SHADER,   "src/Shaders/Models/Vert_ModelSelected.vert" },
            { GL_FRAGMENT_SHADER, "src/Shaders/Models/Frag_ModelSelected.frag" }
        });
        cam.SetView(glm::vec3(0.f, 2.f, 4.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
        cam.SetAspect(1.0f);
    }
};

} // namespace

TEST_F(ModelRenderFixture, VisitModelRunsWithoutGLError) {
    ASSERT_NE(modelProgram, 0u);

    Model model("tri");
    model.SetProgram(modelProgram);
    model.AddMesh(MakeTriangleMesh());

    OpenGLRendererVisitor visitor(cam);
    while (glGetError() != GL_NO_ERROR) {}
    visitor.Visit(model);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(ModelRenderFixture, VisitWireframeModelRunsWithoutGLError) {
    ASSERT_NE(modelProgram, 0u);

    Model model("tri_wireframe");
    model.SetProgram(modelProgram);
    model.SetWireframe(true);
    model.AddMesh(MakeTriangleMesh());

    OpenGLRendererVisitor visitor(cam);
    while (glGetError() != GL_NO_ERROR) {}
    visitor.Visit(model);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(ModelRenderFixture, VisitSelectedModelDrawsHighlightOverlayWithoutGLError) {
    ASSERT_NE(modelProgram, 0u);
    ASSERT_NE(modelSelectedProgram, 0u);

    Model model("tri_selected");
    model.SetProgram(modelProgram);
    model.SetSelectedProgram(modelSelectedProgram);
    model.AddMesh(MakeTriangleMesh());

    OpenGLRendererVisitor visitor(cam);
    visitor.SetSelected(&model);

    while (glGetError() != GL_NO_ERROR) {}
    visitor.Visit(model);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(ModelRenderFixture, VisitSelectedRayMarchedModelDrawsHighlightOverlayWithoutGLError) {
    // Same selection-highlight branch as above, but on the RayMarchedModel
    // overload of Visit() -- a separate code path with its own copy of the
    // wireframe-overlay logic.
    GLuint csmProgram = shaderMgr.Load("csm_render_test", {
        { GL_VERTEX_SHADER,   "src/Shaders/RayMarching/Vert_RM.vert" },
        { GL_GEOMETRY_SHADER, "src/Shaders/RayMarching/Geom_RM_abcd.geom" },
        { GL_FRAGMENT_SHADER, "src/Shaders/RayMarching/Frag_ConeStepMapping.frag" }
    });
    GLuint conemapOriginal = shaderMgr.Load("conemap_render_test",
        { { GL_COMPUTE_SHADER, "src/Shaders/Conemap/Comp_Conemap.comp" } });
    GLuint conemapConservative = shaderMgr.Load("conemap_render_test_c",
        { { GL_COMPUTE_SHADER, "src/Shaders/Conemap/Comp_Conemap.comp" } }, { "CONSERVATIVE" });
    ASSERT_NE(csmProgram, 0u);
    ASSERT_NE(modelSelectedProgram, 0u);

    // Reuse the flat-quad + flat-heightmap pattern from test_raymarching.cpp.
    auto heightmapPath = std::filesystem::temp_directory_path() / "csm_render_test_heightmap.bmp";
    {
        // 4x4 flat gray BMP (24bpp, no row padding needed).
        const int size = 4;
        std::vector<uint8_t> buf(54 + size * size * 3, 0);
        buf[0] = 'B'; buf[1] = 'M';
        auto put32 = [&](int off, uint32_t v) { std::memcpy(&buf[off], &v, 4); };
        auto put16 = [&](int off, uint16_t v) { std::memcpy(&buf[off], &v, 2); };
        put32(2, static_cast<uint32_t>(buf.size()));
        put32(10, 54);
        put32(14, 40);
        put32(18, size);
        put32(22, size);
        put16(26, 1);
        put16(28, 24);
        put32(34, static_cast<uint32_t>(size * size * 3));
        for (int i = 0; i < size * size; ++i) {
            buf[54 + i * 3 + 0] = 128;
            buf[54 + i * 3 + 1] = 128;
            buf[54 + i * 3 + 2] = 128;
        }
        std::ofstream ofs(heightmapPath, std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
    }

    TextureManager texMgr;
    auto heightmap = texMgr.GetOrLoad(heightmapPath, false);
    ConemapGenerator gen(conemapOriginal, conemapConservative);

    auto model = std::make_shared<RayMarchedModel>("rm_selected");
    model->SetTechnique(std::make_shared<ConeStepMapping>(csmProgram));
    model->SetHeightmap(heightmap, &gen);
    model->SetSelectedProgram(modelSelectedProgram);

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
    auto mat = std::make_shared<Material>();
    mat->SetDiffuseColor(glm::vec3(0.8f));
    mesh->SetMaterial(mat);
    model->AddMesh(mesh);

    OpenGLRendererVisitor visitor(cam);
    visitor.SetSelected(model.get());

    while (glGetError() != GL_NO_ERROR) {}
    visitor.Visit(*model);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    std::error_code ec;
    std::filesystem::remove(heightmapPath, ec);
}
