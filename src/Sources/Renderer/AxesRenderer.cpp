#include "Headers/Renderer/AxesRenderer.h"
#include "Utils/ICamera.h"
#include "Utils/GLUtils.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

AxesRenderer::AxesRenderer(GLuint programID) : m_programID(programID) {}

AxesRenderer::~AxesRenderer() = default; // ShaderManager owns the program (DeleteAll())

void AxesRenderer::Render(const ICamera& cam)
{
    glUseProgram(m_programID);

    glm::mat4 world = glm::translate(glm::identity<glm::mat4>(), cam.GetAt());
    glUniformMatrix4fv(ul(m_programID, "transformData.world"), 1, GL_FALSE, glm::value_ptr(world));
    glUniformMatrix4fv(ul(m_programID, "cameraData.viewProj"), 1, GL_FALSE, glm::value_ptr(cam.GetViewProj()));

    // Always visible, regardless of whether there is an object in front of it
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_LINES, 0, 6);
    glEnable(GL_DEPTH_TEST);
}
