#pragma once

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>

// ---------------------------------------------------------------------------
// Shader loading
// ---------------------------------------------------------------------------

struct ShaderStage {
    GLenum      type;   // GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc.
    std::string path;
};

// ---------------------------------------------------------------------------
// Frame update
// ---------------------------------------------------------------------------

struct SUpdateInfo {
    float ElapsedTimeInSec = 0.0f;
    float DeltaTimeInSec   = 0.0f;
};
