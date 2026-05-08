#pragma once

#include <memory>

#include "../Types.h"
#include "../../Utils/GLUtils.hpp"

class Mesh {
private:
	std::shared_ptr<Material> m_material;
	OGLObject m_GPU;

public:
	~Mesh() {
		CleanOGLObject(m_GPU);
	}

	inline void SetMaterial(std::shared_ptr<Material> material) {
		m_material = std::move(material);
	}
	inline std::shared_ptr<Material> GetMaterial() const {
		return m_material;
	}
	inline GLuint GetVAO() const {
		return m_GPU.vaoID;
	}
	inline GLsizei GetVertexCount() const {
		return m_GPU.count;
	}

	template <typename VertexT = Vertex>
	void Build(MeshObject<VertexT>&& mesh) {
		m_GPU = CreateGLObjectFromMesh(mesh, VertexT::GetLayout());
	}

	void Render(const MeshRenderParams& p);
};