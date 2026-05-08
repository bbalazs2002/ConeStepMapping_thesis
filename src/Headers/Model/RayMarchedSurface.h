#pragma once

#include <memory>

#include "Model.h"
#include "../Types.h"
#include "../Material/Material.h"
#include "../Model/Mesh.h"
#include "../config.h"

class RayMarchedSurface : public Model {
protected:
	std::string m_hMapPath;
	GLuint m_HMapTextureID = 0;
	GLuint m_conemapTextureID = 0;
	GLuint m_programConemapID = 0;
	GLuint m_timeQueryID = 0;
	glm::vec2 m_HMres{ 0,0 };

	void RenderConeMap();

	// Debug
	void SetPointsBase();
	void InitSSBOs();
	void CleanSSBOs() {
		glDeleteBuffers(1, &m_pointsSSBO);
		m_pointsSSBO = 0;
		glDeleteBuffers(1, &m_debugSSBO);
		m_debugSSBO = 0;
	}

	glm::vec3 m_pointsBase{2.2f, .5f, 0};
	glm::vec3 m_pointsDir{ 0, 0, 1.f };
	bool m_interpolateTexture = true;

	// Debug buffers
	GLuint m_pointsSSBO = 0;
	GLuint m_debugSSBO = 0;

public:

	// Render options
	GLint m_maxSteps = 64;
	bool m_discardFragments = true;
	bool m_displayNonConverged = false;
	glm::vec3 m_lightDir{ 0, 5, 0 };
	GLfloat m_epsilon = 1.f;
	GLfloat m_modelNormalMult = 1.f;
	GLint m_activeTechnique = 1;
	bool m_showDebug = false;
	bool m_showSteps = false;
	bool m_showCones = false;
	bool m_showEnterExit = false;
	bool m_showFlags = false;
	
	char m_objHMapPathBuffer[256] = "";

	RayMarchedSurface(const RayMarchedSurfaceParams& params);
	~RayMarchedSurface() {
		glDeleteQueries(1, &m_timeQueryID);

		glDeleteTextures(1, &m_conemapTextureID);
		m_conemapTextureID = 0;

		glDeleteTextures(1, &m_HMapTextureID);
		m_HMapTextureID = 0;

		CleanSSBOs();
	}

	void SetDebugRay(glm::vec3 point, glm::vec3 direction) {
		m_pointsBase = point;
		m_pointsDir = direction;
		SetPointsBase();
	}
	void SetDebugRayStart(glm::vec3 point) {
		m_pointsBase = point;
		SetPointsBase();
	}
	glm::vec3 GetDebugRayStart() {
		return m_pointsBase;
	}
	void SetDebubRayDir(glm::vec3 direction) {
		m_pointsDir = direction;
		SetPointsBase();
	}
	glm::vec3 GetDebugRayDir() {
		return m_pointsDir;
	}

	GLuint GetDebugSSBOID() {
		return m_debugSSBO;
	}

	void SetInterpolateTexture(bool interpolate) {
		m_interpolateTexture = interpolate;

		RenderConeMap();
	}
	bool GetInterpolateTexture() {
		return m_interpolateTexture;
	}
	
	// IDrawable methods
	void Render(const RenderParams& p) override;

	void SetHeightmap(const char* path);
	GLuint GetConemapID() const {
		return m_conemapTextureID;
	}

	using Model::SetObjPath;
	void SetObjPath() override;
};