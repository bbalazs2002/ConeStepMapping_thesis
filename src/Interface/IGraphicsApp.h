#pragma once

#include "../Headers/Types.h"

/**
 * @brief Abstract interface for graphics applications.
 * Any application class must inherit from this and implement its methods.
 */
interface IGraphicsApp {
public:
    // Virtual destructor is crucial for proper cleanup of derived classes
    virtual ~IGraphicsApp() = default;

    // Called once at the start to load resources (textures, shaders, etc.)
    virtual bool Init() = 0;

    // Called every frame to handle logic, physics, and input
    virtual void Update(const SUpdateInfo&) = 0;

    // Called every frame to handle OpenGL drawing commands
    virtual void Render() = 0;

    // Called every frame to handle ImGui
    virtual void RenderGUI() = 0;

    // Called before shutdown to release allocated memory and resources
    virtual void Clean() = 0;

    // SDL event handlers
    virtual void KeyboardDown(const SDL_KeyboardEvent&) = 0;
    virtual void KeyboardUp(const SDL_KeyboardEvent&) = 0;
    virtual void MouseMove(const SDL_MouseMotionEvent&) = 0;
    virtual void MouseDown(const SDL_MouseButtonEvent&) = 0;
    virtual void MouseUp(const SDL_MouseButtonEvent&) = 0;
    virtual void MouseWheel(const SDL_MouseWheelEvent&) = 0;
    virtual void Resize(int, int) = 0;
    virtual void OtherEvent(const SDL_Event&) = 0;
};