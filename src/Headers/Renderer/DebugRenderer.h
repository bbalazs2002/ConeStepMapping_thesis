#pragma once
#include <GL/glew.h>

struct RayMarchDebugState;
interface ICamera;

class DebugRenderer {
public:
    DebugRenderer(GLuint progRay, GLuint progEnterExit,
                  GLuint progSteps, GLuint progCones, GLuint progHit);
    ~DebugRenderer();

    void Render(const RayMarchDebugState& state, const ICamera& camera);

private:
    GLuint m_progRay;
    GLuint m_progEnterExit;
    GLuint m_progSteps;
    GLuint m_progCones;
    GLuint m_progHit;
};
