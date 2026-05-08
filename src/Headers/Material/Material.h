#pragma once

#include <string>
#include <filesystem>
#include <memory>

// TinyObjLoader
#include <tiny_obj_loader.h>

// GLM
#include <glm/glm.hpp>

// GLEW
#include <GL/glew.h>

// Utils
#include "../../Utils/Log.h"
#include "../../Utils/GLUtils.hpp"
#include "../../Utils/ModelLoader.h"

class Material {
private:
	std::string m_name = "default";

	glm::vec3 m_ambientColor{ 1.0f };
	glm::vec3 m_diffuseColor{ 1.0f };
	glm::vec3 m_specularColor{ 1.0f };

	float m_shininess = 32.0f;

	GLuint m_diffuseTex = 0;
	GLuint m_specularTex = 0;
	GLuint m_emissionTex = 0;
	GLuint m_normalTex = 0;

	static inline GLuint lastTextureTargets[4];

public:

	Material() {}
	
	~Material() {
		glDeleteTextures(1, &m_diffuseTex);
		m_diffuseTex = 0;
		glDeleteTextures(1, &m_specularTex);
		m_specularTex = 0;
		glDeleteTextures(1, &m_emissionTex);
		m_emissionTex = 0;
		glDeleteTextures(1, &m_normalTex);
		m_normalTex = 0;
	}

	// Name
	inline const std::string& GetName() const { return m_name; }
	inline void SetName(const std::string& name) { m_name = name; }

	// Ambient Color
	inline const glm::vec3& GetAmbientColor() const { return m_ambientColor; }
	inline void SetAmbientColor(const glm::vec3& color) { m_ambientColor = color; }

	// Diffuse Color
	inline const glm::vec3& GetDiffuseColor() const { return m_diffuseColor; }
	inline void SetDiffuseColor(const glm::vec3& color) { m_diffuseColor = color; }

	// Specular Color
	inline const glm::vec3& GetSpecularColor() const { return m_specularColor; }
	inline void SetSpecularColor(const glm::vec3& color) { m_specularColor = color; }

	// Shininess
	inline float GetShininess() const { return m_shininess; }
	inline void SetShininess(float shininess) { m_shininess = shininess; }

	// Diffuse Texture
	inline GLuint GetDiffuseTex() const { return m_diffuseTex; }
	inline void SetDiffuseTex(GLuint tex) { m_diffuseTex = tex; }

	// Specular Texture
	inline GLuint GetSpecularTex() const { return m_specularTex; }
	inline void SetSpecularTex(GLuint tex) { m_specularTex = tex; }

	// Emission Texture
	inline GLuint GetEmissionTex() const { return m_emissionTex; }
	inline void SetEmissionTex(GLuint tex) { m_emissionTex = tex; }

	// Normal Texture
	inline GLuint GetNormalTex() const { return m_normalTex; }
	inline void SetNormalTex(GLuint tex) { m_normalTex = tex; }

	// --- static utilities ---

	/**
	 * @brief Loads an image from a file, creates an OpenGL texture object, and configures its parameters.
	 *
	 * This function handles file reading, error checking, texture object generation,
	 * data upload, mipmap creation, and sets standard filtering and wrapping modes.
	 *
	 * @param path The filesystem path to the image file to load.
	 * @param flip Boolean flag to indicate whether the image data should be flipped vertically (e.g., for common image libraries that load origin at top-left).
	 * @return The OpenGL texture ID (GLuint) if successful, or 0 if loading failed or the path was empty.
	 */
	static inline GLuint LoadTexture(const std::filesystem::path& path, bool flip = false) {
		if (path.empty()) return 0;

		ImageRGBA img = ImageFromFile(path, flip);
		if (img.width <= 0 || img.height <= 0) {
			LOG_ERROR("[Material] Failed to load texture: ", path.generic_string());
			return 0;
		}

		GLuint texID;
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height,
			0, GL_RGBA, GL_UNSIGNED_BYTE, img.data());
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindTexture(GL_TEXTURE_2D, 0);

		return texID;
	}

	/**
	 * @brief Synchronizes material state with the GPU by binding textures and updating shader uniforms.
	 *
	 * Efficiently packs color and texture handles into vec4 structures (materialData) to reduce
	 * uniform overhead. It handles conditional texture binding based on availability and maps
	 * samplers to the designated texture units provided in textureTargets.
	 *
	 * @note The target shader program must be active (glUseProgram) before calling this function.
	 *
	 * @param programID The handle of the currently active shader program.
	 * @param material Pointer to the source material containing visual properties and GL handles.
	 * @param textureTargets Array defining the texture unit indices for [Diffuse, Specular, Emission, Normal] maps.
	 */
	static inline void UploadMaterialToShader(
		GLuint programID, const std::shared_ptr<Material> material,
		GLuint textureTargets[4]	// diffuse, specular, emission, normal
	) {
		// --- Uniform Helper Lambdas ---
		auto setVec4 = [&](const std::string& name, const glm::vec4& v) {
			GLint loc = glGetUniformLocation(programID, name.c_str());
			if (loc >= 0) glUniform4fv(loc, 1, &v[0]);
		};

		auto setFloat = [&](const std::string& name, float value) {
			GLint loc = glGetUniformLocation(programID, name.c_str());
			if (loc >= 0) glUniform1f(loc, value);
		};

		auto setInt = [&](const std::string& name, int value) {
			GLint loc = glGetUniformLocation(programID, name.c_str());
			if (loc >= 0) glUniform1i(loc, value);
		};

		// save texture data
		memcpy(textureTargets, lastTextureTargets, sizeof(GLuint) * 4);

		// --- Upload Scalar and Color Uniforms (materialData struct) ---
		setVec4("materialData.diffuseColorTex", glm::vec4(material->GetDiffuseColor(), material->GetDiffuseTex()));
		setVec4("materialData.specularColorTex", glm::vec4(material->GetSpecularColor(), material->GetSpecularTex()));
		setVec4("materialData.ambientColorEmissionTex", glm::vec4(material->GetAmbientColor(), material->GetEmissionTex()));
		setFloat("materialData.shininess", material->GetShininess());
		setInt("materialData.hasNormalTex", material->GetNormalTex());

		// --- Texture Binding and Sampler Setup ---
		// 1. Diffuse Map
		if (material->GetDiffuseTex() > 0) {
			glActiveTexture(GL_TEXTURE0 + textureTargets[0]);
			glBindTexture(GL_TEXTURE_2D, material->GetDiffuseTex());
			setInt("materialDiffuseTex", textureTargets[0]);
		}
		// 2. Specular Map
		if (material->GetSpecularTex() > 0) {
			glActiveTexture(GL_TEXTURE0 + textureTargets[1]);
			glBindTexture(GL_TEXTURE_2D, material->GetSpecularTex());
			setInt("materialSpecularTex", textureTargets[1]);
		}
		// 3. Emission Map
		if (material->GetEmissionTex() > 0) {
			glActiveTexture(GL_TEXTURE0 + textureTargets[2]);
			glBindTexture(GL_TEXTURE_2D, material->GetEmissionTex());
			setInt("materialEmissionTex", textureTargets[2]);
		}
		// 4. Normal Map
		if (material->GetNormalTex() > 0) {
			glActiveTexture(GL_TEXTURE0 + textureTargets[3]);
			glBindTexture(GL_TEXTURE_2D, material->GetNormalTex());
			setInt("materialNormalTex", textureTargets[3]);
		}

		// Restore the active texture unit to GL_TEXTURE0 for safety after all bindings.
		glActiveTexture(GL_TEXTURE0);
	}

	static inline void UploadMaterialToShader(
		GLuint programID, const std::shared_ptr<Material> material
	) {
		GLuint tt[] = { 0,1,2,3 };
		UploadMaterialToShader(programID, material, tt);
	}

	/**
	 * @brief Unbinds material textures from their respective texture units and resets the active unit.
	 * * This cleanup ensures that no stale texture state remains bound to the pipeline,
	 * preventing unintended sampling in subsequent draw calls.
	 */
	static inline void ClearMaterialFromShader() {
		// 1. Diffuse Map
		glActiveTexture(GL_TEXTURE0 + lastTextureTargets[0]);
		glBindTexture(GL_TEXTURE_2D, 0);
		// 2. Specular Map
		glActiveTexture(GL_TEXTURE0 + lastTextureTargets[1]);
		glBindTexture(GL_TEXTURE_2D, 0);
		// 3. Emission Map
		glActiveTexture(GL_TEXTURE0 + lastTextureTargets[2]);
		glBindTexture(GL_TEXTURE_2D, 0);
		// 4. Normal Map
		glActiveTexture(GL_TEXTURE0 + lastTextureTargets[3]);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Restore the active texture unit to GL_TEXTURE0 for safety after all bindings.
		glActiveTexture(GL_TEXTURE0);
	}

	static std::vector<std::shared_ptr<Material>> convertFromTinyObjLoader(std::vector<tinyobj::material_t> materials, const std::filesystem::path& textureFolder = "./") {

		std::vector<std::shared_ptr<Material>> retVal;

		if (materials.empty()) {
			retVal.push_back(std::make_shared<Material>());
		}

		for (const auto& mat : materials) {
			auto material = std::make_shared<Material>();
			material->SetName(mat.name);
			material->SetDiffuseColor(glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]));
			material->SetSpecularColor(glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]));
			material->SetAmbientColor(glm::vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2]));
			material->SetShininess(mat.shininess);

			if (!mat.diffuse_texname.empty())
				material->SetDiffuseTex(Material::LoadTexture(ModelLoader::ResolveTexturePath(mat.diffuse_texname, textureFolder)));
			if (!mat.specular_texname.empty())
				material->SetSpecularTex(Material::LoadTexture(ModelLoader::ResolveTexturePath(mat.specular_texname, textureFolder)));
			if (!mat.emissive_texname.empty())
				material->SetEmissionTex(Material::LoadTexture(ModelLoader::ResolveTexturePath(mat.emissive_texname, textureFolder)));
			if (!mat.normal_texname.empty())
				material->SetNormalTex(Material::LoadTexture(ModelLoader::ResolveTexturePath(mat.normal_texname, textureFolder)));

			retVal.push_back(material);
		}

		return retVal;
	}

};