#include "MyApp.h"

void MyApp::KeyboardDown(const SDL_KeyboardEvent& key)
{
    if (key.repeat == 0) {
        if (key.key == SDLK_F5 && (key.mod & SDL_KMOD_CTRL))
            m_shaderManager.ReloadAll();
    }
    m_cameraManipulator.KeyboardDown(key);
}

void MyApp::KeyboardUp(const SDL_KeyboardEvent& key)
{
    m_cameraManipulator.KeyboardUp(key);
}

void MyApp::MouseMove(const SDL_MouseMotionEvent& mouse)
{
    m_cameraManipulator.MouseMove(mouse);
}

void MyApp::MouseDown(const SDL_MouseButtonEvent&) {}
void MyApp::MouseUp  (const SDL_MouseButtonEvent&) {}

void MyApp::MouseWheel(const SDL_MouseWheelEvent& wheel)
{
    m_cameraManipulator.MouseWheel(wheel);
}

void MyApp::Resize(int w, int h)
{
    glViewport(0, 0, w, h);
    m_camera.SetAspect(static_cast<float>(w) / h);
    m_windowSize = { w, h };
}

void MyApp::OtherEvent(const SDL_Event&) {}