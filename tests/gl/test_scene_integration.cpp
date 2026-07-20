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
#include <filesystem>
#include <fstream>

// IT-11: OBJ loading -> Vertex layout -> Mesh::Build round-trip
// Creates a minimal single-triangle OBJ in a temp directory so the test has
// no dependency on the Assets/ folder (which is gitignored).
TEST(Integration_OBJ, LoadFromOBJWithVertexLayoutSucceeds) {
    auto tmpDir = std::filesystem::temp_directory_path() / "csm_integration_obj";
    std::filesystem::create_directories(tmpDir);
    auto objPath = tmpDir / "tri.obj";
    {
        std::ofstream ofs(objPath);
        ofs << "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
               "vn 0 0 1\n"
               "vt 0 0\nvt 1 0\nvt 0 1\n"
               "f 1/1/1 2/2/1 3/3/1\n";
    }

    auto data = ModelLoader::LoadFromOBJ<Vertex>(objPath); // absolute path
    std::error_code ec;
    std::filesystem::remove_all(tmpDir, ec);

    ASSERT_FALSE(data.matMesh.empty());

    for (auto& mm : data.matMesh) {
        auto mesh = std::make_shared<Mesh>();
        EXPECT_NO_THROW(mesh->Build<Vertex>(std::move(mm.mesh)));
        EXPECT_NE(mesh->GetVAO(), 0u);
    }
}

// IT-12: same texture path cached — only one GL texture object created
// NOTE: Assets/HMaps/cone.jpg is intentionally not committed to the repo.
// If the file is missing, GetOrLoad still returns a non-null (but invalid) Texture.
// This is deliberate: the test only verifies cache identity (t1.get() == t2.get()),
// not that the texture loaded successfully. GetID() equality holds whether both
// are valid (same ID > 0) or both invalid (both 0).
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

// IT-05 (partial): SetHeightmap with nullptr generator is a no-op
// NOTE: Assets/HMaps/cone.jpg is intentionally not committed to the repo.
// The texture may be invalid (file missing on CI), but that is fine here:
// the test only checks that SetHeightmap does not crash when the generator
// is null, and that m_conemap stays null afterwards. Texture validity is
// irrelevant to this control-flow path.
TEST(Integration_RayMarchedModel, SetHeightmapWithNullGeneratorIsNoop) {
    TextureManager texMgr;
    auto heightmap = texMgr.GetOrLoad("Assets/HMaps/cone.jpg", false);
    ASSERT_NE(heightmap, nullptr);

    auto model = std::make_shared<RayMarchedModel>("test_rm");
    EXPECT_NO_THROW(model->SetHeightmap(heightmap, nullptr));
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
