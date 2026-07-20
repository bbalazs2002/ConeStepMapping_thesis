#include <gtest/gtest.h>
#include "Utils/ModelLoader.h"
#include "Utils/GLUtils.hpp"
#include <filesystem>
#include <glm/glm.hpp>

// -----------------------------------------------------------------------
// Path utilities
// -----------------------------------------------------------------------

// UT-18: relative path → absolute (prepends PROJECT_ROOT)
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

// UT-20: relative texture name + texFolder → absolute path containing folder
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

// UT-22: two vertices at same position get the averaged+normalized merged normal
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

    // Both vertices are at the same position → same merged normal
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

// -----------------------------------------------------------------------
// LoadFromOBJ
// -----------------------------------------------------------------------

// UT-25: valid OBJ file returns non-empty data
TEST(ModelLoader_Load, ValidOBJReturnsData) {
    auto data = ModelLoader::LoadFromOBJ<Vertex>(
        "Assets/Fire_Extinguisher/Fire_Extinguisher.obj");
    EXPECT_FALSE(data.matMesh.empty());
    EXPECT_FALSE(data.materials.empty());
}

// Each matMesh entry has a non-empty vertex array
TEST(ModelLoader_Load, ValidOBJHasVertices) {
    auto data = ModelLoader::LoadFromOBJ<Vertex>(
        "Assets/Fire_Extinguisher/Fire_Extinguisher.obj");
    for (const auto& mm : data.matMesh)
        EXPECT_FALSE(mm.mesh.vertexArray.empty()) << "Empty mesh for materialID=" << mm.materialID;
}

// UT-26: non-existent file returns empty matMesh (no throw)
TEST(ModelLoader_Load, MissingOBJReturnsEmpty) {
    auto data = ModelLoader::LoadFromOBJ<Vertex>("Assets/does_not_exist.obj");
    EXPECT_TRUE(data.matMesh.empty());
}

// UT-27: VertexMergedNorm without transformFunc throws runtime_error
TEST(ModelLoader_Load, CustomVertexWithoutTransformFuncThrows) {
    EXPECT_THROW(
        ModelLoader::LoadFromOBJ<VertexMergedNorm>("Assets/uv-sphere-32.obj"),
        std::runtime_error
    );
}
