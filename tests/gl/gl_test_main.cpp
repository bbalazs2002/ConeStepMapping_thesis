// Custom GTest main: initializes SDL3 + OpenGL 4.5 context before running any tests,
// and tears it down cleanly afterwards.
// All test files in the GL suite can assume a valid GL context from the first TEST().

#include <gtest/gtest.h>
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <cstdio>

int main(int argc, char** argv) {
    // --- SDL init ---
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "[gl_test_main] SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // OpenGL 4.5 Core Profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Hidden 1×1 window — no physical display required on the self-hosted runner
    SDL_Window* window = SDL_CreateWindow(
        "CSM_thesis GL Tests", 1, 1,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN
    );
    if (!window) {
        fprintf(stderr, "[gl_test_main] SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    if (!ctx) {
        fprintf(stderr, "[gl_test_main] SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "[gl_test_main] glewInit failed\n");
        SDL_GL_DestroyContext(ctx);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Verify OpenGL 4.5
    int glMajor = 0, glMinor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &glMajor);
    glGetIntegerv(GL_MINOR_VERSION, &glMinor);
    if (glMajor < 4 || (glMajor == 4 && glMinor < 5)) {
        fprintf(stderr,
            "[gl_test_main] OpenGL 4.5 required, got %d.%d — skipping GL tests.\n",
            glMajor, glMinor);
        SDL_GL_DestroyContext(ctx);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0; // exit 0 so CI doesn't fail on unsupported hardware
    }

    fprintf(stdout, "[gl_test_main] Running on OpenGL %d.%d\n", glMajor, glMinor);

    // Run all GL tests
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

    // Teardown
    SDL_GL_DestroyContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return result;
}
