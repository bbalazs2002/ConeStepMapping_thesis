#include <gtest/gtest.h>
#include "Headers/Manager/MaterialManager.h"
#include "Headers/Material/Material.h"
#include <glm/glm.hpp>

// Helper: create a plain material with no textures (GL-free)
static Material MakeMat(const std::string& name,
                         glm::vec3 diffuse = { 0.8f, 0.8f, 0.8f },
                         float shininess   = 32.f) {
    Material m;
    m.SetName(name);
    m.SetDiffuseColor(diffuse);
    m.SetShininess(shininess);
    return m;
}

// UT-07: same material descriptor → same shared_ptr
TEST(MaterialManager, CacheHitReturnsSamePointer) {
    MaterialManager mgr;
    auto mat = MakeMat("red", { 1.f, 0.f, 0.f });
    auto p1 = mgr.GetOrCreate(mat);
    auto p2 = mgr.GetOrCreate(mat);
    EXPECT_EQ(p1.get(), p2.get());
}

// UT-08: different descriptor → different shared_ptr
TEST(MaterialManager, DifferentDescriptorGivesNewObject) {
    MaterialManager mgr;
    auto p1 = mgr.GetOrCreate(MakeMat("red",  { 1.f, 0.f, 0.f }));
    auto p2 = mgr.GetOrCreate(MakeMat("blue", { 0.f, 0.f, 1.f }));
    EXPECT_NE(p1.get(), p2.get());
}

// UT-09: weak_ptr GC — after all external refs drop, next call allocates fresh
TEST(MaterialManager, ExpiredWeakPtrTriggersReallocation) {
    MaterialManager mgr;
    auto mat = MakeMat("temp", { 0.5f, 0.5f, 0.5f });
    Material* rawPtr = nullptr;
    {
        auto ptr = mgr.GetOrCreate(mat);
        rawPtr = ptr.get();
    } // shared_ptr drops → weak_ptr expires

    auto ptr2 = mgr.GetOrCreate(mat);
    EXPECT_NE(ptr2.get(), rawPtr);
}

// Returned material carries correct properties
TEST(MaterialManager, CreatedMaterialHasCorrectProperties) {
    MaterialManager mgr;
    Material mat = MakeMat("shiny", { 0.8f, 0.2f, 0.1f }, 128.f);
    mat.SetAmbientColor({ 0.05f, 0.05f, 0.05f });
    mat.SetSpecularColor({ 1.f, 1.f, 1.f });

    auto ptr = mgr.GetOrCreate(mat);
    EXPECT_EQ(ptr->GetName(), "shiny");
    EXPECT_FLOAT_EQ(ptr->GetDiffuseColor().r, 0.8f);
    EXPECT_FLOAT_EQ(ptr->GetShininess(), 128.f);
    EXPECT_FLOAT_EQ(ptr->GetAmbientColor().r, 0.05f);
}

// Multiple distinct materials can coexist in the cache
TEST(MaterialManager, MultipleEntriesInCache) {
    MaterialManager mgr;
    auto p1 = mgr.GetOrCreate(MakeMat("m1", { 1.f, 0.f, 0.f }));
    auto p2 = mgr.GetOrCreate(MakeMat("m2", { 0.f, 1.f, 0.f }));
    auto p3 = mgr.GetOrCreate(MakeMat("m3", { 0.f, 0.f, 1.f }));

    EXPECT_NE(p1.get(), p2.get());
    EXPECT_NE(p2.get(), p3.get());
    EXPECT_NE(p1.get(), p3.get());

    // Cache hits
    EXPECT_EQ(mgr.GetOrCreate(MakeMat("m1", { 1.f, 0.f, 0.f })).get(), p1.get());
    EXPECT_EQ(mgr.GetOrCreate(MakeMat("m2", { 0.f, 1.f, 0.f })).get(), p2.get());
}
