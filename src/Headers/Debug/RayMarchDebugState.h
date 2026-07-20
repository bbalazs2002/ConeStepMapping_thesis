#pragma once

#include <GL/glew.h>
#include "Utils/Camera.h"

struct RayMarchDebugConfig {
    bool showDebug     = false;
    bool showSteps     = false;
    bool showEnterExit = false;
    bool showCones     = false;
    bool showRay       = false;
    bool showHitPoint  = false;
    int  primitiveID   = -1;
};

struct RayMarchDebugState {
    Camera debugCamera;

    GLuint debugVisualSSBO    = 0;  // binding 0: step positions + CPU config flags
    GLuint debugNumericalSSBO = 0;  // binding 1: numerical trace data

    RayMarchDebugConfig config;
};
