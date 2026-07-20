#pragma once

#include <memory>
#include <GL/glew.h>
#include "Utils/GLUtils.hpp"

class Texture;
interface ICamera;

class SkyboxRenderer {
public:
    SkyboxRenderer(GLuint programID, std::shared_ptr<Texture> cubemap);
    ~SkyboxRenderer();

    SkyboxRenderer(const SkyboxRenderer&)            = delete;
    SkyboxRenderer& operator=(const SkyboxRenderer&) = delete;

    void Render(const ICamera& cam);

private:
    GLuint                   m_programID = 0;
    std::shared_ptr<Texture> m_cubemap;
    OGLObject                m_gpuObject = {};
};
