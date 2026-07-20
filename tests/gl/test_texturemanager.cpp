#include <gtest/gtest.h>
#include "Headers/Manager/TextureManager.h"
#include "Headers/Texture/Texture.h"
#include <filesystem>
#include <fstream>
#include <cstdint>

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

// UT-13: weak_ptr GC — after dropping all refs, next GetOrLoad creates a new object
TEST_F(TextureManagerFixture, ExpiredCacheMissCreatesNewObject) {
    TextureManager mgr;
    Texture* rawPtr = nullptr;
    {
        auto t = mgr.GetOrLoad(tex2, false);
        ASSERT_NE(t, nullptr);
        rawPtr = t.get();
    } // external ref dropped -> weak_ptr may expire

    auto t2 = mgr.GetOrLoad(tex2, false);
    ASSERT_NE(t2, nullptr);
    EXPECT_NE(t2.get(), rawPtr);
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
