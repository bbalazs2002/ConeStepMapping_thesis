#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include "Interfaces/IGraphicsApp.h"
#include "Headers/Command/CommandQueue.h"
#include "Headers/Manager/SceneManager.h"
#include "Headers/Manager/ShaderManager.h"
#include "Headers/Manager/MaterialManager.h"
#include "Headers/Manager/TextureManager.h"
#include "Headers/GUIVisitor/ImGuiVisitor.h"
#include "Headers/Debug/RayMarchDebugState.h"
#include "Utils/Camera.h"
#include "Utils/CameraManipulator.h"

interface IRayMarchingTechnique;
class SkyboxRenderer;
class AxesRenderer;
class DebugRenderer;
class OpenGLRendererVisitor;
class ConemapGenerator;
class LinearSearch;
class ConeStepMapping;
class RayMarchedModel;
class Model;
interface ISceneObject;

class MyApp : public IGraphicsApp {
public:
    MyApp();
    ~MyApp() override;

    bool Init()                                    override;
    void Update(const SUpdateInfo&)                override;
    void Render()                                  override;
    void RenderGUI()                               override;
    void Clean()                                   override;

    void KeyboardDown(const SDL_KeyboardEvent&)    override;
    void KeyboardUp  (const SDL_KeyboardEvent&)    override;
    void MouseMove   (const SDL_MouseMotionEvent&) override;
    void MouseDown   (const SDL_MouseButtonEvent&) override;
    void MouseUp     (const SDL_MouseButtonEvent&) override;
    void MouseWheel  (const SDL_MouseWheelEvent&)  override;
    void Resize      (int w, int h)                override;
    void OtherEvent  (const SDL_Event&)            override;

protected:
    void SetupDebugCallback();
    void InitDebugSSBOs();
    void CleanupDebugSSBOs();
    void RenderDebugPanel();
    void ExportDebugLog();
    void UpdateTechniquePrograms();

    std::shared_ptr<RayMarchedModel> CreateDefaultRayMarchedModel(const std::string& name);
    std::shared_ptr<Model>           CreateModelFromOBJ(const std::string& path);

    // Managers — GL-context-independent value members
    CommandQueue    m_commandQueue;
    SceneManager    m_sceneManager;
    ShaderManager   m_shaderManager;
    MaterialManager m_materialManager;
    TextureManager  m_textureManager;

    // RAII GL objects — constructed in Init() after GL context is ready
    std::unique_ptr<SkyboxRenderer>        m_skyboxRenderer;
    std::unique_ptr<AxesRenderer>          m_axesRenderer;
    std::unique_ptr<OpenGLRendererVisitor> m_rendererVisitor;
    std::unique_ptr<ConemapGenerator>      m_conemapGenerator;
    std::unique_ptr<DebugRenderer>         m_debugRenderer;

    std::map<std::string, std::shared_ptr<IRayMarchingTechnique>> m_techniques;

    // Raw (non-owning) pointers into m_techniques for direct program swapping
    LinearSearch*    m_lsTech  = nullptr;
    ConeStepMapping* m_csmTech = nullptr;

    // GL-independent visitor — reference binds to m_commandQueue (declared above)
    ImGuiVisitor m_imguiVisitor{ m_commandQueue };

    Camera            m_camera;
    CameraManipulator m_cameraManipulator;

    RayMarchDebugState m_debugState;

    glm::ivec2 m_windowSize  { 640, 480 };
    float      m_elapsedTime = 0.f;

    // GUI state
    bool m_showAxes           = true;
    int  m_selectedIndex      = -1;
    int  m_activeHeightmapIdx = 0;
    bool m_objLoadFailed      = false;

    // Global light direction (applied to all RayMarchedModels)
    glm::vec3 m_lightDir = glm::vec3(0.0f, 5.0f, 0.0f);

    // Skybox path input buffer (initialized to default folder in Init)
    char m_skyboxPathBuf[256] = {};

    // .obj path input buffer for Add .obj Model
    char m_objPathBuf[512] = {};

    // Conemap sampling permutation state
    bool m_interpHeight = true;   // true = bilinear height (default)
    bool m_interpCone   = false;  // true = bilinear cone tangent
    bool m_conservative = false;  // true = conservative conemap generation

    const std::vector<std::string> m_heightMaps{
        "Assets/HMaps/heightmap_dot.png",
        "Assets/HMaps/spikes.png",
        "Assets/HMaps/hemisphere.png",
        "Assets/HMaps/cone.jpg",
        "Assets/HMaps/heightmap-terrain.png",
        "Assets/HMaps/heightmap-terrain2.png",
        "Assets/HMaps/heightmap-river.png",
        "Assets/HMaps/heightmap-pyramid.jpg",
        "Assets/HMaps/heightmap-geometries.png",
        "Assets/HMaps/Earth-heightmap-small.png",
        "Assets/HMaps/desert-heightmap.jpg",
    };
};
