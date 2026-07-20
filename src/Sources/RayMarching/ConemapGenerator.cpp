#include "Headers/RayMarching/ConemapGenerator.h"
#include "Headers/Texture/Texture.h"

#include <GL/glew.h>

ConemapGenerator::ConemapGenerator(GLuint originalProgramID, GLuint conservativeProgramID)
    : m_originalProgramID(originalProgramID)
    , m_conservativeProgramID(conservativeProgramID)
{
}

std::shared_ptr<Texture> ConemapGenerator::Generate(const Texture& heightmap)
{
    GLint width = 0, height = 0;
    glGetTextureLevelParameteriv(heightmap.GetID(), 0, GL_TEXTURE_WIDTH,  &width);
    glGetTextureLevelParameteriv(heightmap.GetID(), 0, GL_TEXTURE_HEIGHT, &height);
    if (width <= 0 || height <= 0)
        return nullptr;

    auto conemap = std::make_shared<Texture>(width, height, GL_RGBA8);

    GLuint prog = m_conservative ? m_conservativeProgramID : m_originalProgramID;
    glUseProgram(prog);

    // Bind heightmap as sampler on unit 0, tell the uniform which unit to use
    glBindTextureUnit(0, heightmap.GetID());
    glUniform1i(glGetUniformLocation(prog, "inputImage"), 0);

    // Bind output conemap as writable image on binding 0 (matching layout in shader)
    glBindImageTexture(0, conemap->GetID(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    glUniform1i(glGetUniformLocation(prog, "width"),  width);
    glUniform1i(glGetUniformLocation(prog, "height"), height);

    glDispatchCompute((width + 15) / 16, (height + 15) / 16, 1);

    // Ensure the image writes are visible to subsequent texture reads
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    glUseProgram(0);

    m_lastConemap = conemap;
    return conemap;
}

GLuint ConemapGenerator::GetLastConemapID() const
{
    return m_lastConemap ? m_lastConemap->GetID() : 0;
}
