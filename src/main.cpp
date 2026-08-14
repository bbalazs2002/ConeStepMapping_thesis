// standard
#include <iostream>
#include <sstream>

// memory leak debug
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

// Log (only in debug)
#include "Utils/Log.h"

// GLEW
#include <GL/glew.h>

// SDL
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

// ImGui
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

// interfaces
#include "Interfaces/IGraphicsApp.h"

// application
#include "Headers/MyApp.h"

IGraphicsApp *app;

int main(int argc, char* args[]) {

    // log memory leaks
#ifdef DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // --- 1. Initialize SDL ---
    SDL_SetLogPriority(SDL_LOG_CATEGORY_ERROR, SDL_LOG_PRIORITY_ERROR);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[SDL Init] Failed: %s", SDL_GetError());
        return 1;
    }
    std::atexit(SDL_Quit);

    // --- 2. Configure OpenGL Attributes ---
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    int glVersion[2] = { 4, 5 };
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, glVersion[0]);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, glVersion[1]);

#ifdef DEBUG 
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif 

    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // --- 3. Create Window and Context ---
    SDL_Window* win = SDL_CreateWindow(
        "Cone Step Mapping Demo",
        800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    if (!win) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Window] Failed: %s", SDL_GetError());
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(win);
    if (!context) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[GL Context] Failed: %s", SDL_GetError());
        return 1;
    }

    SDL_GL_SetSwapInterval(1); // Enable VSync

    // --- 4. Initialize GLEW ---
    if (glewInit() != GLEW_OK) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[GLEW] Initialization failed.");
        return 1;
    }

    // Log OpenGL version info
    glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
    glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Running OpenGL %d.%d", glVersion[0], glVersion[1]);

    // Update window title with GL version
    std::stringstream window_title;
    window_title << "OpenGL " << glVersion[0] << "." << glVersion[1];
    SDL_SetWindowTitle(win, window_title.str().c_str());

    // --- 5. Initialize ImGui ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForOpenGL(win, context);
    ImGui_ImplOpenGL3_Init();
    // Enable docking panels
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // --- 6. Main Application Loop ---
    {
        bool quit = false;
        SDL_Event ev;
        bool showImGui = true;

        app = new MyApp(); // Your application instance
        if (!app->Init()) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[APP] Initialization failed.");
        }

        // Call resize on the app
        {
            int w, h;
            SDL_GetWindowSize(win, &w, &h);
            app->Resize(w, h);
        }

        // Set values for timer
        Uint64 lastTime = SDL_GetPerformanceCounter();
        uint64_t freq = SDL_GetPerformanceFrequency();

        while (!quit) {
            // Event Handling
            while (SDL_PollEvent(&ev)) {
                ImGui_ImplSDL3_ProcessEvent(&ev);

                bool isMouseCaptured = ImGui::GetIO().WantCaptureMouse;
                bool isKeyboardCaptured = ImGui::GetIO().WantCaptureKeyboard;

                switch (ev.type)
                {
                    case SDL_EVENT_QUIT:
                        quit = true;
                        break;
                    case SDL_EVENT_KEY_DOWN:

                        if (ev.key.key == SDLK_ESCAPE) quit = true;

                        // ALT + ENTER: Toggle Fullscreen
                        if ((ev.key.key == SDLK_RETURN) && (ev.key.mod & SDL_KMOD_ALT)) {
                            bool isFull = (SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN);
                            SDL_SetWindowFullscreen(win, !isFull);
                        }
                        // CTRL + F1: Toggle ImGui
                        if ((ev.key.key == SDLK_F1) && (ev.key.mod & SDL_KMOD_CTRL)) {
                            showImGui = !showImGui;
                        }

                        if (!isKeyboardCaptured)
                            app->KeyboardDown(ev.key);
                        break;
                    case SDL_EVENT_KEY_UP:
                        if (!isKeyboardCaptured)
                            app->KeyboardUp(ev.key);
                        break;
                    case SDL_EVENT_MOUSE_BUTTON_DOWN:
                        if (!isMouseCaptured)
                            app->MouseDown(ev.button);
                        break;
                    case SDL_EVENT_MOUSE_BUTTON_UP:
                        if (!isMouseCaptured)
                            app->MouseUp(ev.button);
                        break;
                    case SDL_EVENT_MOUSE_WHEEL:
                        if (!isMouseCaptured)
                            app->MouseWheel(ev.wheel);
                        break;
                    case SDL_EVENT_MOUSE_MOTION:
                        if (!isMouseCaptured)
                            app->MouseMove(ev.motion);
                        break;
                    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                        int w, h;
                        SDL_GetWindowSize(win, &w, &h);
                        app->Resize(w, h);
                        break;
                    default:
                        app->OtherEvent(ev);
                }

            }

            // Create update info
            Uint64 currentTime = SDL_GetPerformanceCounter();

            float deltaTime = static_cast<float>(currentTime - lastTime) / static_cast<float>(freq);
            float totalTime = static_cast<float>(currentTime) / static_cast<float>(freq);

            SUpdateInfo updateInfo{
                totalTime,
                deltaTime
            };

            // Update lastTime
            lastTime = currentTime;

            // Logic Update and Rendering
            app->Update(updateInfo);
            app->Render();

            // ImGui Rendering
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            if (showImGui) {
                app->RenderGUI();
            }

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            SDL_GL_SwapWindow(win);
        }

        app->Clean();
        delete app;
    }

    // --- 7. Shutdown ---
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(win);

    return 0;
}
