// GLM
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

// GLEW
#include <GL/glew.h>

#include "../../Headers/Model/RayMarchedSurface.h"
#include "../../Headers/Model/Model.h"
#include "../../Headers/Types.h"
#include "../../Headers/config.h"

RayMarchedSurface::RayMarchedSurface(const RayMarchedSurfaceParams& params) : Model(RAYMARCHEDSURFACE2MODEL) {
	glGenQueries(1, &m_timeQueryID);

	m_programConemapID = params.shaderPrograms.conemapID;

	InitSSBOs();
}

void RayMarchedSurface::RenderConeMap() {

	if (m_programConemapID <= 0) {
		LOG_ERROR("[RayMarchedSurface] Conemap generator compute shader is not found.");
		return;
	}
	glUseProgram(m_programConemapID);

	glDeleteTextures(1, &m_conemapTextureID);
	glCreateTextures(GL_TEXTURE_2D, 1, &m_conemapTextureID);
	glTextureStorage2D(m_conemapTextureID, 1, GL_RG8, m_HMres.x, m_HMres.y);
	glBindTexture(GL_TEXTURE_2D, m_conemapTextureID);
	if (m_interpolateTexture) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	// Input image
	glBindTextureUnit(0, m_HMapTextureID);

	// Output image
	glBindImageTexture(0, m_conemapTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG8);

	// Uniforms
	glUniform1i(ul(m_programConemapID, "width"), m_HMres.x);
	glUniform1i(ul(m_programConemapID, "height"), m_HMres.y);
	glUniform1i(ul(m_programConemapID, "inputImage"), 0);

	// Setup time query
	glBeginQuery(GL_TIME_ELAPSED, m_timeQueryID);

	// Dispatch compute shader
	int localSizeX = 16;
	int localSizeY = 16;
	GLuint numWorkgroupsX = (m_HMres.x + localSizeX - 1) / localSizeX;
	GLuint numWorkgroupsY = (m_HMres.y + localSizeY - 1) / localSizeY;
	glDispatchCompute(numWorkgroupsX, numWorkgroupsY, 1);

	// Stop timer
	glEndQuery(GL_TIME_ELAPSED);

	// Set memory barrier for data integrity
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// Wait for compute shader termination
	GLuint done = 0;
	while (!done) {
		glGetQueryObjectuiv(m_timeQueryID, GL_QUERY_RESULT_AVAILABLE, &done);
	}

	// Get elapsed time
	GLuint64 timeElapsed;		// elapsed time in nano seconds
	glGetQueryObjectui64v(m_timeQueryID, GL_QUERY_RESULT, &timeElapsed);

	LOG("Conemap generated in ", timeElapsed / 1'000'000.f, "ms");

	glBindTexture(GL_TEXTURE_2D, 0);
}

void RayMarchedSurface::SetHeightmap(const char* path) {
	ImageRGBA image;
	image = ImageFromFile(path);
	glCreateTextures(GL_TEXTURE_2D, 1, &m_HMapTextureID);
	glTextureStorage2D(m_HMapTextureID, 1, GL_RGBA8, image.width, image.height);
	glBindTexture(GL_TEXTURE_2D, m_HMapTextureID);

	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTextureSubImage2D(m_HMapTextureID, 0, 0, 0, image.width, image.height, GL_RGBA, GL_UNSIGNED_BYTE, image.data());

	// Setup member fields
	m_HMres.x = image.width;
	m_HMres.y = image.height;

	// Generate conemap
	RenderConeMap();
}

void RayMarchedSurface::SetPointsBase() {
	std::vector<glm::vec4> Points{
		glm::vec4{m_pointsBase, 1},
		glm::vec4{m_pointsBase + m_pointsDir, 1}
	};
	glNamedBufferSubData(m_pointsSSBO, sizeof(glm::vec4) * SSBO_PADDING, sizeof(glm::vec4) * 2, Points.data());
	glNamedBufferSubData(m_debugSSBO, sizeof(glm::vec4), sizeof(glm::vec4) * 2, Points.data());
}

void RayMarchedSurface::InitSSBOs() {
	CleanSSBOs();

	// visual debug
	glGenBuffers(1, &m_pointsSSBO);
	LOG("Visual debug buffer generated (ID: ", m_pointsSSBO, ")");
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_pointsSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * 128, nullptr, GL_DYNAMIC_DRAW);
	glm::vec4 attr{ 2.f, 0, 0, 0 };
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4), &attr);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// numerical debug
	glGenBuffers(1, &m_debugSSBO);
	LOG("Numerical debug buffer generated (ID: ", m_debugSSBO, ")");
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_debugSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * 128, nullptr, GL_DYNAMIC_DRAW);
	attr = { 0, 0, 0, 0 };
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4), &attr);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	SetPointsBase();
}

void RayMarchedSurface::Render(const RenderParams& p) {
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
		glLineWidth(p.lineWidth);
	}
	else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// -- Activate shader --
	GLuint progID = GetProgramID();
	glUseProgram(progID);

	{	// SET UNIFORMS FOR PARALLAX MAPPING
		glUniform2f(ul(progID, "HMres"), m_HMres.x, m_HMres.y);
		// uniform vec2 HMres_r;   // reciprical of the height map resolution (1/w, 1/h)
		glUniform2f(ul(progID, "HMres_r"), 1.f / m_HMres.x, 1.f / m_HMres.y);
		// uniform float relax = 1.;
		glUniform1i(ul(progID, "maxSteps"), m_maxSteps);
		// uniform vec3 camPos;
		glUniform3fv(ul(progID, "camPos"), 1, glm::value_ptr(p.cameraPos));
		glUniform1i(ul(progID, "discardFragments"), m_discardFragments ? 1 : 0);
		glUniform1i(ul(progID, "displayNonConverged"), m_displayNonConverged ? 1 : 0);
		// uniform vec3 lightDir;
		glUniform3fv(ul(progID, "lightDir"), 1,  glm::value_ptr(m_lightDir));
		// uniform float lightIntensity = 1.;
		glUniform1f(ul(progID, "epsilon"), m_epsilon);

		glUniform1f(ul(progID, "modelNormalMult"), m_modelNormalMult);
		glUniform1i(ul(progID, "rayMarchingTechnique"), m_activeTechnique);

		glUniform1i(ul(progID, "SSBOPadding"), SSBO_PADDING);
		glUniform1i(ul(progID, "showSteps"), m_showSteps ? 1 : 0);
		glUniform1i(ul(progID, "showEnterExit"), m_showEnterExit ? 1 : 0);
		glUniform1i(ul(progID, "showFlags"), m_showFlags ? 1 : 0);

		glUniformMatrix4fv(ul(progID, "viewProj"), 1, GL_FALSE, glm::value_ptr(p.viewProj));
		glUniformMatrix4fv(ul(progID, "world"), 1, GL_FALSE, glm::value_ptr(modelTransform));
		glm::mat4 modelTransformIT = glm::inverse(glm::transpose(modelTransform));
		glUniformMatrix4fv(ul(progID, "worldIT"), 1, GL_FALSE, glm::value_ptr(modelTransformIT));
	}

	// --- Set debug buffers ---
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_pointsSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_debugSSBO);
	
	// --- Set cone map ---
	glBindTextureUnit(1, m_conemapTextureID);
	if (m_interpolateTexture) {
		glTextureParameteri(m_conemapTextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_conemapTextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else {
		glTextureParameteri(m_conemapTextureID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(m_conemapTextureID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	glUniform1i(ul(progID, "coneMap"), 1);

	// --- Render all meshes ---
	for (const auto& mesh : m_meshes) {
		mesh->Render(mp);
	}

	// -- Restore initial OGL state --
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

	if (cullFaceEnabled) glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT, polygonMode[0]);
	glPolygonMode(GL_BACK, polygonMode[1]);
	glLineWidth(defLineWidth);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

void RayMarchedSurface::SetObjPath() {
	std::string tempPath = m_objPathBuffer;
	if (tempPath == m_objPath) {
		return;
	}

	// clean old geometry
	CleanGeometry();
	CleanMaterials();

	// load the new file
	ModelLoaderData modelData = ModelLoader::LoadFromOBJ<VertexMergedNorm>(tempPath, "./", [](const std::vector<Vertex>& rawVerts) {
		return ModelLoader::MergeNormals(rawVerts);
	});
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