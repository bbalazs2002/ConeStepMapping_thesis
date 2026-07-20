#include <gtest/gtest.h>
#include "Headers/Manager/SceneManager.h"
#include "Headers/Manager/TextureManager.h"
#include "Headers/Manager/MaterialManager.h"
#include "Headers/Model/RayMarchedModel.h"
#include "Headers/Model/Model.h"
#include "Headers/Model/Mesh.h"
#include "Headers/Material/Material.h"
#include "Utils/ModelLoader.h"
#include "Utils/GLUtils.hpp"

// IT-11: OBJ loading → textures resolve → UV layout is correct (Vertex, not VertexMergedNorm)
TEST(Integration_OBJ, LoadFromOBJWithVertexLayoutSucceeds) {
    auto data = ModelLoader::LoadFromOBJ<Vertex>(
        "Assets/Fire_Extinguisher/Fire_Extinguisher.obj");
    ASSERT_FALSE(data.matMesh.empty());

    // Build meshes with Vertex (texcoord at location 2 — matches model shader)
    for (auto& mm : data.matMesh) {
        auto mesh = std::make_shared<Mesh>();
        EXPECT_NO_THROW(mesh->Build<Vertex>(std::move(mm.mesh)));
        EXPECT_NE(mesh->GetVAO(), 0u);
    }
}

// IT-12: same texture path cached — only one GL texture object created
TEST(Integration_TextureCache, SameHeightmapUsedByTwoModelsIsCached) {
    TextureManager mgr;
    auto t1 = mgr.GetOrLoad("Assets/HMaps/cone.jpg", false);
    auto t2 = mgr.GetOrLoad("Assets/HMaps/cone.jpg", false);
    ASSERT_NE(t1, nullptr);
    EXPECT_EQ(t1.get(), t2.get()); // same GL texture object
    EXPECT_EQ(t1->GetID(), t2->GetID());
}

// MaterialManager + Mesh: full mesh-material binding round-trip
TEST(Integration_Mesh_Material, MeshMaterialBindingRoundtrip) {
    MaterialManager matMgr;

    Material mat;
    mat.SetName("integration_test_mat");
    mat.SetDiffuseColor({ 0.8f, 0.2f, 0.1f });
    auto matPtr = matMgr.GetOrCreate(mat);

    MeshObject<Vertex> tri;
    tri.vertexArray = {
        { {  0.f, 1.f, 0.f }, { 0.f,0.f,1.f }, { 0.5f,1.f } },
        { { -1.f,-1.f, 0.f }, { 0.f,0.f,1.f }, { 0.f, 0.f } },
        { {  1.f,-1.f, 0.f }, { 0.f,0.f,1.f }, { 1.f, 0.f } },
    };
    tri.indexArray = { 0, 1, 2 };

    auto mesh = std::make_shared<Mesh>();
    mesh->Build<Vertex>(std::move(tri));
    mesh->SetMaterial(matPtr);

    EXPECT_EQ(mesh->GetMaterial().get(), matPtr.get());
    EXPECT_NE(mesh->GetVAO(), 0u);
}

// IT-05 (partial): SetHeightmap stores conemap reference (generator is nullptr → no-op)
TEST(Integration_RayMarchedModel, SetHeightmapWithNullGeneratorIsNoop) {
    TextureManager texMgr;
    auto heightmap = texMgr.GetOrLoad("Assets/HMaps/cone.jpg", false);
    ASSERT_NE(heightmap, nullptr);

    auto model = std::make_shared<RayMarchedModel>("test_rm");
    // gen == nullptr → SetHeightmap should not crash and m_conemap stays unchanged
    EXPECT_NO_THROW(model->SetHeightmap(heightmap, nullptr));
    // conemap should remain null (generator was null)
    EXPECT_EQ(model->GetConemap(), nullptr);
}

// SceneManager Add/Remove with real GL-backed objects
TEST(Integration_SceneManager, AddAndRemoveGLBackedModels) {
    SceneManager mgr;

    auto rm = std::make_shared<RayMarchedModel>("rm_test");
    auto md = std::make_shared<Model>("model_test");

    mgr.Add(rm);
    mgr.Add(md);
    EXPECT_EQ(mgr.GetSceneObjects().size(), 2u);

    mgr.Remove(rm);
    EXPECT_EQ(mgr.GetSceneObjects().size(), 1u);

    mgr.Remove(md);
    EXPECT_EQ(mgr.GetSceneObjects().size(), 0u);
}
