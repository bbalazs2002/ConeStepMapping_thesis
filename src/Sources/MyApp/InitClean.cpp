#include "MyApp.h"

#include "Headers/Renderer/SkyboxRenderer.h"
#include "Headers/Renderer/AxesRenderer.h"
#include "Headers/Renderer/DebugRenderer.h"
#include "Headers/RendererVisitor/OpenGLRendererVisitor.h"
#include "Headers/RayMarching/ConemapGenerator.h"
#include "Headers/RayMarching/LinearSearch.h"
#include "Headers/RayMarching/ConeStepMapping.h"
#include "Headers/Model/RayMarchedModel.h"
#include "Headers/Model/Mesh.h"
#include "Headers/Material/Material.h"
#include "Headers/Types.h"
#include "Utils/ModelLoader.h"
#include <array>
#include <filesystem>
#include <cstring>

bool MyApp::Init()
{
    SetupDebugCallback();

    glClearColor(0.125f, 0.25f, 0.5f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glCullFace(GL_BACK);

    std::strncpy(m_skyboxPathBuf, "Assets/sky1", sizeof(m_skyboxPathBuf) - 1);

    // 1. Shaders --------------------------------------------------------------

    m_shaderManager.Load("model", {
        { GL_VERTEX_SHADER,   "src/Shaders/Models/Vert_Model.vert" },
        { GL_FRAGMENT_SHADER, "src/Shaders/Models/Frag_Model.frag" }
    });
    m_shaderManager.Load("model_selected", {
        { GL_VERTEX_SHADER,   "src/Shaders/Models/Vert_ModelSelected.vert" },
        { GL_FRAGMENT_SHADER, "src/Shaders/Models/Frag_ModelSelected.frag" }
    });
    m_shaderManager.Load("axes", {
        { GL_VERTEX_SHADER,   "src/Shaders/Axes/Vert_Axes.vert" },
        { GL_FRAGMENT_SHADER, "src/Shaders/Axes/Frag_Axes.frag" }
    });
    m_shaderManager.Load("skybox", {
        { GL_VERTEX_SHADER,   "src/Shaders/Skybox/Vert_Skybox.vert" },
        { GL_FRAGMENT_SHADER, "src/Shaders/Skybox/Frag_Skybox.frag" }
    });

    // Ray marching shader permutations:
    //   ls_HN     - Linear search, height nearest
    //   ls_HB     - Linear search, height bilinear
    //   csm_HN_CN - CSM, height nearest,   cone nearest
    //   csm_HB_CN - CSM, height bilinear,  cone nearest
    //   csm_HN_CB - CSM, height nearest,   cone bilinear
    //   csm_HB_CB - CSM, height bilinear,  cone bilinear

    const std::vector<ShaderStage> lsStages = {
        { GL_VERTEX_SHADER,   "src/Shaders/RayMarching/Vert_RM.vert" },
        { GL_GEOMETRY_SHADER, "src/Shaders/RayMarching/Geom_RM_abcd.geom" },
        { GL_FRAGMENT_SHADER, "src/Shaders/RayMarching/Frag_LinearSearch.frag" }
    };
    const std::vector<ShaderStage> csmStages = {
        { GL_VERTEX_SHADER,   "src/Shaders/RayMarching/Vert_RM.vert" },
        { GL_GEOMETRY_SHADER, "src/Shaders/RayMarching/Geom_RM_abcd.geom" },
        { GL_FRAGMENT_SHADER, "src/Shaders/RayMarching/Frag_ConeStepMapping.frag" }
    };

    m_shaderManager.Load("ls_HN",     lsStages);
    m_shaderManager.Load("ls_HB",     lsStages,  { "INTERP_HEIGHT" });
    m_shaderManager.Load("csm_HN_CN", csmStages);
    m_shaderManager.Load("csm_HB_CN", csmStages, { "INTERP_HEIGHT" });
    m_shaderManager.Load("csm_HN_CB", csmStages, { "INTERP_CONE" });
    m_shaderManager.Load("csm_HB_CB", csmStages, { "INTERP_HEIGHT", "INTERP_CONE" });

    // Conemap generation: original and conservative variant
    const std::vector<ShaderStage> conemapStages = {
        { GL_COMPUTE_SHADER, "src/Shaders/Conemap/Comp_Conemap.comp" }
    };
    m_shaderManager.Load("conemap_original",    conemapStages);
    m_shaderManager.Load("conemap_conservative", conemapStages, { "CONSERVATIVE" });

    // Debug renderer shaders
    m_shaderManager.Load("debug_ray", {
        { GL_VERTEX_SHADER,   "src/Shaders/Debug/Vert_DebugRay.vert" },
        { GL_FRAGMENT_SHADER, "src/Shaders/Debug/Frag_Debug.frag" }
    });
    m_shaderManager.Load("debug_enterExit", {
        { GL_VERTEX_SHADER,   "src/Shaders/Debug/Vert_DebugEnterExit.vert" },
        { GL_FRAGMENT_SHADER, "src/Shaders/Debug/Frag_Debug.frag" }
    });
    m_shaderManager.Load("debug_steps", {
        { GL_VERTEX_SHADER,   "src/Shaders/Debug/Vert_DebugSteps.vert" },
        { GL_FRAGMENT_SHADER, "src/Shaders/Debug/Frag_Debug.frag" }
    });
    m_shaderManager.Load("debug_cones", {
        { GL_VERTEX_SHADER,   "src/Shaders/Debug/Vert_DebugCones.vert" },
        { GL_FRAGMENT_SHADER, "src/Shaders/Debug/Frag_Debug.frag" }
    });
    m_shaderManager.Load("debug_hit", {
        { GL_VERTEX_SHADER,   "src/Shaders/Debug/Vert_DebugHit.vert" },
        { GL_FRAGMENT_SHADER, "src/Shaders/Debug/Frag_Debug.frag" }
    });

    // 2. RAII GL members ------------------------------------------------------
    m_skyboxRenderer   = std::make_unique<SkyboxRenderer>(m_shaderManager.Get("skybox"),
        m_textureManager.GetOrLoadCubemap(
            std::array<std::filesystem::path, 6>{
                "Assets/sky1/px.png", "Assets/sky1/nx.png",
                "Assets/sky1/py.png", "Assets/sky1/ny.png",
                "Assets/sky1/pz.png", "Assets/sky1/nz.png"
            }, false));
    m_axesRenderer     = std::make_unique<AxesRenderer>(m_shaderManager.Get("axes"));
    m_rendererVisitor  = std::make_unique<OpenGLRendererVisitor>(m_camera);
    m_conemapGenerator = std::make_unique<ConemapGenerator>(
        m_shaderManager.Get("conemap_original"),
        m_shaderManager.Get("conemap_conservative"));

    // 3. Techniques -----------------------------------------------------------
    // Default permutation: height bilinear (INTERP_HEIGHT), cone nearest.
    auto ls  = std::make_shared<LinearSearch>   (m_shaderManager.Get("ls_HB"));
    auto csm = std::make_shared<ConeStepMapping>(m_shaderManager.Get("csm_HB_CN"));
    m_lsTech  = ls.get();
    m_csmTech = csm.get();
    m_techniques["linear_search"]     = ls;
    m_techniques["cone_step_mapping"] = csm;

    // 4. Camera ---------------------------------------------------------------
    m_camera.SetView(
        glm::vec3(0.f, 10.f, 5.f),
        glm::vec3(0.f,  4.f, 0.f),
        glm::vec3(0.f,  1.f, 0.f)
    );
    m_camera.SetAspect(static_cast<float>(m_windowSize.x) / m_windowSize.y);
    m_cameraManipulator.SetCamera(&m_camera);

    // 5. Debug SSBOs + DebugRenderer (after m_rendererVisitor) ---------------
    InitDebugSSBOs();

    // 6. Scene ----------------------------------------------------------------

    // --- Heightmap surface ---
    auto surface = std::make_shared<RayMarchedModel>("Heightmap Surface");
    surface->SetTechnique(m_techniques["cone_step_mapping"]);
    surface->SetSelectedProgram(m_shaderManager.Get("model_selected"));
    surface->SetLightDir(m_lightDir);

    MeshObject<VertexMergedNorm> planeMesh;
    planeMesh.vertexArray = {
        { { -2.5f, 0.f, -2.5f }, { 0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f } },
        { {  2.5f, 0.f, -2.5f }, { 0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }, { 1.f, 0.f } },
        { {  2.5f, 0.f,  2.5f }, { 0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }, { 1.f, 1.f } },
        { { -2.5f, 0.f,  2.5f }, { 0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 1.f } },
    };
    planeMesh.indexArray = { 0, 1, 2,  0, 2, 3 };

    auto planeMeshObj = std::make_shared<Mesh>();
    planeMeshObj->Build<VertexMergedNorm>(std::move(planeMesh));

    Material surfaceMat;
    surfaceMat.SetName("surface_default");
    surfaceMat.SetDiffuseColor(glm::vec3(0.8f));
    planeMeshObj->SetMaterial(m_materialManager.GetOrCreate(surfaceMat));

    surface->AddMesh(std::move(planeMeshObj));

    auto heightmap = m_textureManager.GetOrLoad("Assets/HMaps/heightmap_dot.png", false);
    surface->SetHeightmap(heightmap, m_conemapGenerator.get());

    m_sceneManager.Add(std::move(surface));

    return true;
}

void MyApp::Clean()
{
    CleanupDebugSSBOs();
    m_debugRenderer.reset();
    m_rendererVisitor.reset();
    m_axesRenderer.reset();
    m_skyboxRenderer.reset();
    m_conemapGenerator.reset();
    m_sceneManager.Clear();
    m_shaderManager.DeleteAll();
}