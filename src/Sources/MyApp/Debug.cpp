#include "MyApp.h"

#include "Headers/Renderer/DebugRenderer.h"
#include "Headers/RendererVisitor/OpenGLRendererVisitor.h"
#include "Utils/SDL_GLDebugMessageCallback.h"
#include <fstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

void MyApp::SetupDebugCallback()
{
    GLint context_flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
    if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
        glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, GL_DONT_CARE, 0, nullptr, GL_FALSE);
        glDebugMessageCallback(SDL_GLDebugMessageCallback, nullptr);
    }
}

// debugVisualSSBO layout (vec4 slots, binding 0):
//   [0]    GS:  stepCount
//   [1-4]  GS:  invM columns (texture-->scene)
//   [5]    CPU: (showDebug, showSteps, showEnterExit, showCones)
//   [6]    CPU: (showRay, showHitPoint, primitiveID, techniqueID)
//   [7]    CPU: debugCamera.eye
//   [8]    CPU: debugCamera.at
//   [9+i]  GS:  step position in texture space (DBG_WRITE_STEP)
//
// debugNumericalSSBO layout (binding 1):
//   [offset 0]  GS:  uvec4 indirectSteps = {stepCount, 1, 0, 0}
//   [offset 16] GS:  uvec4 indirectCones = {3, stepCount, 0, 0}  (instanced LINE_STRIP)
//   [offset 32] debugNumerical[] flexible array (vec4):
//     [0]    GS:  (stepCount, flags, 0, 0)
//     [1]    CPU: debugCamera.eye
//     [2]    CPU: debugCamera.at
//     [3-6]  GS:  M (scene-->tex) columns
//     [7]    GS:  TexEye
//     [8-11] GS:  T (scene-->unit prism) columns
//     [12]   GS:  T_eye
//     [13]   GS:  TexEnter
//     [14]   GS:  TexExit
//     [15]   GS:  TexDir
//     [16]   GS:  hit UV (w=1) or vec4(0)
//     [17-19]GS:  vertex[0-2] scene positions
//     [20-22]GS:  vertex[0-2] UV coordinates
//     [23+]  GS:  (ti,t,h,tan) + (ui,0) per step

void MyApp::InitDebugSSBOs()
{
    // debugVisualSSBO: 9 header slots + 512 step positions = 521 vec4s
    glCreateBuffers(1, &m_debugState.debugVisualSSBO);
    glNamedBufferData(m_debugState.debugVisualSSBO,
        (9 + 512) * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);

    // debugNumericalSSBO: 2 uvec4 (indirect commands) + 23 header vec4s + 512x2 step vec4s
    glCreateBuffers(1, &m_debugState.debugNumericalSSBO);
    glNamedBufferData(m_debugState.debugNumericalSSBO,
        2 * sizeof(glm::uvec4) + (23 + 512 * 2) * sizeof(glm::vec4),
        nullptr, GL_DYNAMIC_DRAW);

    // Initialize indirect command slots to {0, 1, 0, 0}
    // so the first frame draws 0 vertices instead of a garbage count.
    struct DrawCommand { GLuint count = 0, instanceCount = 1, first = 0, baseInstance = 0; };
    DrawCommand initCmds[2];
    glNamedBufferSubData(m_debugState.debugNumericalSSBO,
        0, sizeof(initCmds), initCmds);

    m_rendererVisitor->SetDebugState(&m_debugState);

    // Initial debug camera position
    m_debugState.debugCamera.SetView(
        glm::vec3(0.f, 10.f, 5.f),
        glm::vec3(0.f,  0.f, 0.f),
        glm::vec3(0.f,  1.f, 0.f)
    );

    // Create DebugRenderer with all shader programs
    m_debugRenderer = std::make_unique<DebugRenderer>(
        m_shaderManager.Get("debug_ray"),
        m_shaderManager.Get("debug_enterExit"),
        m_shaderManager.Get("debug_steps"),
        m_shaderManager.Get("debug_cones"),
        m_shaderManager.Get("debug_hit")
    );
}

void MyApp::CleanupDebugSSBOs()
{
    glDeleteBuffers(1, &m_debugState.debugVisualSSBO);
    glDeleteBuffers(1, &m_debugState.debugNumericalSSBO);
    m_debugState.debugVisualSSBO    = 0;
    m_debugState.debugNumericalSSBO = 0;
}

void MyApp::ExportDebugLog()
{
    // -- Read back SSBO header (23 vec4 slots) --------------------------------
    const GLintptr kNumericalOffset = 2 * static_cast<GLintptr>(sizeof(glm::uvec4));

    glm::vec4 hdr[23];
    glGetNamedBufferSubData(m_debugState.debugNumericalSSBO,
        kNumericalOffset, sizeof(hdr), hdr);

    int  stepCount = static_cast<int>(hdr[0].x + 0.5f);
    int  flags     = static_cast<int>(hdr[0].y + 0.5f);
    bool wasHit    = hdr[16].w > 0.5f;

    // -- Read back per-step data (starts at slot 23) ---------------------------
    int readCount = std::min(stepCount, 512);
    std::vector<glm::vec4> stepData(readCount * 2);
    if (readCount > 0) {
        glGetNamedBufferSubData(m_debugState.debugNumericalSSBO,
            kNumericalOffset + 23 * static_cast<GLintptr>(sizeof(glm::vec4)),
            readCount * 2 * static_cast<GLintptr>(sizeof(glm::vec4)),
            stepData.data());
    }

    // -- Build timestamped filename --------------------------------------------
    std::time_t now = std::time(nullptr);
    char timeBuf[32];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y%m%d_%H%M%S", std::localtime(&now));
    std::string filename = std::string("debug_") + timeBuf + ".log";

    std::ofstream f(filename);
    if (!f.is_open()) return;

    f << std::fixed << std::setprecision(6);

    // -- File header -----------------------------------------------------------
    f << "# Cone Step Mapping - Debug Export\n";
    f << "# " << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << "\n";
    f << "#\n";

    int primID = m_debugState.config.primitiveID;
    if (primID < 0)
        f << "# Primitive: auto (last rendered)\n";
    else
        f << "# Primitive: " << primID << "\n";

    f << "# Heightmap: " << m_heightMaps[m_activeHeightmapIdx] << "\n";
    f << "# Steps:  " << stepCount << "\n";
    f << "# Hit:    " << (wasHit ? "YES" : "NO");
    if (wasHit)
        f << "  UV=(" << hdr[16].x << ", " << hdr[16].y << ")";
    f << "\n";

    if (flags) {
        f << "# Flags:";
        if (flags & 1) f << " max_steps";
        if (flags & 2) f << " exited_prism";
        if (flags & 4) f << " converged";
        f << "\n";
    }

    // -- Primitive vertices ----------------------------------------------------
    f << "#\n";
    f << "# [Primitive vertices]\n";
    f << "# v0 scene: " << hdr[17].x << "  " << hdr[17].y << "  " << hdr[17].z
      << "   UV: " << hdr[20].x << "  " << hdr[20].y << "\n";
    f << "# v1 scene: " << hdr[18].x << "  " << hdr[18].y << "  " << hdr[18].z
      << "   UV: " << hdr[21].x << "  " << hdr[21].y << "\n";
    f << "# v2 scene: " << hdr[19].x << "  " << hdr[19].y << "  " << hdr[19].z
      << "   UV: " << hdr[22].x << "  " << hdr[22].y << "\n";

    // -- Ray in texture space --------------------------------------------------
    f << "#\n";
    f << "# [Ray - texture space]\n";
    f << "# Eye:   " << hdr[7].x  << "  " << hdr[7].y  << "  " << hdr[7].z  << "\n";
    f << "# Enter: " << hdr[13].x << "  " << hdr[13].y << "  " << hdr[13].z << "\n";
    f << "# Exit:  " << hdr[14].x << "  " << hdr[14].y << "  " << hdr[14].z << "\n";
    f << "# Dir:   " << hdr[15].x << "  " << hdr[15].y << "  " << hdr[15].z << "\n";

    // -- Debug camera ---------------------------------------------------------
    f << "#\n";
    f << "# [Debug camera - scene space]\n";
    f << "# Eye: " << hdr[1].x << "  " << hdr[1].y << "  " << hdr[1].z << "\n";
    f << "# At:  " << hdr[2].x << "  " << hdr[2].y << "  " << hdr[2].z << "\n";

    // -- M matrix (scene --> texture space) -----------------------------------
    f << "#\n";
    f << "# [M - scene to texture space]  (row-major)\n";
    for (int row = 0; row < 4; ++row)
        f << "# M[" << row << "]:  "
          << hdr[3][row] << "  " << hdr[4][row] << "  "
          << hdr[5][row] << "  " << hdr[6][row] << "\n";

    // -- T matrix (scene --> unit prism space) ---------------------------------
    f << "#\n";
    f << "# [T - scene to unit prism space]  (row-major)\n";
    for (int row = 0; row < 4; ++row)
        f << "# T[" << row << "]:  "
          << hdr[8][row]  << "  " << hdr[9][row]  << "  "
          << hdr[10][row] << "  " << hdr[11][row] << "\n";

    // -- Per-step CSV table ----------------------------------------------------
    f << "#\n";
    f << "step, ti, t, height, tan, u, v, w\n";
    for (int i = 0; i < readCount; ++i) {
        const glm::vec4& num = stepData[i * 2];
        const glm::vec4& pos = stepData[i * 2 + 1];
        f << i     << ", "
          << num.x << ", "
          << num.y << ", "
          << num.z << ", "
          << num.w << ", "
          << pos.x << ", "
          << pos.y << ", "
          << pos.z << "\n";
    }

    f.close();

    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
        "Export complete", filename.c_str(), nullptr);
}