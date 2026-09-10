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

    // Set whenever config or debugCamera changes via the GUI; cleared once the
    // header has been re-uploaded to the SSBOs. Starts true so the first frame
    // with showDebug enabled always writes a fresh header.
    bool dirty = true;
};
