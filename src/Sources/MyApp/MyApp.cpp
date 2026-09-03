#include "MyApp.h"

#include "Interfaces/ICommand.h"
#include "Headers/Renderer/SkyboxRenderer.h"
#include "Headers/Renderer/AxesRenderer.h"
#include "Headers/Renderer/DebugRenderer.h"
#include "Headers/RendererVisitor/OpenGLRendererVisitor.h"
#include "Headers/RayMarching/ConemapGenerator.h"
#include "Headers/RayMarching/LinearSearch.h"
#include "Headers/RayMarching/ConeStepMapping.h"

MyApp::MyApp()  = default;
MyApp::~MyApp() = default;

// -- Update --------------------------------------------------------------------

void MyApp::Update(const SUpdateInfo& info)
{
    m_commandQueue->Execute();
    m_sceneManager.Update(info);
    m_cameraManipulator.Update(info.DeltaTimeInSec);
    m_elapsedTime = info.ElapsedTimeInSec;
}

// -- Render --------------------------------------------------------------------

void MyApp::Render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Reset debug indirect counts each frame so stale counts don't persist
    // when the targeted primitive doesn't produce debug output (fixes Primitive ID filter).
    if (m_debugState.config.showDebug && m_debugState.debugNumericalSSBO) {
        struct DrawCmd { GLuint count = 0, instanceCount = 1, first = 0, baseInstance = 0; };
        DrawCmd resetCmds[2];
        glNamedBufferSubData(m_debugState.debugNumericalSSBO, 0, sizeof(resetCmds), resetCmds);
    }

    m_sceneManager.Render(*m_rendererVisitor);              // scene
    m_skyboxRenderer->Render(m_camera);                     // skybox
    if (m_showAxes)
        m_axesRenderer->Render(m_camera);                   // axes
    if (m_debugState.config.showDebug)
        m_debugRenderer->Render(m_debugState, m_camera);    // debug
}

// -- UpdateTechniquePrograms ---------------------------------------------------

void MyApp::UpdateTechniquePrograms()
{
    if (!m_lsTech || !m_csmTech) return;

    std::string lsName = m_interpHeight ? "ls_HB" : "ls_HN";

    std::string csmName = "csm";
    csmName += m_interpHeight ? "_HB" : "_HN";
    csmName += m_interpCone   ? "_CB" : "_CN";

    m_lsTech->SetProgramID( m_shaderManager.Get(lsName));
    m_csmTech->SetProgramID(m_shaderManager.Get(csmName));
}
