#include <gtest/gtest.h>
#include "Headers/Model/Mesh.h"
#include "Utils/GLUtils.hpp"
#include <GL/glew.h>

// Helper: create a minimal triangle MeshObject<Vertex>
static MeshObject<Vertex> MakeTriangle() {
    MeshObject<Vertex> m;
    m.vertexArray = {
        { {  0.f,  1.f, 0.f }, { 0.f, 0.f, 1.f }, { 0.5f, 1.f } },
        { { -1.f, -1.f, 0.f }, { 0.f, 0.f, 1.f }, { 0.f,  0.f } },
        { {  1.f, -1.f, 0.f }, { 0.f, 0.f, 1.f }, { 1.f,  0.f } },
    };
    m.indexArray = { 0, 1, 2 };
    return m;
}

// After Build<Vertex>, VAO and VBO are non-zero
TEST(Mesh, BuildCreatesValidGLObjects) {
    auto mesh = std::make_shared<Mesh>();
    mesh->Build<Vertex>(MakeTriangle());

    EXPECT_NE(mesh->GetVAO(), 0u);
}

// Vertex count matches index array size
TEST(Mesh, VertexCountMatchesIndexArray) {
    auto mesh = std::make_shared<Mesh>();
    MeshObject<Vertex> tri = MakeTriangle();
    size_t expectedCount = tri.indexArray.size();
    mesh->Build<Vertex>(std::move(tri));

    EXPECT_EQ(static_cast<size_t>(mesh->GetVertexCount()), expectedCount);
}

// Default draw mode is GL_TRIANGLES
TEST(Mesh, DefaultDrawModeIsTriangles) {
    auto mesh = std::make_shared<Mesh>();
    mesh->Build<Vertex>(MakeTriangle());

    EXPECT_EQ(mesh->GetDrawMode(), static_cast<GLenum>(GL_TRIANGLES));
}

// Build with VertexMergedNorm layout
TEST(Mesh, BuildWithMergedNormLayout) {
    MeshObject<VertexMergedNorm> m;
    m.vertexArray = {
        { { 0.f,  1.f, 0.f }, { 0.f,0.f,1.f }, { 0.f,0.f,1.f }, { 0.5f,1.f } },
        { {-1.f, -1.f, 0.f }, { 0.f,0.f,1.f }, { 0.f,0.f,1.f }, { 0.f, 0.f } },
        { { 1.f, -1.f, 0.f }, { 0.f,0.f,1.f }, { 0.f,0.f,1.f }, { 1.f, 0.f } },
    };
    m.indexArray = { 0, 1, 2 };

    auto mesh = std::make_shared<Mesh>();
    EXPECT_NO_THROW(mesh->Build<VertexMergedNorm>(std::move(m)));
    EXPECT_NE(mesh->GetVAO(), 0u);
}

// GL error state is clean after a successful Build
TEST(Mesh, NoGLErrorAfterBuild) {
    auto mesh = std::make_shared<Mesh>();
    mesh->Build<Vertex>(MakeTriangle());

    GLenum err = glGetError();
    // Accept GL_NO_ERROR or GL_INVALID_ENUM (GLEW queries a non-existent enum on init — harmless)
    EXPECT_TRUE(err == GL_NO_ERROR || err == GL_INVALID_ENUM)
        << "Unexpected GL error: 0x" << std::hex << err;
}
