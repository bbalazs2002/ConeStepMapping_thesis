#pragma once

#include <memory>
#include <GL/glew.h>

class Texture;

class ConemapGenerator {
public:
    ConemapGenerator(GLuint originalProgramID, GLuint conservativeProgramID);

    void SetConservative(bool conservative) { m_conservative = conservative; }
    bool IsConservative() const             { return m_conservative; }

    std::shared_ptr<Texture> Generate(const Texture& heightmap);

    // Returns the GL texture ID of the most recently generated conemap (0 if none).
    GLuint GetLastConemapID() const;

private:
    GLuint m_originalProgramID;
    GLuint m_conservativeProgramID;
    bool   m_conservative = false;

    // Retained so the UI can display a preview even after the conemap ownership
    // transfers to the RayMarchedModel via SetHeightmap().
    std::shared_ptr<Texture> m_lastConemap;
};
