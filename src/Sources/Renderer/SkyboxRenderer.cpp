#include "Headers/Renderer/SkyboxRenderer.h"
#include "Headers/Texture/Texture.h"
#include "Utils/ICamera.h"
#include "Utils/GLUtils.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

SkyboxRenderer::SkyboxRenderer(GLuint programID, std::shared_ptr<Texture> cubemap)
    : m_programID(programID), m_cubemap(std::move(cubemap))
{
    // --- Geometry: unit cube, position-only ---
    MeshObject<glm::vec3> skyboxCPU = {
        std::vector<glm::vec3>{
            // back
            glm::vec3(-1, -1, -1),
            glm::vec3( 1, -1, -1),
            glm::vec3( 1,  1, -1),
            glm::vec3(-1,  1, -1),
            // front
            glm::vec3(-1, -1, 1),
            glm::vec3( 1, -1, 1),
            glm::vec3( 1,  1, 1),
            glm::vec3(-1,  1, 1),
        },
        std::vector<GLuint>{
            // back
            0, 1, 2,
            2, 3, 0,
            // front
            4, 6, 5,
            6, 4, 7,
            // left
            0, 3, 4,
            4, 3, 7,
            // right
            1, 5, 2,
            5, 6, 2,
            // bottom
            1, 0, 4,
            1, 4, 5,
            // top
            3, 2, 6,
            3, 6, 7,
        }
    };
    m_gpuObject = CreateGLObjectFromMesh(skyboxCPU, { { 0, offsetof(glm::vec3, x), 3, GL_FLOAT } });
}

SkyboxRenderer::~SkyboxRenderer()
{
    CleanOGLObject(m_gpuObject);
}

void SkyboxRenderer::Render(const ICamera& cam)
{
    glUseProgram(m_programID);

    glm::mat4 world = glm::translate(glm::identity<glm::mat4>(), cam.GetEye());
    glUniformMatrix4fv(ul(m_programID, "transformData.world"), 1, GL_FALSE, glm::value_ptr(world));
    glUniformMatrix4fv(ul(m_programID, "cameraData.viewProj"), 1, GL_FALSE, glm::value_ptr(cam.GetViewProj()));
    glUniform1i(ul(m_programID, "skyboxTexture"), 1);

    // Everything is pushed to the far clipping plane, so use less-than-or-equal
    GLint prevDepthFunc;
    glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
    glDepthFunc(GL_LEQUAL);

    glBindTextureUnit(1, m_cubemap->GetID());
    glBindVertexArray(m_gpuObject.vaoID);
    glDrawElements(GL_TRIANGLES, m_gpuObject.count, GL_UNSIGNED_INT, nullptr);

    glDepthFunc(static_cast<GLenum>(prevDepthFunc));
}
