#include <memory>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

// GLEW
#include <GL/glew.h>

#include "../../Headers/Types.h"
#include "../../Headers/config.h"
#include "../../Headers/Model/ModelBase.h"
#include "../../Headers/Model/Model.h"
#include "../../Headers/Model/Mesh.h"

Model::Model(const ModelParams& params) : ModelBase(MODEL2MODELBASE) {
	m_wireframe = params.wireFrame;
}
Model::~Model() {
	CleanGeometry();
	CleanMaterials();
}

// Drawable methods
void Model::Render(const RenderParams& p) {
	if (!GetShow()) {
		return;
	}

	glm::mat4 modelTransform = GetApplyTransforms() ?
		GetTransform() :
		glm::identity<glm::mat4>();

	MeshRenderParams mp{
		GetProgramID(),
		GetDrawMode()
	};

	// -- Set render options --
	bool cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
	GLfloat defLineWidth;
	glGetFloatv(GL_LINE_WIDTH, &defLineWidth);
	GLint polygonMode[2];
	glGetIntegerv(GL_POLYGON_MODE, polygonMode);
	if (GetWireFrame()) {
		glDisable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// -- Activate shader --
	GLuint progID = GetProgramID();
	glUseProgram(progID);

	// -- Set shader input data --
	// Camera module
	glUniform3fv(ul(progID, "cameraData.eye"), 1, glm::value_ptr(p.cameraPos));
	glUniformMatrix4fv(ul(progID, "cameraData.viewProj"), 1, GL_FALSE, glm::value_ptr(p.viewProj));

	// Transform module
	glUniformMatrix4fv(ul(progID, "transformData.world"), 1, GL_FALSE, glm::value_ptr(modelTransform));

	// Render all meshes
	for (const auto& mesh : m_meshes) {
		mesh->Render(mp);
	}

	// -- Restore initial OGL state --
	if (cullFaceEnabled) glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT, polygonMode[0]);
	glPolygonMode(GL_BACK, polygonMode[1]);
	glLineWidth(defLineWidth);
	glUseProgram(0);
}

void Model::SetObjPath() {
	std::string tempPath = m_objPathBuffer;
	if (tempPath == m_objPath) {
		return;
	}

	// clean old geometry
	CleanGeometry();
	CleanMaterials();

	// load the new file
	ModelLoaderData modelData = ModelLoader::LoadFromOBJ(tempPath);
	if (modelData.matMesh.size() <= 0) {
		return;
	}
	m_objPath = tempPath;

	// Build materials
	m_materials = std::move(Material::convertFromTinyObjLoader(modelData.materials, ModelLoader::ResolveMTLSearchPath(tempPath, "./")));

	// Build meshes
	for (auto& mm : modelData.matMesh) {

		// Create a unique_ptr for the mesh - Mesh will be owned by the Model/Return object
		auto mesh = std::make_unique<Mesh>();

		// Assign material using shared_ptr from the temporary storage
		// Note: We access modelData.materials BEFORE moving it to retVal
		if (mm.materialID >= 0 && mm.materialID < static_cast<int>(modelData.materials.size())) {
			mesh->SetMaterial(m_materials[mm.materialID]);
		}

		mesh->Build(std::move(mm.mesh));
		m_meshes.push_back(std::move(mesh));
	}
}