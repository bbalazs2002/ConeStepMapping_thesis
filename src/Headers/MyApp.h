#pragma once

#include <vector>
#include <string>
#include <memory>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

// GLEW
#include <GL/glew.h>

// SDL
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

// Utils
#include "../Utils/Camera.h"
#include "../Utils/CameraManipulator.h"
#include "../Utils/GLUtils.hpp"

// Interface
#include "../Interfaces/IGraphicsApp.h"

#include "Model/Model.h"
#include "Model/RayMarchedSurface.h"

class MyApp : public IGraphicsApp
{
public:
	MyApp();
	~MyApp();

	bool Init();
	void Clean();

	void Update(const SUpdateInfo&);
	void Render();
	void RenderGUI();

	void KeyboardDown(const SDL_KeyboardEvent&);
	void KeyboardUp(const SDL_KeyboardEvent&);
	void MouseMove(const SDL_MouseMotionEvent&);
	void MouseDown(const SDL_MouseButtonEvent&);
	void MouseUp(const SDL_MouseButtonEvent&);
	void MouseWheel(const SDL_MouseWheelEvent&);
	void Resize(int, int);
	void OtherEvent(const SDL_Event&);

protected:
	void SetupDebugCallback();

	// Variables
	float m_ElapsedTimeInSec = 0.0f;
	int m_width = 640, m_height = 480;
	std::unique_ptr<RayMarchedSurface> m_model;

	// Camera
	Camera m_camera;
	CameraManipulator m_cameraManipulator;

	// Shader variables
	GLuint m_programModelID = 0;		// Program rendering models
	GLuint m_programRaymarchID = 0;		// Program showing ray-marched surfaces
	GLuint m_programAxesID = 0;			// Program showing X,Y,Z directions
	GLuint m_programSkyboxID = 0;		// Skybox shaders
	GLuint m_programConemapID = 0;		// Conemap generation

	// Shader initialization and termination
	void InitShaders();
	void CleanShaders();
	void InitSkyboxShader();
	void CleanSkyboxShader();
	void InitAxesShader();
	void CleanAxesShader();

	// Geometry variables
	OGLObject m_SkyboxGPU = {};

	// Geometry initialization and termination
	void InitGeometry();
	void CleanGeometry();
	void InitSkyboxGeometry();
	void CleanSkyboxGeometry();

	// Textures
	GLuint m_modelTextureID = 0;
	GLuint m_skyboxTextureID = 0;

	// Texture initialization
	void InitTexture();
	void CleanTexture();
	void InitSkyboxTexture();
	void CleanSkyboxTexture();

	// rendering methods
	void RenderAxes();
	void RenderSkybox();

	// ImGui state variables
	bool m_showAxes = true;

	int m_activeHeightMap = 0;
	std::vector<std::string> m_heightMaps{
		"Assets/HMaps/heightmap_dot.png",
		"Assets/HMaps/spikes.png",
		"Assets/HMaps/hemisphere.png",
		"Assets/HMaps/cone.jpg",
		"Assets/HMaps/heightmap-terrain.png",
		"Assets/HMaps/heightmap-terrain2.png",
		"Assets/HMaps/heightmap-river.png",
		"Assets/HMaps/heightmap-pyramid.jpg",
		"Assets/HMaps/heightmap-geometries.png",
		"Assets/HMaps/Earth-heightmap-small.png",
		"Assets/HMaps/desert-heightmap.jpg"
	};

	int m_activeTechnique = 1;
	std::vector<std::string> m_rayMarchingTechniques{
		"Linear search",
		"Cone step mapping"
	};
};