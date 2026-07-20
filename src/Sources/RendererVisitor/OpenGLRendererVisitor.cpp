#include "Headers/RendererVisitor/OpenGLRendererVisitor.h"
#include "Headers/Debug/RayMarchDebugState.h"
#include "Headers/Model/ModelBase.h"
#include "Headers/Model/Model.h"
#include "Headers/Model/RayMarchedModel.h"
#include "Headers/Model/Mesh.h"
#include "Utils/ICamera.h"

#include <glm/gtc/type_ptr.hpp>

OpenGLRendererVisitor::OpenGLRendererVisitor(const ICamera& cam)
    : m_camera(cam)
{
}

// -- Shared setup: camera + world transform ------------------------------------

void OpenGLRendererVisitor::VisitModelBase(const ModelBase& target)
{
    GLuint prog = target.GetProgramID();

    glm::vec3 eye      = m_camera.GetEye();
    glm::mat4 viewProj = m_camera.GetViewProj();
    glUniform3fv      (glGetUniformLocation(prog, "cameraData.eye"),      1,           glm::value_ptr(eye));
    glUniformMatrix4fv(glGetUniformLocation(prog, "cameraData.viewProj"), 1, GL_FALSE, glm::value_ptr(viewProj));

    glm::mat4 world = target.GetTransform().GetWorldMatrix();
    glUniformMatrix4fv(glGetUniformLocation(prog, "transformData.world"), 1, GL_FALSE, glm::value_ptr(world));
}

// -- Visit(Model) -------------------------------------------------------------

void OpenGLRendererVisitor::Visit(const Model& target)
{
    if (!target.IsShown()) return;

    GLuint prog = target.GetProgramID();
    if (!prog) return;

    GLboolean prevCullFace = glIsEnabled(GL_CULL_FACE);

    GLint prevPolygonMode[2] = { GL_FILL, GL_FILL };
    glGetIntegerv(GL_POLYGON_MODE, prevPolygonMode);

    if (target.IsWireframe()) {
        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glEnable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glUseProgram(prog);
    VisitModelBase(target);

    for (const auto& mesh : target.GetMeshes()) {
        if (mesh) mesh->Render();
    }

    glUseProgram(0);

    prevCullFace ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, prevPolygonMode[0]);

    // -- Selection highlight: wireframe overlay --
    GLuint selProg = target.GetSelectedProgramID();
    if (static_cast<const ISceneObject*>(&target) == m_selected && selProg != 0) {
        GLfloat prevLineWidth;
        glGetFloatv(GL_LINE_WIDTH, &prevLineWidth);

        GLint selPolygonMode[2] = { GL_FILL, GL_FILL };
        glGetIntegerv(GL_POLYGON_MODE, selPolygonMode);

        GLboolean selCullFace = glIsEnabled(GL_CULL_FACE);

        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(3.0f);

        glUseProgram(selProg);

        glm::mat4 world    = target.GetTransform().GetWorldMatrix();
        glm::mat4 viewProj = m_camera.GetViewProj();
        glUniformMatrix4fv(glGetUniformLocation(selProg, "transformData.world"),    1, GL_FALSE, glm::value_ptr(world));
        glUniformMatrix4fv(glGetUniformLocation(selProg, "cameraData.viewProj"),    1, GL_FALSE, glm::value_ptr(viewProj));
        glUniform3f       (glGetUniformLocation(selProg, "colorData.color"),   1.0f, 0.6f, 0.0f);

        for (const auto& mesh : target.GetMeshes()) {
            if (!mesh) continue;
            glBindVertexArray(mesh->GetVAO());
            glDrawElements(mesh->GetDrawMode(), mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
        }
        glBindVertexArray(0);
        glUseProgram(0);

        selCullFace ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, selPolygonMode[0]);
        glLineWidth(prevLineWidth);
    }
}

// -- Visit(RayMarchedModel) ---------------------------------------------------

void OpenGLRendererVisitor::Visit(const RayMarchedModel& target)
{
    if (!target.IsShown()) return;

    auto technique = target.GetTechnique();
    if (!technique) return;

    GLuint prog = technique->GetProgramID();
    if (!prog) return;

    GLboolean prevCullFace = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    // -- Bind debug SSBOs and write CPU config before draw --------------------
    if (m_debugState && m_debugState->config.showDebug) {
        const auto& cfg = m_debugState->config;
        const auto& cam = m_debugState->debugCamera;

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_debugState->debugVisualSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_debugState->debugNumericalSSBO);

        // Config slots [5] and [6] of debugVisualSSBO (unchanged layout)
        glm::vec4 slots[4];
        slots[0] = glm::vec4(
            cfg.showDebug     ? 1.0f : 0.0f,
            cfg.showSteps     ? 1.0f : 0.0f,
            cfg.showEnterExit ? 1.0f : 0.0f,
            cfg.showCones     ? 1.0f : 0.0f
        );
        slots[1] = glm::vec4(
            cfg.showRay      ? 1.0f : 0.0f,
            cfg.showHitPoint ? 1.0f : 0.0f,
            static_cast<float>(cfg.primitiveID),
            static_cast<float>(technique->GetTechniqueID())
        );
        glm::vec3 eye = cam.GetEye();
        glm::vec3 at  = cam.GetAt();
        slots[2] = glm::vec4(eye, 1.0f);
        slots[3] = glm::vec4(at,  1.0f);
        glNamedBufferSubData(m_debugState->debugVisualSSBO,
            5 * sizeof(glm::vec4), 4 * sizeof(glm::vec4), slots);

        // Camera eye/at --> debugNumerical[1] and [2]
        // The SSBO starts with 2 x uvec4 (32 bytes) for the indirect commands,
        // so debugNumerical[1] byte offset = 32 + 1x16 = 48.
        glm::vec4 camSlots[2] = {
            glm::vec4(eye, 1.0f),
            glm::vec4(at,  0.0f)
        };
        glNamedBufferSubData(m_debugState->debugNumericalSSBO,
            2 * sizeof(glm::uvec4) + 1 * sizeof(glm::vec4),   // offset 48
            2 * sizeof(glm::vec4), camSlots);
    }

    glUseProgram(prog);
    VisitModelBase(target);
    technique->SetUniforms(target);

    for (const auto& mesh : target.GetMeshes()) {
        if (mesh) mesh->Render();
    }

    glUseProgram(0);

    prevCullFace ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);

    // -- Selection highlight: mesh wireframe overlay --
    GLuint selProg = target.GetSelectedProgramID();
    if (static_cast<const ISceneObject*>(&target) == m_selected && selProg != 0) {
        GLfloat prevLineWidth;
        glGetFloatv(GL_LINE_WIDTH, &prevLineWidth);

        GLint selPolygonMode[2] = { GL_FILL, GL_FILL };
        glGetIntegerv(GL_POLYGON_MODE, selPolygonMode);

        GLboolean selCullFace = glIsEnabled(GL_CULL_FACE);

        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(3.0f);

        glUseProgram(selProg);

        glm::mat4 world    = target.GetTransform().GetWorldMatrix();
        glm::mat4 viewProj = m_camera.GetViewProj();
        glUniformMatrix4fv(glGetUniformLocation(selProg, "transformData.world"), 1, GL_FALSE, glm::value_ptr(world));
        glUniformMatrix4fv(glGetUniformLocation(selProg, "cameraData.viewProj"), 1, GL_FALSE, glm::value_ptr(viewProj));
        glUniform3f       (glGetUniformLocation(selProg, "colorData.color"),  1.0f, 0.6f, 0.0f);

        for (const auto& mesh : target.GetMeshes()) {
            if (!mesh) continue;
            glBindVertexArray(mesh->GetVAO());
            glDrawElements(mesh->GetDrawMode(), mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
        }
        glBindVertexArray(0);
        glUseProgram(0);

        selCullFace ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, selPolygonMode[0]);
        glLineWidth(prevLineWidth);
    }
}
