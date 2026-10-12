// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

#pragma once

#include <GL/glew.h>

#ifdef _WIN32 // Windows
#define NOMINMAX
#include <Windows.h>

#define OGL_DEBUGBREAK() __debugbreak()

[[msvc::forceinline]] inline bool OGLIsDebuggerPresent()
{
#ifndef NDEBUG //Debug build
    return IsDebuggerPresent(); // Checking if debugger is attached
#else // Release build
    return false;
#endif
}

#else // Linux, mac

inline void OGL_DEBUGBREAK() {}
inline bool OGLIsDebuggerPresent() { return false; }

#endif

void GLAPIENTRY SDL_GLDebugMessageCallback(GLenum source, GLenum type, GLuint id,
    GLenum severity, GLsizei length,
    const GLchar* msg, const void* data);