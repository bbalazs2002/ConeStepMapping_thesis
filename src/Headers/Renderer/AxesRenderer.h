#pragma once

#include <GL/glew.h>

interface ICamera;

class AxesRenderer {
public:
    explicit AxesRenderer(GLuint programID);
    ~AxesRenderer();

    AxesRenderer(const AxesRenderer&)            = delete;
    AxesRenderer& operator=(const AxesRenderer&) = delete;

    void Render(const ICamera& cam);

private:
    GLuint m_programID = 0;
};
