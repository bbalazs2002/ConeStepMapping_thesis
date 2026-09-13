#include <gtest/gtest.h>
#include "Utils/ModelLoader.h"
#include "Utils/GLUtils.hpp"
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>

// -----------------------------------------------------------------------
// Path utilities
// -----------------------------------------------------------------------

// UT-18: relative path -> absolute (prepends PROJECT_ROOT)
TEST(ModelLoader_Path, RelToAbsPathMakesAbsolute) {
    auto result = ModelLoader::RelToAbsPath("Assets/test.obj");
    EXPECT_TRUE(result.is_absolute());
}

// UT-19: absolute path is returned unchanged
TEST(ModelLoader_Path, RelToAbsPathAbsolutePassthrough) {
    std::filesystem::path abs = std::filesystem::path(PROJECT_ROOT) / "Assets" / "test.obj";
    auto result = ModelLoader::RelToAbsPath(abs);
    EXPECT_EQ(std::filesystem::weakly_canonical(result),
              std::filesystem::weakly_canonical(abs));
}

// UT-20: relative texture name + texFolder -> absolute path containing folder
TEST(ModelLoader_Path, ResolveTexturePathRelative) {
    std::filesystem::path texFolder =
        std::filesystem::path(PROJECT_ROOT) / "Assets" / "Fire_Extinguisher";
    auto result = ModelLoader::ResolveTexturePath("Fire_Extinguisher.png", texFolder);
    EXPECT_TRUE(result.is_absolute());
    EXPECT_TRUE(result.string().find("Fire_Extinguisher") != std::string::npos);
}

// UT-21: absolute texture path is returned unchanged
TEST(ModelLoader_Path, ResolveTexturePathAbsolutePassthrough) {
    std::filesystem::path abs =
        std::filesystem::path(PROJECT_ROOT) / "Assets" / "HMaps" / "cone.jpg";
    auto result = ModelLoader::ResolveTexturePath(abs.string(), "");
    EXPECT_TRUE(result.is_absolute());
}

// An absolute mtlSearchPath is returned unchanged, without being resolved
// relative to the .obj file's directory.
TEST(ModelLoader_Path, ResolveMTLSearchPathAbsolutePassthrough) {
    std::filesystem::path abs = std::filesystem::path(PROJECT_ROOT) / "Assets";
    auto result = ModelLoader::ResolveMTLSearchPath("some/obj/path.obj", abs);
    EXPECT_EQ(result, abs);
}

// -----------------------------------------------------------------------
// MergeNormals
// -----------------------------------------------------------------------

// UT-23: output size matches input
TEST(ModelLoader_MergeNormals, OutputSizeMatchesInput) {
    std::vector<Vertex> input(6);
    for (int i = 0; i < 6; ++i) {
        input[i].position = { float(i % 3), 0.f, 0.f };
        input[i].normal   = { 0.f, 1.f, 0.f };
        input[i].texcoord = { float(i) / 6.f, 0.f };
    }
    auto output = ModelLoader::MergeNormals(input);
    EXPECT_EQ(output.size(), input.size());
}

// UT-24: MergeNormals preserves texcoords exactly
TEST(ModelLoader_MergeNormals, PreservesTexcoords) {
    std::vector<Vertex> input(4);
    for (int i = 0; i < 4; ++i) {
        input[i].position = { float(i), 0.f, 0.f };
        input[i].normal   = { 0.f, 1.f, 0.f };
        input[i].texcoord = { float(i) * 0.1f, float(i) * 0.2f };
    }
    auto output = ModelLoader::MergeNormals(input);
    for (size_t i = 0; i < input.size(); ++i) {
        EXPECT_FLOAT_EQ(output[i].texcoord.x, input[i].texcoord.x) << "at i=" << i;
        EXPECT_FLOAT_EQ(output[i].texcoord.y, input[i].texcoord.y) << "at i=" << i;
    }
}

// UT-22: two vertices at same position get averaged+normalized merged normal
TEST(ModelLoader_MergeNormals, AveragesNormalsAtSharedPosition) {
    std::vector<Vertex> input(2);
    input[0].position = { 0.f, 0.f, 0.f };
    input[0].normal   = { 1.f, 0.f, 0.f };
    input[0].texcoord = { 0.f, 0.f };

    input[1].position = { 0.f, 0.f, 0.f }; // same position
    input[1].normal   = { 0.f, 1.f, 0.f };
    input[1].texcoord = { 1.f, 1.f };

    auto output = ModelLoader::MergeNormals(input);
    glm::vec3 expected = glm::normalize(glm::vec3(1.f, 1.f, 0.f));

    EXPECT_NEAR(output[0].mergedNormal.x, expected.x, 1e-5f);
    EXPECT_NEAR(output[0].mergedNormal.y, expected.y, 1e-5f);
    EXPECT_NEAR(output[0].mergedNormal.z, expected.z, 1e-5f);

    EXPECT_EQ(output[0].mergedNormal, output[1].mergedNormal);
}

// Distinct positions keep their own normals unchanged
TEST(ModelLoader_MergeNormals, DistinctPositionsKeepOwnNormals) {
    std::vector<Vertex> input(2);
    input[0].position = { 0.f, 0.f, 0.f };
    input[0].normal   = { 1.f, 0.f, 0.f };
    input[0].texcoord = { 0.f, 0.f };

    input[1].position = { 1.f, 0.f, 0.f }; // different position
    input[1].normal   = { 0.f, 1.f, 0.f };
    input[1].texcoord = { 1.f, 0.f };

    auto output = ModelLoader::MergeNormals(input);
    EXPECT_NEAR(output[0].mergedNormal.x, 1.f, 1e-5f);
    EXPECT_NEAR(output[0].mergedNormal.y, 0.f, 1e-5f);
    EXPECT_NEAR(output[1].mergedNormal.x, 0.f, 1e-5f);
    EXPECT_NEAR(output[1].mergedNormal.y, 1.f, 1e-5f);
}

// Two vertices at the same position with exactly opposing normals sum to a
// near-zero vector — MergeNormals falls back to the vertex's own normal
// instead of normalizing a (0,0,0) vector.
TEST(ModelLoader_MergeNormals, FallsBackToOwnNormalWhenSumIsNearZero) {
    std::vector<Vertex> input(2);
    input[0].position = { 0.f, 0.f, 0.f };
    input[0].normal   = { 0.f, 1.f, 0.f };
    input[0].texcoord = { 0.f, 0.f };

    input[1].position = { 0.f, 0.f, 0.f }; // same position
    input[1].normal   = { 0.f, -1.f, 0.f }; // exactly opposite -> sum is (0,0,0)
    input[1].texcoord = { 1.f, 1.f };

    auto output = ModelLoader::MergeNormals(input);
    EXPECT_EQ(output[0].mergedNormal, input[0].normal);
    EXPECT_EQ(output[1].mergedNormal, input[1].normal);
}

// -----------------------------------------------------------------------
// File fixture
// Creates OBJ / MTL / BMP files on the fly in a temp directory.
// Passes the absolute path directly to LoadFromOBJ so that RelToAbsPath
// returns it unchanged (PROJECT_ROOT prefix is not applied).
// -----------------------------------------------------------------------

class ModelLoaderFileFixture : public ::testing::Test {
protected:
    std::filesystem::path tmpDir;
    std::filesystem::path objPath;
    std::filesystem::path mtlPath;
    std::filesystem::path texPath;

    void SetUp() override {
        tmpDir  = std::filesystem::temp_directory_path() / "csm_ml_test";
        std::filesystem::create_directories(tmpDir);
        objPath = tmpDir / "test.obj";
        mtlPath = tmpDir / "test.mtl";
        texPath = tmpDir / "test.bmp";
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(tmpDir, ec);
    }

    // Single triangle, optional MTL reference
    void WriteOBJ(bool withMTL = false) {
        std::ofstream ofs(objPath);
        if (withMTL) ofs << "mtllib test.mtl\n";
        ofs << "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
               "vn 0 0 1\n"
               "vt 0 0\nvt 1 0\nvt 0 1\n";
        if (withMTL) ofs << "usemtl TestMat\n";
        ofs << "f 1/1/1 2/2/1 3/3/1\n";
    }

    // MTL with one material, optional texture reference
    void WriteMTL(bool withTex = false) {
        std::ofstream ofs(mtlPath);
        ofs << "newmtl TestMat\n"
               "Ka 0.1 0.1 0.1\n"
               "Kd 0.8 0.2 0.1\n"
               "Ks 1.0 1.0 1.0\n"
               "Ns 128.0\n";
        if (withTex)
            ofs << "map_Kd test.bmp\n";
    }

    // Minimal valid 1x1 white-pixel BMP (24-bit RGB, 58 bytes)
    void WriteTextureBMP() {
        static const uint8_t kBmp[] = {
            // BITMAPFILEHEADER (14 bytes)
            0x42, 0x4D,              // 'BM'
            0x3A, 0x00, 0x00, 0x00, // file size = 58
            0x00, 0x00, 0x00, 0x00, // reserved
            0x36, 0x00, 0x00, 0x00, // pixel data offset = 54
            // BITMAPINFOHEADER (40 bytes)
            0x28, 0x00, 0x00, 0x00, // header size = 40
            0x01, 0x00, 0x00, 0x00, // width = 1
            0x01, 0x00, 0x00, 0x00, // height = 1
            0x01, 0x00,              // color planes = 1
            0x18, 0x00,              // bits per pixel = 24
            0x00, 0x00, 0x00, 0x00, // compression = BI_RGB
            0x04, 0x00, 0x00, 0x00, // image size = 4 bytes
            0x13, 0x0B, 0x00, 0x00, // X pixels/meter (72 DPI)
            0x13, 0x0B, 0x00, 0x00, // Y pixels/meter (72 DPI)
            0x00, 0x00, 0x00, 0x00, // colors in table
            0x00, 0x00, 0x00, 0x00, // important colors
            // Pixel data: white BGR + 1 byte row padding
            0xFF, 0xFF, 0xFF, 0x00
        };
        std::ofstream ofs(texPath, std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(kBmp), sizeof(kBmp));
    }
};

// -----------------------------------------------------------------------
// LoadFromOBJ — no MTL
// -----------------------------------------------------------------------

// UT-25: valid OBJ without MTL returns non-empty matMesh
TEST_F(ModelLoaderFileFixture, ValidOBJReturnsData) {
    WriteOBJ();
    auto data = ModelLoader::LoadFromOBJ<Vertex>(objPath);
    EXPECT_FALSE(data.matMesh.empty());
}

// Each matMesh entry has a non-empty vertex array
TEST_F(ModelLoaderFileFixture, ValidOBJHasVertices) {
    WriteOBJ();
    auto data = ModelLoader::LoadFromOBJ<Vertex>(objPath);
    ASSERT_FALSE(data.matMesh.empty()) << "OBJ failed to load";
    for (const auto& mm : data.matMesh)
        EXPECT_FALSE(mm.mesh.vertexArray.empty()) << "materialID=" << mm.materialID;
}

// Loaded triangle has exactly 3 vertices
TEST_F(ModelLoaderFileFixture, ValidOBJHasThreeVertices) {
    WriteOBJ();
    auto data = ModelLoader::LoadFromOBJ<Vertex>(objPath);
    ASSERT_EQ(data.matMesh.size(), 1u);
    EXPECT_EQ(data.matMesh[0].mesh.vertexArray.size(), 3u);
}

// UT-26: non-existent file returns empty matMesh without throwing
TEST(ModelLoader_Load, MissingOBJReturnsEmpty) {
    auto nonexistent = std::filesystem::temp_directory_path() / "csm_test_no_such_file.obj";
    std::filesystem::remove(nonexistent);
    auto data = ModelLoader::LoadFromOBJ<Vertex>(nonexistent);
    EXPECT_TRUE(data.matMesh.empty());
}

// UT-27: VertexMergedNorm without transformFunc throws runtime_error
TEST_F(ModelLoaderFileFixture, CustomVertexWithoutTransformFuncThrows) {
    WriteOBJ();
    EXPECT_THROW(
        ModelLoader::LoadFromOBJ<VertexMergedNorm>(objPath),
        std::runtime_error
    );
}

// LoadFromOBJ<Vertex> (the default/identity vertex type) also accepts an
// optional transformFunc — a distinct code path from the no-transformFunc
// "direct move" case. Passing identity here still exercises it.
TEST_F(ModelLoaderFileFixture, VertexTypeWithTransformFuncUsesIt) {
    WriteOBJ();
    bool called = false;
    std::function<std::vector<Vertex>(const std::vector<Vertex>&)> identity =
        [&](const std::vector<Vertex>& in) { called = true; return in; };

    auto data = ModelLoader::LoadFromOBJ<Vertex>(objPath, "./", identity);
    EXPECT_TRUE(called);
    ASSERT_FALSE(data.matMesh.empty());
    EXPECT_EQ(data.matMesh[0].mesh.vertexArray.size(), 3u);
}

// The realistic use case for a custom VertexT: loading straight into
// VertexMergedNorm via ModelLoader::MergeNormals as the transformFunc —
// exercises the "Custom Vertex Type, transformFunc provided" branch that
// CreateModelFromOBJ() never reaches (it only ever loads plain Vertex).
TEST_F(ModelLoaderFileFixture, VertexMergedNormWithMergeNormalsTransformFunc) {
    WriteOBJ();
    auto data = ModelLoader::LoadFromOBJ<VertexMergedNorm>(
        objPath, "./", ModelLoader::MergeNormals);
    ASSERT_FALSE(data.matMesh.empty());
    EXPECT_EQ(data.matMesh[0].mesh.vertexArray.size(), 3u);
}

// A relative face index that resolves out of range (here: a negative index
// used before any vertex was defined) makes tinyobjloader's ParseFromFile
// itself fail -> LoadFromOBJ propagates that as a thrown runtime_error.
// (A positive out-of-range index like "f 999" is accepted by tinyobjloader
// without bounds-checking against the vertex count -- it is not a parse
// error there, and would instead crash inside our own InternalLoader when
// indexing attrib.vertices. That is a separate, pre-existing robustness gap,
// not something this test can safely exercise.)
TEST_F(ModelLoaderFileFixture, MalformedOBJThrowsOnParseFailure) {
    {
        std::ofstream ofs(objPath);
        ofs << "f -1 -2 -3\n"; // negative index with zero vertices defined
    }
    EXPECT_THROW(
        ModelLoader::LoadFromOBJ<Vertex>(objPath),
        std::runtime_error
    );
}


// -----------------------------------------------------------------------
// LoadFromOBJ — with MTL
// -----------------------------------------------------------------------

// OBJ + MTL: exactly one material is loaded
TEST_F(ModelLoaderFileFixture, OBJWithMTLLoadsOneMaterial) {
    WriteOBJ(true);
    WriteMTL();
    auto data = ModelLoader::LoadFromOBJ<Vertex>(objPath);
    ASSERT_FALSE(data.matMesh.empty());
    EXPECT_EQ(data.materials.size(), 1u);
}

// Material name from MTL is parsed correctly
TEST_F(ModelLoaderFileFixture, OBJWithMTLMaterialNameCorrect) {
    WriteOBJ(true);
    WriteMTL();
    auto data = ModelLoader::LoadFromOBJ<Vertex>(objPath);
    ASSERT_EQ(data.materials.size(), 1u);
    EXPECT_EQ(data.materials[0].name, "TestMat");
}

// Kd values from MTL are parsed into diffuse[3]
TEST_F(ModelLoaderFileFixture, OBJWithMTLDiffuseColorCorrect) {
    WriteOBJ(true);
    WriteMTL();
    auto data = ModelLoader::LoadFromOBJ<Vertex>(objPath);
    ASSERT_EQ(data.materials.size(), 1u);
    EXPECT_NEAR(data.materials[0].diffuse[0], 0.8f, 1e-4f);
    EXPECT_NEAR(data.materials[0].diffuse[1], 0.2f, 1e-4f);
    EXPECT_NEAR(data.materials[0].diffuse[2], 0.1f, 1e-4f);
}

// Ns (shininess) from MTL is parsed
TEST_F(ModelLoaderFileFixture, OBJWithMTLShininessCorrect) {
    WriteOBJ(true);
    WriteMTL();
    auto data = ModelLoader::LoadFromOBJ<Vertex>(objPath);
    ASSERT_EQ(data.materials.size(), 1u);
    EXPECT_NEAR(data.materials[0].shininess, 128.0f, 1e-4f);
}

// map_Kd texture reference is stored in diffuse_texname
TEST_F(ModelLoaderFileFixture, OBJWithMTLTexturePathRecorded) {
    WriteTextureBMP();   // valid 1x1 BMP on disk
    WriteOBJ(true);
    WriteMTL(true);      // map_Kd test.bmp
    auto data = ModelLoader::LoadFromOBJ<Vertex>(objPath);
    ASSERT_EQ(data.materials.size(), 1u);
    EXPECT_FALSE(data.materials[0].diffuse_texname.empty());
}
