#include <gtest/gtest.h>
#include "Headers/Manager/TextureManager.h"
#include "Headers/Texture/Texture.h"
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <array>
#include <utility>
#include <GL/glew.h>

// ---------------------------------------------------------------------------
// Fixture: writes a minimal valid 1x1 white BMP to a temp path.
// SDL_Image can load BMP without any extra codecs, so this works on any runner.
// ---------------------------------------------------------------------------
class TextureManagerFixture : public ::testing::Test {
protected:
    std::filesystem::path tex1;
    std::filesystem::path tex2;

    void SetUp() override {
        auto tmp = std::filesystem::temp_directory_path();
        tex1 = tmp / "csm_test_tex1.bmp";
        tex2 = tmp / "csm_test_tex2.bmp";
        WriteBMP(tex1);
        WriteBMP(tex2);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove(tex1, ec);
        std::filesystem::remove(tex2, ec);
    }

private:
    static void WriteBMP(const std::filesystem::path& p) {
        // 1x1 white pixel, 24-bit RGB BMP — 58 bytes, no external dependencies
        static const uint8_t kBmp[] = {
            // BITMAPFILEHEADER (14 bytes)
            0x42, 0x4D,              // 'BM'
            0x3A, 0x00, 0x00, 0x00, // file size = 58
            0x00, 0x00, 0x00, 0x00, // reserved
            0x36, 0x00, 0x00, 0x00, // pixel data offset = 54
            // BITMAPINFOHEADER (40 bytes)
            0x28, 0x00, 0x00, 0x00, // header size = 40
            0x01, 0x00, 0x00, 0x00, // width = 1
            0x01, 0x00, 0x00, 0x00, // height = 1
            0x01, 0x00,              // color planes = 1
            0x18, 0x00,              // bits per pixel = 24
            0x00, 0x00, 0x00, 0x00, // compression = BI_RGB
            0x04, 0x00, 0x00, 0x00, // image data size = 4
            0x13, 0x0B, 0x00, 0x00, // X pixels/meter
            0x13, 0x0B, 0x00, 0x00, // Y pixels/meter
            0x00, 0x00, 0x00, 0x00, // colors in table
            0x00, 0x00, 0x00, 0x00, // important colors
            // Pixel data: BGR white + 1 byte row padding
            0xFF, 0xFF, 0xFF, 0x00
        };
        std::ofstream ofs(p, std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(kBmp), sizeof(kBmp));
    }
};

// UT-10: same path + same flip -> same shared_ptr (cache hit)
TEST_F(TextureManagerFixture, CacheHitReturnsSamePointer) {
    TextureManager mgr;
    auto t1 = mgr.GetOrLoad(tex1, false);
    auto t2 = mgr.GetOrLoad(tex1, false);
    ASSERT_NE(t1, nullptr);
    EXPECT_EQ(t1.get(), t2.get());
}

// UT-11: same path, different flip -> different objects
TEST_F(TextureManagerFixture, DifferentFlipGivesDifferentObject) {
    TextureManager mgr;
    auto t_noflip = mgr.GetOrLoad(tex1, false);
    auto t_flip   = mgr.GetOrLoad(tex1, true);
    ASSERT_NE(t_noflip, nullptr);
    ASSERT_NE(t_flip,   nullptr);
    EXPECT_NE(t_noflip.get(), t_flip.get());
}

// UT-12: missing file does not crash
TEST(TextureManager, MissingFileDoesNotCrash) {
    TextureManager mgr;
    EXPECT_NO_THROW({
        auto t = mgr.GetOrLoad(
            std::filesystem::temp_directory_path() / "csm_no_such_texture_xyz.png",
            false);
        (void)t;
    });
}

// UT-13: weak_ptr GC — after dropping all refs, next GetOrLoad reloads instead
// of resurrecting the expired entry.
//
// Note: this used to compare `t2.get() != rawPtr` (raw pointer identity).
// That is not a valid check in C++ — once the first Texture is destroyed,
// its heap slot is free, and make_shared for the second Texture may (and,
// empirically, often does) land on that exact same address. The test was
// flaky (~35% failure rate over 20 runs) purely from allocator reuse, not
// from any defect in TextureManager. The real invariant to check is
// behavioral: the cache must not keep a dangling entry around, and the
// reloaded texture must be a genuinely valid, freshly created GL object.
TEST_F(TextureManagerFixture, ExpiredCacheMissCreatesNewObject) {
    TextureManager mgr;
    {
        auto t = mgr.GetOrLoad(tex2, false);
        ASSERT_NE(t, nullptr);
    } // external ref dropped -> weak_ptr expires

    auto t2 = mgr.GetOrLoad(tex2, false);
    ASSERT_NE(t2, nullptr);
    EXPECT_TRUE(t2->IsValid());
    EXPECT_NE(t2->GetID(), 0u);
    // Exactly one live entry: the expired one was purged, not accumulated.
    EXPECT_EQ(mgr.GetCachedCount(), 1u);
}

// Loaded texture has a valid GL ID and passes IsValid()
TEST_F(TextureManagerFixture, LoadedTextureIsValid) {
    TextureManager mgr;
    auto t = mgr.GetOrLoad(tex1, false);
    ASSERT_NE(t, nullptr);
    EXPECT_TRUE(t->IsValid());
    EXPECT_NE(t->GetID(), 0u);
}

// Two different paths -> two different objects
TEST_F(TextureManagerFixture, DifferentPathsGiveDifferentObjects) {
    TextureManager mgr;
    auto t1 = mgr.GetOrLoad(tex1, false);
    auto t2 = mgr.GetOrLoad(tex2, false);
    ASSERT_NE(t1, nullptr);
    ASSERT_NE(t2, nullptr);
    EXPECT_NE(t1.get(), t2.get());
}

// Cached count reflects live entries
TEST_F(TextureManagerFixture, CachedCountReflectsLiveEntries) {
    TextureManager mgr;
    auto t1 = mgr.GetOrLoad(tex1, false);
    auto t2 = mgr.GetOrLoad(tex2, false);
    EXPECT_GE(mgr.GetCachedCount(), 2u);
}

// Move construction/assignment transfer GL id ownership; the moved-from
// object is left with id 0 so its destructor is a no-op (no double-delete).
TEST_F(TextureManagerFixture, MoveConstructionTransfersOwnership) {
    Texture a(tex1, false);
    GLuint id = a.GetID();
    ASSERT_NE(id, 0u);

    Texture b(std::move(a));
    EXPECT_EQ(b.GetID(), id);
    EXPECT_EQ(a.GetID(), 0u);
}

TEST_F(TextureManagerFixture, MoveAssignmentTransfersOwnershipAndFreesTarget) {
    Texture a(tex1, false);
    GLuint idA = a.GetID();
    ASSERT_NE(idA, 0u);

    Texture b(tex2, false);
    GLuint idB = b.GetID();
    ASSERT_NE(idB, 0u);
    ASSERT_NE(idA, idB);

    b = std::move(a);
    EXPECT_EQ(b.GetID(), idA);
    EXPECT_EQ(a.GetID(), 0u);
}

// ---------------------------------------------------------------------------
// Cubemap — Texture's 3rd constructor and TextureManager::GetOrLoadCubemap()
// were entirely untested (the skybox is the only production consumer, and
// Assets/ is gitignored, so no test previously touched this path). Reuses
// the same 1x1 BMP for all 6 faces -- content doesn't matter, only that
// each face loads and the resulting object is a valid GL_TEXTURE_CUBE_MAP.
// ---------------------------------------------------------------------------

class CubemapFixture : public ::testing::Test {
protected:
    std::array<std::filesystem::path, 6> faces;

    void SetUp() override {
        auto tmp = std::filesystem::temp_directory_path();
        static const char* names[6] = {
            "csm_cube_px.bmp", "csm_cube_nx.bmp", "csm_cube_py.bmp",
            "csm_cube_ny.bmp", "csm_cube_pz.bmp", "csm_cube_nz.bmp"
        };
        for (int i = 0; i < 6; ++i) {
            faces[i] = tmp / names[i];
            WriteWhiteBMP(faces[i]);
        }
    }

    void TearDown() override {
        std::error_code ec;
        for (const auto& f : faces) std::filesystem::remove(f, ec);
    }

private:
    static void WriteWhiteBMP(const std::filesystem::path& p) {
        static const uint8_t kBmp[] = {
            0x42, 0x4D, 0x3A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x36, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00,
            0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x13, 0x0B,
            0x00, 0x00, 0x13, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00
        };
        std::ofstream ofs(p, std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(kBmp), sizeof(kBmp));
    }
};

TEST_F(CubemapFixture, DirectConstructionProducesValidCubemap) {
    Texture cube(faces, false);
    EXPECT_TRUE(cube.IsValid());
    EXPECT_EQ(cube.GetType(), static_cast<GLenum>(GL_TEXTURE_CUBE_MAP));
}

TEST_F(CubemapFixture, GetOrLoadCubemapCacheHitReturnsSamePointer) {
    TextureManager mgr;
    auto t1 = mgr.GetOrLoadCubemap(faces, false);
    auto t2 = mgr.GetOrLoadCubemap(faces, false);
    ASSERT_NE(t1, nullptr);
    EXPECT_EQ(t1.get(), t2.get());
    EXPECT_EQ(t1->GetType(), static_cast<GLenum>(GL_TEXTURE_CUBE_MAP));
}

TEST_F(CubemapFixture, MissingFaceFileLeavesTextureInvalid) {
    auto broken = faces;
    broken[2] = std::filesystem::temp_directory_path() / "csm_cube_no_such_face.bmp";
    Texture cube(broken, false);
    EXPECT_FALSE(cube.IsValid());
}

TEST_F(CubemapFixture, GetOrLoadCubemapKeyDoesNotCollideWithPlain2DTexture) {
    // The cubemap cache key ("|"-joined face paths) must not collide with a
    // 2D texture cached under one of the individual face paths.
    TextureManager mgr;
    auto flat = mgr.GetOrLoad(faces[0], false);
    auto cube = mgr.GetOrLoadCubemap(faces, false);
    ASSERT_NE(flat, nullptr);
    ASSERT_NE(cube, nullptr);
    EXPECT_NE(flat->GetType(), cube->GetType());
}
