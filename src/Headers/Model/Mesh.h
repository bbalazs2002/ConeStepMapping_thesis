#pragma once

#include <memory>

#include "Headers/Types.h"
#include "Headers/Material/Material.h"
#include "Utils/GLUtils.hpp"

class Mesh {
public:
    ~Mesh() {
        CleanOGLObject(m_GPU);
    }

    inline std::shared_ptr<Material> GetMaterial() const { return m_material; }
    inline void SetMaterial(std::shared_ptr<Material> mat) { m_material = std::move(mat); }

    inline int  GetDrawMode()      const { return m_drawMode; }
    inline void SetDrawMode(int mode)    { m_drawMode = mode; }

    inline GLuint   GetVAO()         const { return m_GPU.vaoID; }
    inline GLsizei  GetVertexCount() const { return m_GPU.count; }

    template <typename VertexT = Vertex>
    void Build(MeshObject<VertexT>&& mesh) {
        m_GPU = CreateGLObjectFromMesh(mesh, VertexT::GetLayout());
    }

    void Render();

private:
    std::shared_ptr<Material> m_material;
    int                       m_drawMode = GL_TRIANGLES;
    OGLObject                 m_GPU;
};
