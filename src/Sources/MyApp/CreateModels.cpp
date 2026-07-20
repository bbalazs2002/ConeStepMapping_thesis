#include "MyApp.h"

#include "Headers/Model/RayMarchedModel.h"
#include "Headers/Model/Model.h"
#include "Headers/Model/Mesh.h"
#include "Headers/Material/Material.h"
#include "Headers/Types.h"
#include "Utils/ModelLoader.h"
#include <filesystem>

std::shared_ptr<RayMarchedModel> MyApp::CreateDefaultRayMarchedModel(const std::string& name)
{
    auto model = std::make_shared<RayMarchedModel>(name);
    model->SetTechnique(m_techniques["cone_step_mapping"]);
    model->SetSelectedProgram(m_shaderManager.Get("model_selected"));
    model->SetLightDir(m_lightDir);

    MeshObject<VertexMergedNorm> planeMesh;
    planeMesh.vertexArray = {
        { { -2.5f, 0.f, -2.5f }, { 0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f } },
        { {  2.5f, 0.f, -2.5f }, { 0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }, { 1.f, 0.f } },
        { {  2.5f, 0.f,  2.5f }, { 0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }, { 1.f, 1.f } },
        { { -2.5f, 0.f,  2.5f }, { 0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 1.f } },
    };
    planeMesh.indexArray = { 0, 1, 2,  0, 2, 3 };

    auto mesh = std::make_shared<Mesh>();
    mesh->Build<VertexMergedNorm>(std::move(planeMesh));

    Material mat;
    mat.SetName("surface_default");
    mat.SetDiffuseColor(glm::vec3(0.8f));
    mesh->SetMaterial(m_materialManager.GetOrCreate(mat));

    model->AddMesh(std::move(mesh));

    auto heightmap = m_textureManager.GetOrLoad(m_heightMaps[m_activeHeightmapIdx], false);
    model->SetHeightmap(heightmap, m_conemapGenerator.get());

    return model;
}

std::shared_ptr<Model> MyApp::CreateModelFromOBJ(const std::string& path)
{
    try {
        auto data = ModelLoader::LoadFromOBJ<Vertex>(path, "./");
        if (data.matMesh.empty()) return nullptr;

        std::string stemName = std::filesystem::path(path).stem().string();
        if (stemName.empty()) stemName = "Model";

        // Texture paths in the .mtl are relative to the .obj file's directory
        std::filesystem::path texFolder = std::filesystem::path(path).parent_path();

        auto model = std::make_shared<Model>(stemName);
        model->SetProgram(m_shaderManager.Get("model"));
        model->SetSelectedProgram(m_shaderManager.Get("model_selected"));

        for (auto& mm : data.matMesh) {
            auto mesh = std::make_shared<Mesh>();
            mesh->Build<Vertex>(std::move(mm.mesh));

            Material mat;

            if (mm.materialID >= 0 && mm.materialID < (int)data.materials.size()) {
                const auto& tmat = data.materials[mm.materialID];

                mat.SetName(tmat.name.empty() ? stemName + "_mat" : tmat.name);
                mat.SetAmbientColor (glm::vec3(tmat.ambient[0],  tmat.ambient[1],  tmat.ambient[2]));
                mat.SetDiffuseColor (glm::vec3(tmat.diffuse[0],  tmat.diffuse[1],  tmat.diffuse[2]));
                mat.SetSpecularColor(glm::vec3(tmat.specular[0], tmat.specular[1], tmat.specular[2]));
                mat.SetShininess(tmat.shininess);

                auto loadTex = [&](const std::string& texName) -> std::shared_ptr<Texture> {
                    if (texName.empty()) return nullptr;
                    auto absPath = ModelLoader::ResolveTexturePath(texName, texFolder);
                    return m_textureManager.GetOrLoad(absPath, false);
                };

                if (auto t = loadTex(tmat.diffuse_texname))  mat.SetDiffuseTex(t);
                if (auto t = loadTex(tmat.specular_texname)) mat.SetSpecularTex(t);
                if (auto t = loadTex(tmat.emissive_texname)) mat.SetEmissionTex(t);
                if (auto t = loadTex(tmat.normal_texname))   mat.SetNormalTex(t);
            } else {
                mat.SetName(stemName + "_default");
                mat.SetDiffuseColor(glm::vec3(0.8f));
            }

            mesh->SetMaterial(m_materialManager.GetOrCreate(mat));
            model->AddMesh(std::move(mesh));
        }

        return model;
    } catch (...) {
        return nullptr;
    }
}
