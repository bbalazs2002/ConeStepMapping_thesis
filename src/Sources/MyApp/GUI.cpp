#include "MyApp.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <filesystem>
#include "Headers/Model/RayMarchedModel.h"
#include "Headers/Renderer/SkyboxRenderer.h"
#include "Headers/RayMarching/ConemapGenerator.h"
#include "Headers/Command/SetSelectedCommand.h"
#include "Headers/Command/CreateRMObjectCommand.h"
#include "Headers/Command/CreateObjObjectCommand.h"
#include "Headers/Command/DeleteObjectCommand.h"
#include "Headers/Command/SetTechniqueCommand.h"
#include "Headers/Command/SetHeightmapCommand.h"

void MyApp::RenderGUI()
{
    // Fullscreen dockspace host
    ImGuiID dockspace_id;
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags host_flags =
            ImGuiWindowFlags_NoTitleBar       | ImGuiWindowFlags_NoCollapse       |
            ImGuiWindowFlags_NoResize         | ImGuiWindowFlags_NoMove           |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus  |
            ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("DockspaceHost", nullptr, host_flags);
        dockspace_id = ImGui::GetID("MainDockspace");
        ImGui::PopStyleVar(3);
        ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::End();
    }

    static bool layoutInitialized = false;
    if (!layoutInitialized) {
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

        ImGuiID dock_top, dock_bottom;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.28f, &dock_bottom, &dock_top);

        ImGuiID dock_left, dock_center;
        ImGui::DockBuilderSplitNode(dock_top, ImGuiDir_Left, 0.3f, &dock_left, &dock_center);

        // Global first so it is the front (active) tab on startup
        ImGui::DockBuilderDockWindow("Global", dock_left);
        ImGui::DockBuilderDockWindow("Models", dock_left);
        ImGui::DockBuilderDockWindow("Debug",  dock_bottom);
        ImGui::DockBuilderFinish(dockspace_id);
        layoutInitialized = true;
    }

    // -- Global panel ----------------------------------------------------------

    if (ImGui::Begin("Global")) {
        ImGui::Text("Resolution: %dx%d", m_windowSize.x, m_windowSize.y);
        ImGui::Checkbox("Show axes", &m_showAxes);

        ImGui::Separator();
        ImGui::BeginDisabled(!m_debugState.config.showDebug);
        if (ImGui::Button("Export debug log"))
            ExportDebugLog();
        ImGui::EndDisabled();
        if (!m_debugState.config.showDebug)
            ImGui::TextDisabled("Enable debug to export");

        // -- Light direction ---------------------------------------------------
        ImGui::Separator();
        ImGui::SeparatorText("Lighting");

        if (ImGui::DragFloat3("Light Dir", glm::value_ptr(m_lightDir), 0.01f)) {
            for (const auto& obj : m_sceneManager.GetSceneObjects()) {
                auto rm = std::dynamic_pointer_cast<RayMarchedModel>(obj);
                if (rm) rm->SetLightDir(m_lightDir);
            }
        }

        // -- Skybox -----------------------------------------------------------
        ImGui::Separator();
        ImGui::SeparatorText("Skybox");
        ImGui::TextUnformatted("Folder (px/nx/py/ny/pz/nz.png):");
        ImGui::SetNextItemWidth(-1.f);
        ImGui::InputText("##skyboxpath", m_skyboxPathBuf, sizeof(m_skyboxPathBuf));
        if (ImGui::Button("Apply Skybox")) {
            namespace fs = std::filesystem;
            std::string folder(m_skyboxPathBuf);
            try {
                auto cubemap = m_textureManager.GetOrLoadCubemap(
                    std::array<fs::path, 6>{
                        fs::path(folder) / "px.png", fs::path(folder) / "nx.png",
                        fs::path(folder) / "py.png", fs::path(folder) / "ny.png",
                        fs::path(folder) / "pz.png", fs::path(folder) / "nz.png"
                    }, false);
                m_skyboxRenderer = std::make_unique<SkyboxRenderer>(
                    m_shaderManager.Get("skybox"), cubemap);
            } catch (...) {}
        }

    }
    ImGui::End();

    // -- Models panel (was "Scene") --------------------------------------------

    if (ImGui::Begin("Models")) {
        const float listWidth = 160.f;
        const auto& models = m_sceneManager.GetSceneObjects();

        ImGui::BeginChild("ModelList", ImVec2(listWidth, 0.f), true);

        // Model list
        for (int i = 0; i < (int)models.size(); ++i) {
            ImGui::PushID(i);
            bool selected = (i == m_selectedIndex);
            if (ImGui::Selectable(models[i]->GetName().c_str(), selected)) {
                m_selectedIndex = i;
                m_commandQueue->Push(std::make_unique<SetSelectedCommand>(m_sceneManager, models[i]));
            }
            ImGui::PopID();
        }

        ImGui::Separator();

        if (ImGui::Button("Deselect", ImVec2(-1, 0))) {
            m_selectedIndex = -1;
            m_commandQueue->Push(std::make_unique<SetSelectedCommand>(m_sceneManager, nullptr));
        }

        ImGui::Spacing();

        // Add buttons
        if (ImGui::Button("Add RayMarched", ImVec2(-1, 0))) {
            int newIdx = static_cast<int>(models.size()) + 1;
            m_commandQueue->Push(std::make_unique<CreateRMObjectCommand>(
                m_sceneManager,
                CreateDefaultRayMarchedModel("Surface " + std::to_string(newIdx))));
            m_objLoadFailed = false;
        }

        ImGui::Spacing();
        ImGui::TextUnformatted(".obj path:");
        ImGui::SetNextItemWidth(-1.f);
        ImGui::InputText("##objpath", m_objPathBuf, sizeof(m_objPathBuf));
        if (ImGui::Button("Add .obj Model", ImVec2(-1, 0))) {
            m_objLoadFailed = false;
            auto newModel = CreateModelFromOBJ(std::string(m_objPathBuf));
            if (newModel)
                m_commandQueue->Push(std::make_unique<CreateObjObjectCommand>(
                    m_sceneManager, std::move(newModel)));
            else
                m_objLoadFailed = true;
        }
        if (m_objLoadFailed)
            ImGui::TextColored(ImVec4(1.f, 0.35f, 0.35f, 1.f), "Failed to load");

        // Delete button — only shown when a model is selected
        if (m_selectedIndex >= 0 && m_selectedIndex < (int)models.size()) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.1f, 0.1f, 1.0f));
            if (ImGui::Button("Delete Selected", ImVec2(-1, 0))) {
                m_commandQueue->Push(std::make_unique<DeleteObjectCommand>(
                    m_sceneManager, models[m_selectedIndex]));
                m_commandQueue->Push(std::make_unique<SetSelectedCommand>(m_sceneManager, nullptr));
                m_selectedIndex = -1;
            }
            ImGui::PopStyleColor();
        }

        ImGui::EndChild();

        ImGui::SameLine();

        // -- Selected model controls -------------------------------------------
        ImGui::BeginChild("ModelControls", ImVec2(0.f, 0.f), false);
        if (!models.empty() && m_selectedIndex >= 0 && m_selectedIndex < (int)models.size()) {
            auto& obj = models[m_selectedIndex];
            obj->AcceptGUIVisitor(m_imguiVisitor);

            auto rm = std::dynamic_pointer_cast<RayMarchedModel>(obj);
            if (rm) {
                ImGui::Separator();

                std::string techName = rm->GetTechnique()
                    ? rm->GetTechnique()->GetName()
                    : "none";
                if (ImGui::BeginCombo("Technique", techName.c_str())) {
                    for (auto& [key, tech] : m_techniques) {
                        bool sel = (rm->GetTechnique() == tech);
                        if (ImGui::Selectable(tech->GetName().c_str(), sel))
                            m_commandQueue->Push(std::make_unique<SetTechniqueCommand>(rm, tech));
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::BeginCombo("Heightmap", m_heightMaps[m_activeHeightmapIdx].c_str())) {
                    for (int i = 0; i < (int)m_heightMaps.size(); ++i) {
                        if (ImGui::Selectable(m_heightMaps[i].c_str(), m_activeHeightmapIdx == i)) {
                            m_activeHeightmapIdx = i;
                            m_commandQueue->Push(std::make_unique<SetHeightmapCommand>(
                                rm, m_textureManager, m_heightMaps[i], m_conemapGenerator.get()
                            ));
                        }
                    }
                    ImGui::EndCombo();
                }

                // -- Conemap section -------------------------------------------
                if (ImGui::TreeNode("Conemap")) {
                    // Interpolation (switches shader permutation instantly)
                    bool interpChanged = false;
                    if (ImGui::Checkbox("Interpolate height", &m_interpHeight)) interpChanged = true;
                    if (ImGui::Checkbox("Interpolate cone",   &m_interpCone))   interpChanged = true;
                    if (interpChanged)
                        UpdateTechniquePrograms();

                    // Generation mode
                    ImGui::Spacing();
                    ImGui::TextUnformatted("Generation:");
                    int conservativeMode = m_conservative ? 1 : 0;
                    ImGui::RadioButton("Original",     &conservativeMode, 0);
                    ImGui::SameLine();
                    ImGui::RadioButton("Conservative", &conservativeMode, 1);
                    m_conservative = (conservativeMode != 0);

                    if (ImGui::Button("Regenerate")) {
                        m_conemapGenerator->SetConservative(m_conservative);
                        m_commandQueue->Push(std::make_unique<SetHeightmapCommand>(
                            rm, m_textureManager, m_heightMaps[m_activeHeightmapIdx],
                            m_conemapGenerator.get()));
                    }

                    // Conemap preview
                    GLuint previewID = m_conemapGenerator->GetLastConemapID();
                    if (previewID != 0) {
                        ImGui::Spacing();
                        ImGui::TextUnformatted("Preview (r=height, g=cone tan):");
                        float w = ImGui::GetContentRegionAvail().x;
                        ImGui::Image(static_cast<ImTextureID>(previewID), ImVec2(w, w));
                    }

                    ImGui::TreePop();
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();

    // -- Debug panel -----------------------------------------------------------
    RenderDebugPanel();
}

// -- RenderDebugPanel ----------------------------------------------------------

void MyApp::RenderDebugPanel()
{
    if (!ImGui::Begin("Debug")) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("DebugTabs")) {

        // -- Settings tab ------------------------------------------------------
        if (ImGui::BeginTabItem("Settings")) {
            auto& cfg = m_debugState.config;
            auto& cam = m_debugState.debugCamera;

            ImGui::SeparatorText("Toggles");
            ImGui::Checkbox("Enable debug",    &cfg.showDebug);
            if (cfg.showDebug) {
                ImGui::Checkbox("Show steps",      &cfg.showSteps);
                ImGui::Checkbox("Show enter/exit", &cfg.showEnterExit);
                ImGui::Checkbox("Show cones",      &cfg.showCones);
                ImGui::Checkbox("Show ray",        &cfg.showRay);
                ImGui::Checkbox("Show hit point",  &cfg.showHitPoint);

                ImGui::SeparatorText("Primitive");
                ImGui::DragInt("Primitive ID", &cfg.primitiveID, 1.0f, -1, 65535,
                    cfg.primitiveID < 0 ? "disabled (all)" : "%d");

                ImGui::SeparatorText("Debug camera");
                if (ImGui::Button("Sync to main camera"))
                    cam.SetView(m_camera.GetEye(), m_camera.GetAt(), glm::vec3(0.f, 1.f, 0.f));
                glm::vec3 eye = cam.GetEye();
                glm::vec3 at  = cam.GetAt();
                if (ImGui::DragFloat3("Eye##dbg", glm::value_ptr(eye), 0.1f))
                    cam.SetView(eye, at, glm::vec3(0.f, 1.f, 0.f));
                if (ImGui::DragFloat3("At##dbg",  glm::value_ptr(at),  0.1f))
                    cam.SetView(eye, at, glm::vec3(0.f, 1.f, 0.f));
            }

            ImGui::EndTabItem();
        }

        // -- Values tab --------------------------------------------------------
        if (ImGui::BeginTabItem("Values")) {
            if (!m_debugState.config.showDebug) {
                ImGui::TextDisabled("Enable debug in Settings to record values.");
            } else {
                // The SSBO starts with 2 x uvec4 (32 bytes) for indirect commands,
                // so the debugNumerical[] flexible array byte offset = 32.
                const GLintptr kNumericalOffset = 2 * static_cast<GLintptr>(sizeof(glm::uvec4));

                glm::vec4 hdr[23];
                glGetNamedBufferSubData(m_debugState.debugNumericalSSBO,
                    kNumericalOffset, sizeof(hdr), hdr);

                int  stepCount = static_cast<int>(hdr[0].x + 0.5f);
                int  flags     = static_cast<int>(hdr[0].y + 0.5f);
                bool wasHit    = hdr[16].w > 0.5f;

                // -- Summary ---------------------------------------------------
                ImGui::Text("Steps: %d", stepCount);
                ImGui::SameLine(0.f, 24.f);
                if (wasHit)
                    ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f),
                        "Hit   UV: (%.4f, %.4f)", hdr[16].x, hdr[16].y);
                else
                    ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "No hit");

                if (flags) {
                    ImGui::SameLine(0.f, 24.f);
                    ImGui::TextColored(ImVec4(1.f, 0.6f, 0.f, 1.f), "Flags:%s%s%s",
                        (flags & 1) ? " max_steps"    : "",
                        (flags & 2) ? " exited_prism" : "",
                        (flags & 4) ? " converged"    : "");
                }

                // -- Ray in texture space --------------------------------------
                if (ImGui::CollapsingHeader("Ray (texture space)")) {
                    ImGui::Text("Eye:   (%.4f, %.4f, %.4f)", hdr[7].x,  hdr[7].y,  hdr[7].z);
                    ImGui::Text("Enter: (%.4f, %.4f, %.4f)", hdr[13].x, hdr[13].y, hdr[13].z);
                    ImGui::Text("Exit:  (%.4f, %.4f, %.4f)", hdr[14].x, hdr[14].y, hdr[14].z);
                    ImGui::Text("Dir:   (%.4f, %.4f, %.4f)", hdr[15].x, hdr[15].y, hdr[15].z);
                }

                // -- Matrix display helper -------------------------------------
                auto showMatrix4x4 = [](const char* label,
                                        const glm::vec4& c0, const glm::vec4& c1,
                                        const glm::vec4& c2, const glm::vec4& c3)
                {
                    if (ImGui::CollapsingHeader(label)) {
                        ImGui::Text("  %9.4f  %9.4f  %9.4f  %9.4f", c0.x, c1.x, c2.x, c3.x);
                        ImGui::Text("  %9.4f  %9.4f  %9.4f  %9.4f", c0.y, c1.y, c2.y, c3.y);
                        ImGui::Text("  %9.4f  %9.4f  %9.4f  %9.4f", c0.z, c1.z, c2.z, c3.z);
                        ImGui::Text("  %9.4f  %9.4f  %9.4f  %9.4f", c0.w, c1.w, c2.w, c3.w);
                    }
                };

                showMatrix4x4("M  (scene --> texture space)",
                    hdr[3], hdr[4], hdr[5], hdr[6]);
                showMatrix4x4("T  (scene --> unit prism space)",
                    hdr[8], hdr[9], hdr[10], hdr[11]);

                // -- Primitive vertices ----------------------------------------
                if (ImGui::CollapsingHeader("Primitive vertices (scene space)")) {
                    ImGui::Text("v0: (%.4f, %.4f, %.4f)  UV: (%.4f, %.4f)",
                        hdr[17].x, hdr[17].y, hdr[17].z, hdr[20].x, hdr[20].y);
                    ImGui::Text("v1: (%.4f, %.4f, %.4f)  UV: (%.4f, %.4f)",
                        hdr[18].x, hdr[18].y, hdr[18].z, hdr[21].x, hdr[21].y);
                    ImGui::Text("v2: (%.4f, %.4f, %.4f)  UV: (%.4f, %.4f)",
                        hdr[19].x, hdr[19].y, hdr[19].z, hdr[22].x, hdr[22].y);
                }

                // -- Steps table -----------------------------------------------
                if (stepCount > 0) {
                    char stepsLabel[32];
                    snprintf(stepsLabel, sizeof(stepsLabel), "Steps (%d)###Steps", stepCount);

                    if (ImGui::CollapsingHeader(stepsLabel, ImGuiTreeNodeFlags_DefaultOpen)) {
                        int readCount = stepCount < 512 ? stepCount : 512;
                        std::vector<glm::vec4> stepData(readCount * 2);
                        glGetNamedBufferSubData(m_debugState.debugNumericalSSBO,
                            kNumericalOffset + 23 * static_cast<GLintptr>(sizeof(glm::vec4)),
                            readCount * 2 * sizeof(glm::vec4),
                            stepData.data());

                        constexpr ImGuiTableFlags kTableFlags =
                            ImGuiTableFlags_ScrollY        |
                            ImGuiTableFlags_BordersOuter   |
                            ImGuiTableFlags_BordersInnerV  |
                            ImGuiTableFlags_RowBg          |
                            ImGuiTableFlags_SizingFixedFit;

                        if (ImGui::BeginTable("StepsTable", 6, kTableFlags, ImVec2(0.f, 150.f))) {
                            ImGui::TableSetupScrollFreeze(0, 1);
                            ImGui::TableSetupColumn("#",                    ImGuiTableColumnFlags_WidthFixed, 28.f);
                            ImGui::TableSetupColumn("ti");
                            ImGui::TableSetupColumn("t");
                            ImGui::TableSetupColumn("height");
                            ImGui::TableSetupColumn("tan");
                            ImGui::TableSetupColumn("ui (texture space)");
                            ImGui::TableHeadersRow();

                            for (int i = 0; i < readCount; ++i) {
                                const glm::vec4& num = stepData[i * 2];
                                const glm::vec4& pos = stepData[i * 2 + 1];
                                ImGui::TableNextRow();
                                ImGui::TableNextColumn(); ImGui::Text("%d",   i);
                                ImGui::TableNextColumn(); ImGui::Text("%.5f", num.x);
                                ImGui::TableNextColumn(); ImGui::Text("%.5f", num.y);
                                ImGui::TableNextColumn(); ImGui::Text("%.4f", num.z);
                                ImGui::TableNextColumn(); ImGui::Text("%.4f", num.w);
                                ImGui::TableNextColumn(); ImGui::Text("(%.4f, %.4f, %.4f)",
                                    pos.x, pos.y, pos.z);
                            }
                            ImGui::EndTable();
                        }
                    }
                }
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}
