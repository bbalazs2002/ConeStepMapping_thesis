#pragma once

#include <memory>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// GLEW
#include <GL/glew.h>

struct ShaderProgramCollection {
    GLuint programID = 0;
    GLuint conemapID = 0;
};

// Model params
struct ModelBaseParams {
    ShaderProgramCollection shaderPrograms;
    const char* name = "";
    bool show = true;
    int drawMode = GL_TRIANGLES;
};
struct ModelParams {
    ShaderProgramCollection shaderPrograms;
    const char* name = "";
    bool show = true;
    bool wireFrame = false;
    int drawMode = GL_TRIANGLES;
};
struct RayMarchedSurfaceParams {
    ShaderProgramCollection shaderPrograms;
    const char* name = "";
    bool show = true;
    bool wireFrame = false;
    int drawMode = GL_TRIANGLES;
};

// Render params
struct MeshRenderParams {
    GLuint progID;
    int drawMode;
};
struct MeshRenderSelectionParams {
    GLuint progID;
    int drawMode;
};

struct RenderParams {
    float lineWidth = 1.f;
    glm::vec3 cameraPos = glm::vec3(0, 0, 0);
    int modelIndex = 0;
    glm::ivec2 windowSize = glm::ivec2(0, 0);
    glm::mat4 viewProj = glm::identity<glm::mat4>();
    void* otherData = nullptr;
};

// Update info
struct SUpdateInfo
{
    float ElapsedTimeInSec = 0.0f;	// Elapsed time since start of the program
    float DeltaTimeInSec = 0.0f;	// Elapsed time since last update
};