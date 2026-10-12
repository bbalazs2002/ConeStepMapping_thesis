// standard
#include <iostream>
#include <sstream>
#include <string>

// memory leak debug (Windows + Debug builds only; compiled out of every
// release build, never runs on non-Windows platforms)
#if defined(_WIN32) && defined(DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define NOMINMAX
#include <Windows.h>
#endif

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

    // log memory leaks: redirect the CRT leak report to a .log file next to
    // the .exe instead of the debugger's Output window, and print only the
    // log file's path to the console.
#if defined(_WIN32) && defined(DEBUG)
    {
        char exePath[MAX_PATH];
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::string logPath(exePath);
        logPath = logPath.substr(0, logPath.find_last_of("\\/") + 1) + "memleak_report.log";

        HANDLE logHandle = CreateFileA(logPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
            nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (logHandle != INVALID_HANDLE_VALUE) {
            _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
            _CrtSetReportFile(_CRT_WARN, (_HFILE)logHandle);
            std::cout << "[DEBUG] Memory leak report: " << logPath << std::endl;
        } else {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "[DEBUG] Could not open %s for the memory leak report.", logPath.c_str());
        }

        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    }
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
    // glewExperimental is required for core-profile contexts: GLEW's classic
    // extension detection queries glGetString(GL_EXTENSIONS), which core
    // profiles don't support (only glGetStringi is valid there), so
    // glewInit() can fail even with a perfectly valid context. Common on
    // Linux/Mesa; Windows' proprietary drivers tend to be lenient enough
    // that this never surfaced there.
    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    // GLEW_ERROR_NO_GLX_DISPLAY is a known false alarm on Linux: glewInit()
    // unconditionally probes GLX (glXGetCurrentDisplay()), but if SDL
    // created the context via EGL instead (e.g. a Wayland session, which
    // this app's SDL3 build supports), there's no GLX involved at all - the
    // probe fails even though the context and every core GL function
    // pointer GLEW already loaded (via glGetString/glGetStringi, unrelated
    // to GLX) are perfectly valid. Any other error code is a real failure.
    if (glewErr != GLEW_OK && glewErr != GLEW_ERROR_NO_GLX_DISPLAY) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[GLEW] Initialization failed: %s",
            reinterpret_cast<const char*>(glewGetErrorString(glewErr)));
        return 1;
    }
    if (glewErr == GLEW_ERROR_NO_GLX_DISPLAY) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
            "[GLEW] No GLX display (expected when the context is EGL-backed, e.g. Wayland) - continuing.");
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


    // Optional: Load a custom font for larger text
    // ImFontConfig fontConfig;
    // fontConfig.SizePixels = 26.0f; // default is 13.0f
    // io.Fonts->AddFontDefault(&fontConfig);

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
