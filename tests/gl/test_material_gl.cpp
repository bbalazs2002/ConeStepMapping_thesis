#include <gtest/gtest.h>
#include "Headers/Manager/ShaderManager.h"
#include "Headers/Material/Material.h"
#include "Headers/Texture/Texture.h"
#include <GL/glew.h>

// Previously untested: UploadMaterialToShader / ClearMaterialFromShader never
// ran against a real linked program in any test suite.

namespace {

GLuint CompileModelProgram() {
    static ShaderManager mgr;
    static GLuint prog = mgr.Load("model_for_material_test", {
        { GL_VERTEX_SHADER,   "src/Shaders/Models/Vert_Model.vert" },
        { GL_FRAGMENT_SHADER, "src/Shaders/Models/Frag_Model.frag" }
    });
    return prog;
}

} // namespace

TEST(MaterialGL, UploadWithoutTexturesDoesNotError) {
    GLuint prog = CompileModelProgram();
    ASSERT_NE(prog, 0u);

    auto mat = std::make_shared<Material>();
    mat->SetDiffuseColor(glm::vec3(0.5f, 0.3f, 0.1f));

    glUseProgram(prog);
    while (glGetError() != GL_NO_ERROR) {} // drain any pre-existing error

    Material::UploadMaterialToShader(prog, mat);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    glUseProgram(0);
}

TEST(MaterialGL, UploadWithDiffuseTextureBindsUnit) {
    GLuint prog = CompileModelProgram();
    ASSERT_NE(prog, 0u);

    auto tex = std::make_shared<Texture>(4, 4, GL_RGBA8);
    ASSERT_TRUE(tex->IsValid());

    auto mat = std::make_shared<Material>();
    mat->SetDiffuseTex(tex);

    glUseProgram(prog);
    while (glGetError() != GL_NO_ERROR) {}

    GLuint targets[4] = { 0, 1, 2, 3 };
    Material::UploadMaterialToShader(prog, mat, targets);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    GLint boundTex = 0;
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);
    EXPECT_EQ(static_cast<GLuint>(boundTex), tex->GetID());

    glUseProgram(0);
}

TEST(MaterialGL, UploadWithAllFourTextureTypesBindsAllUnits) {
    // The prior test only ever set a diffuse texture, leaving the
    // specular/emission/normal texID(...) != 0 branches unexercised.
    GLuint prog = CompileModelProgram();
    ASSERT_NE(prog, 0u);

    auto diffuse  = std::make_shared<Texture>(4, 4, GL_RGBA8);
    auto specular = std::make_shared<Texture>(4, 4, GL_RGBA8);
    auto emission = std::make_shared<Texture>(4, 4, GL_RGBA8);
    auto normal   = std::make_shared<Texture>(4, 4, GL_RGBA8);

    auto mat = std::make_shared<Material>();
    mat->SetDiffuseTex(diffuse);
    mat->SetSpecularTex(specular);
    mat->SetEmissionTex(emission);
    mat->SetNormalTex(normal);

    glUseProgram(prog);
    while (glGetError() != GL_NO_ERROR) {}

    GLuint targets[4] = { 0, 1, 2, 3 };
    Material::UploadMaterialToShader(prog, mat, targets);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    auto boundAt = [](GLuint unit) {
        GLint id = 0;
        glActiveTexture(GL_TEXTURE0 + unit);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &id);
        return static_cast<GLuint>(id);
    };
    EXPECT_EQ(boundAt(0), diffuse->GetID());
    EXPECT_EQ(boundAt(1), specular->GetID());
    EXPECT_EQ(boundAt(2), emission->GetID());
    EXPECT_EQ(boundAt(3), normal->GetID());

    glUseProgram(0);
}

TEST(MaterialGL, ClearUnbindsPreviouslyBoundTargets) {
    GLuint prog = CompileModelProgram();
    ASSERT_NE(prog, 0u);

    auto tex = std::make_shared<Texture>(4, 4, GL_RGBA8);
    auto mat = std::make_shared<Material>();
    mat->SetDiffuseTex(tex);

    glUseProgram(prog);
    GLuint targets[4] = { 0, 1, 2, 3 };
    Material::UploadMaterialToShader(prog, mat, targets);

    Material::ClearMaterialFromShader();
    while (glGetError() != GL_NO_ERROR) {}

    GLint boundTex = -1;
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);
    EXPECT_EQ(boundTex, 0);

    glUseProgram(0);
}

TEST(MaterialGL, SecondUploadClearsFirstTargets) {
    // ClearMaterialFromShader is invoked internally at the start of every
    // UploadMaterialToShader call — verify the *first* material's texture
    // unit is released once a second material is uploaded on top of it.
    GLuint prog = CompileModelProgram();
    ASSERT_NE(prog, 0u);

    auto tex1 = std::make_shared<Texture>(4, 4, GL_RGBA8);
    auto mat1 = std::make_shared<Material>();
    mat1->SetDiffuseTex(tex1);

    glUseProgram(prog);
    GLuint targets[4] = { 0, 1, 2, 3 };
    Material::UploadMaterialToShader(prog, mat1, targets);

    // Second material with no textures at all — its upload should still
    // clear unit 0 (left bound by mat1) before doing (nothing) itself.
    auto mat2 = std::make_shared<Material>();
    Material::UploadMaterialToShader(prog, mat2, targets);

    GLint boundTex = -1;
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);
    EXPECT_EQ(boundTex, 0);

    glUseProgram(0);
}
