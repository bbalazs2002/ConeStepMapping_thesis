#include <gtest/gtest.h>
#include "Headers/Material/Material.h"

// Previously untested: operator== and Hash() never ran in any test suite.

TEST(Material, DefaultConstructedAreEqual) {
    Material a, b;
    EXPECT_TRUE(a == b);
}

TEST(Material, DifferentNameMakesUnequal) {
    Material a, b;
    a.SetName("a");
    b.SetName("b");
    EXPECT_FALSE(a == b);
}

TEST(Material, DifferentDiffuseColorMakesUnequal) {
    Material a, b;
    a.SetDiffuseColor(glm::vec3(1.f, 0.f, 0.f));
    b.SetDiffuseColor(glm::vec3(0.f, 1.f, 0.f));
    EXPECT_FALSE(a == b);
}

TEST(Material, HashIsStableForSameData) {
    Material a, b;
    a.SetName("mat");
    a.SetDiffuseColor(glm::vec3(0.2f, 0.4f, 0.6f));
    b.SetName("mat");
    b.SetDiffuseColor(glm::vec3(0.2f, 0.4f, 0.6f));
    EXPECT_EQ(Material::Hash(a), Material::Hash(b));
}

TEST(Material, HashDiffersForDifferentData) {
    Material a, b;
    a.SetName("mat_a");
    b.SetName("mat_b");
    EXPECT_NE(Material::Hash(a), Material::Hash(b));
}

TEST(Material, EqualityIgnoresNothingUnexpected) {
    // Same everything except shininess -> not equal
    Material a, b;
    a.SetShininess(16.f);
    b.SetShininess(32.f);
    EXPECT_FALSE(a == b);
}
