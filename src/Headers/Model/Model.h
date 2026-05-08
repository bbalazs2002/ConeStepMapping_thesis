#pragma once

#include <memory>

#include "ModelBase.h"
#include "../Types.h"
#include "../Material/Material.h"
#include "../Model/Mesh.h"
#include "../../Utils/GLUtils.hpp"

class Model : public ModelBase {
protected:
	std::vector<std::shared_ptr<Material>> m_materials;
	std::vector<std::unique_ptr<Mesh>> m_meshes;
	bool m_wireframe = false;
	std::string m_objPath;

public:
	char m_objPathBuffer[256] = "";

	Model(const ModelParams& params);
	~Model();

	// IDrawable methods
	virtual void Render(const RenderParams& p) override;

	inline void AddMaterial(std::shared_ptr<Material> material) {
		m_materials.push_back(std::move(material));
	}
	inline std::shared_ptr<Material> GetMaterial(int id) const {
		return m_materials[id];
	}
	inline void SetMaterial(std::shared_ptr<Material> material) {
		m_materials.clear();
		m_materials.push_back(material);

		for (auto& m : m_meshes) {
			m->SetMaterial(material);
		}
	}

	inline void AddMesh(std::unique_ptr<Mesh> mesh) {
		m_meshes.push_back(std::move(mesh));
	}
	inline void SetWireFrame(bool wireframe) {
		m_wireframe = wireframe;
	}
	inline bool GetWireFrame() const {
		return m_wireframe;
	}

	inline void SetObjPath(const char* path) {
		strcpy_s(m_objPathBuffer, path);
		SetObjPath();
	}
	virtual void SetObjPath();

	inline void CleanGeometry() {
		m_meshes.clear();
		m_materials.clear();
	}

	inline void CleanMaterials() {
		m_materials.clear();
	}
};