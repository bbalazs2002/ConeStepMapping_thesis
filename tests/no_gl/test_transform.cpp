#include <gtest/gtest.h>
#include "Headers/Transform/Transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

// UT-05
TEST(Transform, DefaultIsIdentity) {
    Transform t;
    EXPECT_EQ(t.GetMatrix(), glm::identity<glm::mat4>());
}

// UT-02
TEST(Transform, SetLocationMarksDirty) {
    Transform t;
    t.GetMatrix();
    EXPECT_FALSE(t.IsDirty());
    t.SetLocation({ 1.f, 0.f, 0.f });
    EXPECT_TRUE(t.IsDirty());
}

TEST(Transform, SetRotationMarksDirty) {
    Transform t;
    t.GetMatrix();
    EXPECT_FALSE(t.IsDirty());
    t.SetRotation(glm::identity<glm::quat>());
    EXPECT_TRUE(t.IsDirty());
}

TEST(Transform, SetScaleMarksDirty) {
    Transform t;
    t.GetMatrix();
    EXPECT_FALSE(t.IsDirty());
    t.SetScale({ 2.f, 2.f, 2.f });
    EXPECT_TRUE(t.IsDirty());
}

// UT-01: cache — GetMatrix() returns the same reference on second call
TEST(Transform, CacheReturnsSameReferenceWhenClean) {
    Transform t;
    t.SetLocation({ 3.f, 1.f, 2.f });
    const glm::mat4& m1 = t.GetMatrix();
    EXPECT_FALSE(t.IsDirty());
    const glm::mat4& m2 = t.GetMatrix();
    EXPECT_EQ(&m1, &m2);
}

// Location applied to column 3
TEST(Transform, LocationAppliedToMatrix) {
    Transform t;
    t.SetLocation({ 5.f, 3.f, -2.f });
    glm::mat4 m = t.GetMatrix();
    EXPECT_FLOAT_EQ(m[3][0], 5.f);
    EXPECT_FLOAT_EQ(m[3][1], 3.f);
    EXPECT_FLOAT_EQ(m[3][2], -2.f);
    EXPECT_FLOAT_EQ(m[3][3], 1.f);
}

// Scale applied to diagonal
TEST(Transform, ScaleAppliedToMatrix) {
    Transform t;
    t.SetScale({ 2.f, 3.f, 4.f });
    glm::mat4 m = t.GetMatrix();
    EXPECT_FLOAT_EQ(m[0][0], 2.f);
    EXPECT_FLOAT_EQ(m[1][1], 3.f);
    EXPECT_FLOAT_EQ(m[2][2], 4.f);
}

// UT-06: +90° around Y maps +X to -Z (GLM right-hand convention)
TEST(Transform, RotationY90MapsXToNegZ) {
    Transform t;
    t.SetRotation(glm::angleAxis(glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f)));
    glm::mat4 m = t.GetMatrix();
    glm::vec4 result = m * glm::vec4(1.f, 0.f, 0.f, 0.f);
    EXPECT_NEAR(result.x,  0.f, 1e-5f);
    EXPECT_NEAR(result.y,  0.f, 1e-5f);
    EXPECT_NEAR(result.z, -1.f, 1e-5f);
}

// UT-03: world matrix without parent == local matrix
TEST(Transform, WorldMatrixEqualsLocalWithNoParent) {
    Transform t;
    t.SetLocation({ 1.f, 2.f, 3.f });
    t.SetScale({ 2.f, 2.f, 2.f });
    EXPECT_EQ(t.GetWorldMatrix(), t.GetMatrix());
}

// UT-04: parent translation propagates into child world matrix
TEST(Transform, WorldMatrixCombinesParentTranslation) {
    Transform parent;
    parent.SetLocation({ 10.f, 0.f, 0.f });

    Transform child;
    child.SetLocation({ 1.f, 0.f, 0.f });
    child.SetParent(&parent);

    glm::mat4 world = child.GetWorldMatrix();
    EXPECT_NEAR(world[3][0], 11.f, 1e-4f);
    EXPECT_NEAR(world[3][1],  0.f, 1e-4f);
    EXPECT_NEAR(world[3][2],  0.f, 1e-4f);
}

// Combined TRS: translation + rotation + scale
TEST(Transform, CombinedTRSIsCorrect) {
    Transform t;
    t.SetLocation({ 1.f, 0.f, 0.f });
    t.SetScale({ 2.f, 2.f, 2.f });
    // No rotation — result should translate a unit point by (1,0,0) and scale by 2
    glm::vec4 result = t.GetMatrix() * glm::vec4(1.f, 0.f, 0.f, 1.f);
    EXPECT_NEAR(result.x, 3.f, 1e-4f); // 2*1 + 1
    EXPECT_NEAR(result.y, 0.f, 1e-4f);
    EXPECT_NEAR(result.z, 0.f, 1e-4f);
}
