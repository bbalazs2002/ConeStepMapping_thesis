// standard
#include <iostream>
#include <string>
#include <sstream>
#include <memory>

// ImGui
#include <imgui.h>
#include <imgui_internal.h>

// Utils
#include "../Utils/SDL_GLDebugMessageCallback.h"
#include "../Utils/ProgramBuilder.h"
#include "../Utils/Log.h"

#include "../Headers/MyApp.h"
#include "../Headers/Types.h"
#include "../Headers/Model/Model.h"
#include "../Headers/Model/RayMarchedSurface.h"

MyApp::MyApp()
{
}
MyApp::~MyApp()
{
}

/////////////////////
// --- SHADERS --- //
/////////////////////
void MyApp::InitShaders()
{

	// conemap generation
	m_programConemapID = glCreateProgram();
	ProgramBuilder{ m_programConemapID }
		.ShaderStage(GL_COMPUTE_SHADER, "src/Shaders/Conemap/Comp_Conemap.comp")
		.Link();

	// models
	m_programModelID = glCreateProgram();
	ProgramBuilder{ m_programModelID }
		.ShaderStage(GL_VERTEX_SHADER, "src/Shaders/Models/Vert_Model.vert")
		.ShaderStage(GL_FRAGMENT_SHADER, "src/Shaders/Models/Frag_Model.frag")
		.Link();

	// ray-marched surfaces
	m_programRaymarchID = glCreateProgram();
	ProgramBuilder{ m_programRaymarchID}
		.ShaderStage(GL_VERTEX_SHADER, "src/Shaders/RayMarching/Vert_Model.vert")
		.ShaderStage(GL_GEOMETRY_SHADER, "src/Shaders/RayMarching/Geom_Model_old.geom")
		.ShaderStage(GL_FRAGMENT_SHADER, "src/Shaders/RayMarching/Frag_Improved.frag")
		.Link();

	InitAxesShader();
	InitSkyboxShader();
}
void MyApp::CleanShaders()
{
	glDeleteProgram(m_programConemapID);
	m_programConemapID = 0;

	glDeleteProgram(m_programModelID);
	m_programModelID = 0;

	CleanSkyboxShader();
	CleanAxesShader();
}
void MyApp::InitSkyboxShader() {
	m_programSkyboxID = glCreateProgram();
	ProgramBuilder{ m_programSkyboxID }
		.ShaderStage(GL_VERTEX_SHADER, "src/Shaders/Skybox/Vert_skybox.vert")
		.ShaderStage(GL_FRAGMENT_SHADER, "src/Shaders/Skybox/Frag_skybox_skeleton.frag")
		.Link();
}
void MyApp::CleanSkyboxShader() {
	glDeleteProgram(m_programSkyboxID);
	m_programSkyboxID = 0;
}
void MyApp::InitAxesShader()
{
	m_programAxesID = glCreateProgram();
	ProgramBuilder{ m_programAxesID }
		.ShaderStage(GL_VERTEX_SHADER, "src/Shaders/Axes/Vert_axes.vert")
		.ShaderStage(GL_FRAGMENT_SHADER, "src/Shaders/Axes/Frag_PosCol.frag")
		.Link();
}
void MyApp::CleanAxesShader()
{
	glDeleteProgram(m_programAxesID);
	m_programAxesID = 0;
}

//////////////////////
// --- GEOMETRY --- //
//////////////////////
void MyApp::InitGeometry()
{

	// Equinox from .obj file with .mtl
	/*
	m_model = std::make_unique<Model>(
		ModelParams{
			ShaderProgramCollection{
				m_programModelID
			},
			"Equinox",
			true
		}
	);
	// m_model.get()->SetObjPath("Assets/Equinox-render/Equinox.obj");
	m_model.get()->SetObjPath("Assets/Fire_Extinguisher/Fire_Extinguisher.obj");
	m_model.get()->AddTransform(glm::transpose(glm::mat4{
			{ 1, 0, 0, 0 },
			{ 0, 1, 0, 5 },
			{ 0, 0, 1, 0 },
			{ 0, 0, 0, 1 }
		}
	));
	*/

	m_model = std::make_unique<RayMarchedSurface>(
		RayMarchedSurfaceParams{
			ShaderProgramCollection{
				m_programRaymarchID,
				 m_programConemapID
			},
			"Quad",
			true
		}
	);
	m_model.get()->SetHeightmap("Assets/HMaps/heightmap_dot.png");
	m_model.get()->SetObjPath("Assets/Quad/Quad.obj");
	m_model.get()->AddTransform(glm::transpose(glm::mat4{
			{ 1, 0, 0, 0 },
			{ 0, 1, 0, 5 },
			{ 0, 0, 1, 0 },
			{ 0, 0, 0, 1 }
		}
	));

	InitSkyboxGeometry();
}
void MyApp::CleanGeometry()
{
	CleanSkyboxGeometry();
}
void MyApp::InitSkyboxGeometry() {
	// skybox geometry
	MeshObject<glm::vec3> skyboxCPU =
	{
		std::vector<glm::vec3>
		{
			// back
			glm::vec3(-1, -1, -1),
			glm::vec3(1, -1, -1),
			glm::vec3(1,  1, -1),
			glm::vec3(-1,  1, -1),
			// front
			glm::vec3(-1, -1, 1),
			glm::vec3(1, -1, 1),
			glm::vec3(1,  1, 1),
			glm::vec3(-1,  1, 1),
		},

		std::vector<GLuint>
		{
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

	m_SkyboxGPU = CreateGLObjectFromMesh(skyboxCPU, { { 0, offsetof(glm::vec3, x), 3, GL_FLOAT } });
}
void MyApp::CleanSkyboxGeometry()
{
	CleanOGLObject(m_SkyboxGPU);
}

//////////////////////
// --- TEXTURES --- //
//////////////////////
void MyApp::InitTexture() {
	InitSkyboxTexture();
}
void MyApp::CleanTexture() {
	CleanSkyboxTexture();
}
void MyApp::InitSkyboxTexture() {
	// skybox texture
	static const char* skyboxFiles[6] = {
		"Assets/sky1/px.png",
		"Assets/sky1/nx.png",
		"Assets/sky1/py.png",
		"Assets/sky1/ny.png",
		"Assets/sky1/pz.png",
		"Assets/sky1/nz.png",
	};

	ImageRGBA images[6];
	for (int i = 0; i < 6; ++i)
	{
		images[i] = ImageFromFile(skyboxFiles[i], false);
	}

	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_skyboxTextureID);
	glTextureStorage2D(m_skyboxTextureID, 1, GL_RGBA8, images[0].width, images[0].height);

	for (int face = 0; face < 6; ++face)
	{
		glTextureSubImage3D(m_skyboxTextureID, 0, 0, 0, face, images[face].width, images[face].height, 1, GL_RGBA, GL_UNSIGNED_BYTE, images[face].data());
	}

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}
void MyApp::CleanSkyboxTexture() {
	glDeleteTextures(1, &m_skyboxTextureID);
	m_skyboxTextureID = 0;
}

////////////////////////////
// --- INITIALIZATION --- //
////////////////////////////
void MyApp::SetupDebugCallback()
{
	// Enable and set the debug callback function if we are in debug context
	GLint context_flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
	if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
		glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, GL_DONT_CARE, 0, nullptr, GL_FALSE);
		glDebugMessageCallback(SDL_GLDebugMessageCallback, nullptr);
	}
}
bool MyApp::Init()
{
	SetupDebugCallback();

	// Set a bluish clear color
	// glClear() will use this for clearing the color buffer.
	glClearColor(0.125f, 0.25f, 0.5f, 1.0f);

	InitShaders();
	InitTexture();
	InitGeometry();

	//
	// Other
	//
	// glEnable(GL_CULL_FACE);	 // Enable discarding the back-facing faces.
	glCullFace(GL_BACK);     // GL_BACK: facets facing away from camera, GL_FRONT: facets facing towards the camera
	glEnable(GL_DEPTH_TEST); // Enable depth testing. (for overlapping geometry)
	glDepthFunc(GL_LESS);

	// Camera
	m_camera.SetView(
		glm::vec3(0, 10, 5),	// From where we look at the scene - eye
		glm::vec3(0, 4, 0),		// Which point of the scene we are looking at - at
		glm::vec3(0, 1, 0)		// Upwards direction - up
	);
	m_cameraManipulator.SetCamera(&m_camera);

	return true;
}

////////////////////
// --- UPDATE --- //
////////////////////
void MyApp::Update(const SUpdateInfo& updateInfo)
{
	m_cameraManipulator.Update(updateInfo.DeltaTimeInSec);
	m_ElapsedTimeInSec = updateInfo.ElapsedTimeInSec;
}

////////////////////
// --- RENDER --- //
////////////////////
void MyApp::RenderAxes()
{
	glUseProgram(m_programAxesID);

	glm::mat4 axisWorld = glm::translate(m_camera.GetAt());
	glProgramUniformMatrix4fv(m_programAxesID, ul(m_programAxesID, "viewProj"), 1, GL_FALSE, glm::value_ptr(m_camera.GetViewProj()));
	glProgramUniformMatrix4fv(m_programAxesID, ul(m_programAxesID, "world"), 1, GL_FALSE, glm::value_ptr(axisWorld));

	// We always want to see it, regardless of whether there is an object in front of it
	glDisable(GL_DEPTH_TEST);

	glDrawArrays(GL_LINES, 0, 6);
	glUseProgram(0);
	glEnable(GL_DEPTH_TEST);
}
void MyApp::RenderSkybox() {
	glUseProgram(m_programSkyboxID);

	glProgramUniform1i(m_programSkyboxID, ul(m_programSkyboxID, "skyboxTexture"), 1);
	glProgramUniformMatrix4fv(m_programSkyboxID, ul(m_programSkyboxID, "viewProj"), 1, GL_FALSE, glm::value_ptr(m_camera.GetViewProj()));
	glProgramUniformMatrix4fv(m_programSkyboxID, ul(m_programSkyboxID, "world"), 1, GL_FALSE, glm::value_ptr(glm::translate(m_camera.GetEye())));

	// Save the last Z-test, namely the relation by which we update the pixel.
	GLint prevDepthFnc;
	glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFnc);

	// Now we use less-then-or-equal, because we push everything to the far clipping plane
	glDepthFunc(GL_LEQUAL);

	glBindTextureUnit(1, m_skyboxTextureID);
	glBindVertexArray(m_SkyboxGPU.vaoID);

	glDrawElements(GL_TRIANGLES, m_SkyboxGPU.count, GL_UNSIGNED_INT, nullptr);

	glDepthFunc(prevDepthFnc);

	glUseProgram(0);
	glBindVertexArray(0);
	glBindTextureUnit(0, 0);
}
void MyApp::Render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// render model
	const RenderParams rp{
		1.f, m_camera.GetEye(), 0, glm::ivec2(m_width, m_height), m_camera.GetViewProj()
	};
	m_model.get()->Render(rp);

	RenderSkybox();
	if (m_showAxes) {
		RenderAxes();
	}
}
void MyApp::RenderGUI()
{
	ImGuiID dockspace_id;

	// Make a fullscreen invisible window to host the dockspace
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags host_flags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus |
			ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::Begin("DockspaceHost", nullptr, host_flags);
		dockspace_id = ImGui::GetID("MainDockspace");
		ImGui::PopStyleVar(3);

		ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

		ImGui::End();
	}

	// Init docked windows layout
	static bool ImGuiLayoutInitialized = false;
	if (!ImGuiLayoutInitialized) {
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

		ImGuiID dock_top, dock_left, dock_bottom;

		// First split bottom from the full width dockspace
		ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.5f, &dock_bottom, &dock_top);

		// Then split left from the remaining top area
		ImGui::DockBuilderSplitNode(dock_top, ImGuiDir_Left, 0.4f, &dock_left, &dock_top);

		ImGui::DockBuilderDockWindow("Global options", dock_left);
		ImGui::DockBuilderDockWindow("Model options", dock_left);
		ImGui::DockBuilderDockWindow("Visual debug", dock_bottom);
		ImGui::DockBuilderDockWindow("Numerical debug", dock_bottom);

		ImGui::DockBuilderFinish(dockspace_id);

		ImGuiLayoutInitialized = true;
	}

	auto m = m_model.get();

	////////////////////////////
	// --- Global options --- //
	////////////////////////////
	if (ImGui::Begin("Global options"))
	{
		ImGui::Text("Render resolution %dx%d", m_width, m_height);
		ImGui::Checkbox("Show axes", &m_showAxes);

		ImGui::SliderFloat3("light_dir", &m->m_lightDir.x, -10.f, 10.f);

		ImGui::SliderInt("max steps", &m->m_maxSteps, 1, 100);
		ImGui::SliderFloat("epsilon", &m->m_epsilon, 0.0, 1.0, "%.3f");
		ImGui::SliderFloat("modelNormalMult", &m->m_modelNormalMult, 0.1, 2.0, "%.3f");
		ImGui::Checkbox("show non-converge", &m->m_displayNonConverged);
		ImGui::Checkbox("discard fragments", &m->m_discardFragments);
		ImGui::Checkbox("show con-step flags", &m->m_showFlags);
	}
	ImGui::End();

	///////////////////////////
	// --- Model options --- //
	///////////////////////////
	if (ImGui::Begin("Model options"))
	{
		GLuint conemapID = m_model.get()->GetConemapID();
		if (conemapID > 0) {
			ImGui::Image(conemapID, ImVec2(256, 256));
		}

		auto wireframe = m->GetWireFrame();
		if (ImGui::Checkbox("Wireframe", &wireframe)) {
			m->SetWireFrame(wireframe);
		}

		// ray marching technique
		if (ImGui::BeginCombo("Ray marching technique", m_rayMarchingTechniques[m_activeTechnique].c_str()))
		{
			for (int i = 0; i < m_rayMarchingTechniques.size(); ++i) {
				if (ImGui::Selectable(m_rayMarchingTechniques[i].c_str(), m_activeTechnique == i)) {
					m_activeTechnique = i;
					m->m_activeTechnique = i;
				}
			}
			ImGui::EndCombo();
		}

		auto interpolate = m->GetInterpolateTexture();
		if (ImGui::Checkbox("Texture interpolation", &interpolate)) {
			m->SetInterpolateTexture(interpolate);
		}

		// heightmap
		int hmapID = m_activeHeightMap;
		if (ImGui::BeginCombo("Heightmap", m_heightMaps[hmapID].c_str()))
		{
			for (int i = 0; i < m_heightMaps.size(); ++i) {
				if (ImGui::Selectable(m_heightMaps[i].c_str(), hmapID == i)) {
					m_activeHeightMap = i;
					m->SetHeightmap(m_heightMaps[i].c_str());
				}
			}
			ImGui::EndCombo();
		}
	}
	ImGui::End();

	///////////////////
	// --- Debug --- //
	///////////////////
	if (ImGui::Begin("Visual debug"))
	{
		bool pointsChanged = false;
		glm::vec3 pb = m->GetDebugRayStart();
		glm::vec3 pd = m->GetDebugRayDir();

		ImGui::Checkbox("Show debug", &m->m_showDebug);
		ImGui::Checkbox("Show steps", &m->m_showSteps);
		ImGui::Checkbox("Show enter/exit", &m->m_showEnterExit);
		ImGui::Checkbox("Show cones", &m->m_showCones);
		if (ImGui::SliderFloat3("Base", &pb.x, -10.0f, 10.0f)) {
			pointsChanged = true;
		}
		if (ImGui::SliderFloat3("Direction", &pd.x, -1.0f, 1.0f)) {
			pointsChanged = true;
		}
		if (ImGui::Button("To camera")) {
			pb = m_camera.GetEye();
			pd = m_camera.GetAt() - pb;
			pointsChanged = true;
		}

		if (pointsChanged) {
			m->SetDebugRay(pb, glm::normalize(pd));
		}
	}
	ImGui::End();

	if (ImGui::Begin("Numerical debug"))
	{
		bool pointsChanged = false;
		glm::vec3 pb = m->GetDebugRayStart();
		glm::vec3 pd = m->GetDebugRayDir();

		if (ImGui::SliderFloat3("Base", &pb.x, -10.0f, 10.0f)) {
			pointsChanged = true;
		}
		if (ImGui::SliderFloat3("Direction", &pd.x, -1.0f, 1.0f)) {
			pointsChanged = true;
		}
		if (ImGui::Button("To camera")) {
			pb = m_camera.GetEye();
			pd = glm::normalize(m_camera.GetAt() - pb);
			pointsChanged = true;
		}

		if (pointsChanged) {
			m->SetDebugRay(pb, pd);
		}

		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m->GetDebugSSBOID());
			GLint size = 0;
			glGetBufferParameteriv(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE, &size);
			glm::vec4* data = (glm::vec4*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
			if (data) {
				ImGui::Text("Step count: %.4f", data[0].x);
				ImGui::Text("Termination flags: %d", (int)data[0].y);
				ImGui::Text("Eye (scene space): %.4f; %.4f; %.4f", data[1].x, data[1].y, data[1].z);
				ImGui::Text("Direction (scene space): %.4f; %.4f; %.4f", data[2].x, data[2].y, data[2].z);

				if (ImGui::BeginTable("Verteces (scene space)", 4)) {

					ImGui::TableSetupColumn("pos");
					ImGui::TableSetupColumn("norm");
					ImGui::TableSetupColumn("merged");
					ImGui::TableSetupColumn("tex");
					ImGui::TableHeadersRow();

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("0; 0; 0");
					ImGui::TableSetColumnIndex(1); ImGui::Text("0; 1; 0");
					ImGui::TableSetColumnIndex(2); ImGui::Text("0; 1; 0");
					ImGui::TableSetColumnIndex(3); ImGui::Text("0; 0");
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("0; 0; 1");
					ImGui::TableSetColumnIndex(1); ImGui::Text("0; 1; 0");
					ImGui::TableSetColumnIndex(2); ImGui::Text("0; 1; 0");
					ImGui::TableSetColumnIndex(3); ImGui::Text("0; 1");
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("1; 0; 1");
					ImGui::TableSetColumnIndex(1); ImGui::Text("0; 1; 0");
					ImGui::TableSetColumnIndex(2); ImGui::Text("0; 1; 0");
					ImGui::TableSetColumnIndex(3); ImGui::Text("1; 1");

					ImGui::EndTable();
				}

				ImGui::Text("scene2texture space:");
				if (ImGui::BeginTable("scene2texture space", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("%.4f", data[3].x);
					ImGui::TableSetColumnIndex(1); ImGui::Text("%.4f", data[3].y);
					ImGui::TableSetColumnIndex(2); ImGui::Text("%.4f", data[3].z);
					ImGui::TableSetColumnIndex(3); ImGui::Text("%.4f", data[3].w);
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("%.4f", data[4].x);
					ImGui::TableSetColumnIndex(1); ImGui::Text("%.4f", data[4].y);
					ImGui::TableSetColumnIndex(2); ImGui::Text("%.4f", data[4].z);
					ImGui::TableSetColumnIndex(3); ImGui::Text("%.4f", data[4].w);
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("%.4f", data[5].x);
					ImGui::TableSetColumnIndex(1); ImGui::Text("%.4f", data[5].y);
					ImGui::TableSetColumnIndex(2); ImGui::Text("%.4f", data[5].z);
					ImGui::TableSetColumnIndex(3); ImGui::Text("%.4f", data[5].w);
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("%.4f", data[6].x);
					ImGui::TableSetColumnIndex(1); ImGui::Text("%.4f", data[6].y);
					ImGui::TableSetColumnIndex(2); ImGui::Text("%.4f", data[6].z);
					ImGui::TableSetColumnIndex(3); ImGui::Text("%.4f", data[6].w);

					ImGui::EndTable();
				}
				ImGui::Text("Eye (texture space): %.4f; %.4f; %.4f; %.4f", data[7].x, data[7].y, data[7].z, data[7].w);

				ImGui::Text("scene2unit space:");
				if (ImGui::BeginTable("scene2unit space", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("%.4f", data[8].x);
					ImGui::TableSetColumnIndex(1); ImGui::Text("%.4f", data[8].y);
					ImGui::TableSetColumnIndex(2); ImGui::Text("%.4f", data[8].z);
					ImGui::TableSetColumnIndex(3); ImGui::Text("%.4f", data[8].w);
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("%.4f", data[9].x);
					ImGui::TableSetColumnIndex(1); ImGui::Text("%.4f", data[9].y);
					ImGui::TableSetColumnIndex(2); ImGui::Text("%.4f", data[9].z);
					ImGui::TableSetColumnIndex(3); ImGui::Text("%.4f", data[9].w);
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("%.4f", data[10].x);
					ImGui::TableSetColumnIndex(1); ImGui::Text("%.4f", data[10].y);
					ImGui::TableSetColumnIndex(2); ImGui::Text("%.4f", data[10].z);
					ImGui::TableSetColumnIndex(3); ImGui::Text("%.4f", data[10].w);
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("%.4f", data[11].x);
					ImGui::TableSetColumnIndex(1); ImGui::Text("%.4f", data[11].y);
					ImGui::TableSetColumnIndex(2); ImGui::Text("%.4f", data[11].z);
					ImGui::TableSetColumnIndex(3); ImGui::Text("%.4f", data[11].w);

					ImGui::EndTable();
				}
				ImGui::Text("Eye (unit space): %.4f; %.4f; %.4f; %.4f", data[12].x, data[12].y, data[12].z, data[12].w);

				ImGui::Text("In (texture space): %.4f; %.4f; %.4f", data[13].x, data[13].y, data[13].z);
				ImGui::Text("Out (texture space): %.4f; %.4f; %.4f", data[14].x, data[14].y, data[14].z);
				ImGui::Text("v (texture space): %.4f; %.4f; %.4f", data[15].x, data[15].y, data[15].z);

				ImGui::Text("plus data: %.4f; %.4f; %.4f; %.4f", data[16].x, data[16].y, data[16].z, data[16].w);

				if (ImGui::BeginTable("Steps", 5)) {
					ImGui::TableSetupColumn("ti");
					ImGui::TableSetupColumn("t");
					ImGui::TableSetupColumn("height");
					ImGui::TableSetupColumn("tan");
					ImGui::TableSetupColumn("ui");
					ImGui::TableHeadersRow();

					for (int i = 0; i < (int)floor(data[0].x); ++i) {
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0); ImGui::Text("%.4f", data[17 + i * 2].x);
						ImGui::TableSetColumnIndex(1); ImGui::Text("%.4f", data[17 + i * 2].y);
						ImGui::TableSetColumnIndex(2); ImGui::Text("%.4f", data[17 + i * 2].z);
						ImGui::TableSetColumnIndex(3); ImGui::Text("%.4f", data[17 + i * 2].w);
						ImGui::TableSetColumnIndex(4); ImGui::Text("%.4f; %.4f; %.4f", data[17 + i * 2 + 1].x, data[17 + i * 2 + 1].y, data[17 + i * 2 + 1].z);
					}

					ImGui::EndTable();

				}
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}
	}
	ImGui::End();
}

///////////////////
// --- CLEAN --- //
///////////////////
void MyApp::Clean()
{
	CleanShaders();
	CleanGeometry();
	CleanTexture();
}

////////////////////////////
// --- EVENT HANDLERS --- //
////////////////////////////
void MyApp::KeyboardDown(const SDL_KeyboardEvent& key)
{
	if (key.repeat == 0) // Triggers only once when held
	{
		if (key.key == SDLK_F5 && key.mod & SDL_KMOD_CTRL) // CTRL + F5
		{
			CleanShaders();
			InitShaders();
		}
		if (key.key == SDLK_F1) // F1
		{
			GLint polygonModeFrontAndBack[2] = {};
			// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGet.xhtml
			glGetIntegerv(GL_POLYGON_MODE, polygonModeFrontAndBack); // Query the current polygon mode. It gives the front and back modes separately.
			GLenum polygonMode = (polygonModeFrontAndBack[0] != GL_FILL ? GL_FILL : GL_LINE); // Switch between FILL and LINE
			// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glPolygonMode.xhtml
			glPolygonMode(GL_FRONT_AND_BACK, polygonMode); // Set the new polygon mode
		}
	}
	m_cameraManipulator.KeyboardDown(key);
}
void MyApp::KeyboardUp(const SDL_KeyboardEvent& key)
{
	m_cameraManipulator.KeyboardUp(key);
}
void MyApp::MouseMove(const SDL_MouseMotionEvent& mouse)
{
	m_cameraManipulator.MouseMove(mouse);
}
void MyApp::MouseDown(const SDL_MouseButtonEvent& mouse)
{
}
void MyApp::MouseUp(const SDL_MouseButtonEvent& mouse)
{
}
void MyApp::MouseWheel(const SDL_MouseWheelEvent& wheel)
{
	m_cameraManipulator.MouseWheel(wheel);
}
void MyApp::Resize(int _w, int _h)
{
	glViewport(0, 0, _w, _h);
	m_camera.SetAspect(static_cast<float>(_w) / _h);
	m_width = _w;
	m_height = _h;
}
void MyApp::OtherEvent(const SDL_Event& ev)
{
}