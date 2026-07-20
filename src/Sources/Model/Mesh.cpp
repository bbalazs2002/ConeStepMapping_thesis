#include "Headers/Model/Mesh.h"
#include "Headers/Material/Material.h"
#include "Utils/Log.h"

#include <GL/glew.h>

void Mesh::Render()
{
    if (!m_material) {
        LOG_ERROR("Mesh::Render: null material — minden Model mesh-nek kell material");
        exit(1);
    }

    GLint prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);

    glBindVertexArray(m_GPU.vaoID);
    Material::UploadMaterialToShader(static_cast<GLuint>(prog), m_material);
    glDrawElements(m_drawMode, m_GPU.count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    Material::ClearMaterialFromShader();
}
