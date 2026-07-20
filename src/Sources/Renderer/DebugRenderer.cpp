#include "Headers/Renderer/DebugRenderer.h"
#include "Headers/Debug/RayMarchDebugState.h"
#include "Utils/ICamera.h"

#include <glm/gtc/type_ptr.hpp>

DebugRenderer::DebugRenderer(GLuint progRay, GLuint progEnterExit,
                             GLuint progSteps, GLuint progCones, GLuint progHit)
    : m_progRay      (progRay)
    , m_progEnterExit(progEnterExit)
    , m_progSteps    (progSteps)
    , m_progCones    (progCones)
    , m_progHit      (progHit)
{
}

DebugRenderer::~DebugRenderer() = default;

void DebugRenderer::Render(const RayMarchDebugState& state, const ICamera& camera)
{
    const auto& cfg = state.config;

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, state.debugVisualSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, state.debugNumericalSSBO);

    // Save and configure GL state
    GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLfloat   prevPointSize;
    glGetFloatv(GL_POINT_SIZE, &prevPointSize);

    glDisable(GL_DEPTH_TEST);

    glm::mat4 vp = camera.GetViewProj();

    // Helper lambda: switch program + upload viewProj + draw
    const auto draw = [&](GLuint prog, GLenum mode, GLsizei count) {
        glUseProgram(prog);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_viewProj"),
            1, GL_FALSE, glm::value_ptr(vp));
        glDrawArrays(mode, 0, count);
    };

    // -- Fixed vertex-count draw calls ----------------------------------------

    if (cfg.showRay)
        draw(m_progRay, GL_LINES, 4);

    if (cfg.showEnterExit)
        draw(m_progEnterExit, GL_LINES, 16);

    if (cfg.showHitPoint)
        draw(m_progHit, GL_LINES, 8);

    // -- Indirect draw calls (variable vertex count, GS writes the command buffer) -

    if (cfg.showSteps || cfg.showCones) {
        // The indirect commands occupy the start of debugNumericalSSBO:
        //   offset  0: indirectSteps = {stepCount,   1, 0, 0}
        //   offset 16: indirectCones = {stepCount*4, 1, 0, 0}
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, state.debugNumericalSSBO);

        if (cfg.showSteps) {
            glUseProgram(m_progSteps);
            glUniformMatrix4fv(glGetUniformLocation(m_progSteps, "u_viewProj"),
                1, GL_FALSE, glm::value_ptr(vp));
            glPointSize(8.0f);
            glDrawArraysIndirect(GL_POINTS,
                reinterpret_cast<const void*>(0));                   // indirectSteps @ offset 0
        }

        if (cfg.showCones) {
            glUseProgram(m_progCones);
            glUniformMatrix4fv(glGetUniformLocation(m_progCones, "u_viewProj"),
                1, GL_FALSE, glm::value_ptr(vp));
            glDrawArraysIndirect(GL_LINE_STRIP,
                reinterpret_cast<const void*>(sizeof(GLuint) * 4));  // indirectCones @ offset 16
        }

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    }

    // -- Restore GL state -----------------------------------------------------

    glPointSize(prevPointSize);
    prevDepthTest ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
    glUseProgram(0);
}
