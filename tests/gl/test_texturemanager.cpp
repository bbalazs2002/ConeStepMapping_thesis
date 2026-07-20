#include <gtest/gtest.h>
#include "Headers/Manager/TextureManager.h"
#include "Headers/Texture/Texture.h"

// Test asset paths (relative to PROJECT_ROOT, resolved via RelToAbsPath inside Texture)
static constexpr const char* kHeightmap1 = "Assets/HMaps/cone.jpg";
static constexpr const char* kHeightmap2 = "Assets/HMaps/desert-heightmap.jpg";

// UT-10: same path + same flip → same shared_ptr (cache hit)
TEST(TextureManager, CacheHitReturnsSamePointer) {
    TextureManager mgr;
    auto t1 = mgr.GetOrLoad(kHeightmap1, false);
    auto t2 = mgr.GetOrLoad(kHeightmap1, false);
    ASSERT_NE(t1, nullptr);
    EXPECT_EQ(t1.get(), t2.get());
}

// UT-11: same path, different flip → different objects
TEST(TextureManager, DifferentFlipGivesDifferentObject) {
    TextureManager mgr;
    auto t_noflip = mgr.GetOrLoad(kHeightmap1, false);
    auto t_flip   = mgr.GetOrLoad(kHeightmap1, true);
    ASSERT_NE(t_noflip, nullptr);
    ASSERT_NE(t_flip,   nullptr);
    EXPECT_NE(t_noflip.get(), t_flip.get());
}

// UT-12: missing file does not crash (returns empty/null or invalid texture)
TEST(TextureManager, MissingFileDoesNotCrash) {
    TextureManager mgr;
    EXPECT_NO_THROW({
        auto t = mgr.GetOrLoad("Assets/does_not_exist_12345.png", false);
        // We don't require null — implementation may return a valid but empty Texture.
        // The only requirement is no crash and no exception.
        (void)t;
    });
}

// UT-13: weak_ptr GC — after dropping all refs, next GetOrLoad creates new object
TEST(TextureManager, ExpiredCacheMissCreatesNewObject) {
    TextureManager mgr;
    Texture* rawPtr = nullptr;
    {
        auto t = mgr.GetOrLoad(kHeightmap2, false);
        ASSERT_NE(t, nullptr);
        rawPtr = t.get();
    } // external ref dropped → weak_ptr may expire

    auto t2 = mgr.GetOrLoad(kHeightmap2, false);
    ASSERT_NE(t2, nullptr);
    EXPECT_NE(t2.get(), rawPtr);
}

// Loaded texture has a valid GL ID and passes IsValid()
TEST(TextureManager, LoadedTextureIsValid) {
    TextureManager mgr;
    auto t = mgr.GetOrLoad(kHeightmap1, false);
    ASSERT_NE(t, nullptr);
    EXPECT_TRUE(t->IsValid());
    EXPECT_NE(t->GetID(), 0u);
}

// Two different paths → two different objects
TEST(TextureManager, DifferentPathsGiveDifferentObjects) {
    TextureManager mgr;
    auto t1 = mgr.GetOrLoad(kHeightmap1, false);
    auto t2 = mgr.GetOrLoad(kHeightmap2, false);
    ASSERT_NE(t1, nullptr);
    ASSERT_NE(t2, nullptr);
    EXPECT_NE(t1.get(), t2.get());
}

// Cached count reflects live entries
TEST(TextureManager, CachedCountReflectsLiveEntries) {
    TextureManager mgr;
    auto t1 = mgr.GetOrLoad(kHeightmap1, false);
    auto t2 = mgr.GetOrLoad(kHeightmap2, false);
    // At least these 2 are live; count may include expired entries until GC
    EXPECT_GE(mgr.GetCachedCount(), 2u);
}
