# SDL3-OpenGL-Base

A lightweight, object-oriented starter template for high-performance graphical applications. This boilerplate handles the tedious setup of SDL3 and OpenGL, allowing you to jump straight into creative coding within a structured environment.

---

## Key Features

- **Modern SDL3 Integration:** Leveraging the latest SDL3 features for window management and event handling.  
- **OpenGL 4.6 Ready:** Pre-configured graphics context with a robust debug logger.  
- **OOP Architecture:** Clean separation of concerns. `main.cpp` manages the lifecycle, while `MyApp` encapsulates your application logic (`Update`, `Render`, `Resize`, etc.).
- **Asset Management:** Includes a ready-to-use image loading utility (STB-style via SDL_image) with support for PNG, JPG, and more.  
- **3D Model Loading:** Integrated tinyobjloader support for loading `.obj` files with full `.mtl` material handling, enabling rendering of complex 3D models.  
- **Vcpkg Compatible:** Optimized for easy dependency management.

---

## Tech Stack

- **Windowing / Input:** SDL3  
- **Graphics API:** OpenGL 4.6  
- **Image Loading:** SDL3_image  
- **Model Loading:** tinyobjloader  
- **Mathematics:** GLM (OpenGL Mathematics)  
- **UI:** Dear ImGui (Optional / Integrated)  
- **Package Manager:** vcpkg

---

## Architecture

The project is structured to separate low-level initialization from high-level application logic, while maintaining a clean and scalable directory layout:

### Core Structure

- **`main.cpp`:** Initializes SDL3, creates the OpenGL 4.6 context, and manages the main event loop.  
- **`MyApp`:** Encapsulates application-specific logic with `Update()`, `Render()`, `Resize()`, etc. methods.  

### Project Layout

- **`Assets/`:** Contains static resources such as textures and 3D models (`.obj`, `.mtl`).  

- **`src/`:** Main source directory  
  - **`Headers/`:** C++ header files. Most components are header-only for simplicity and compile-time optimization.  
  - **`Interfaces/`:** Abstract interfaces defining core contracts and promoting decoupled design.  
  - **`Shaders/`:** GLSL shader programs used in the OpenGL rendering pipeline.  
  - **`Sources/`:** Implementation files (`.cpp`) for non-header-only components.  
  - **`Utils/`:** Utility modules, including:
    - Texture and shader loading helpers  
    - Camera system (`Camera`, `CameraManipulator`)  
    - Logging system (`Log`) for debug and runtime diagnostics  

### Utilities

- **Asset Loading:** UTF-8 compliant texture loading and `.obj` model loading with full material (`.mtl`) support.  
- **Rendering Support:** Predefined helpers for shader compilation and OpenGL resource management.

### Shaders

The project includes a modular shader system designed for reuse and clarity:

- **`Shaders/`:** Contains all GLSL programs used in the rendering pipeline.  

- **`Shaders/Modules/`:** Reusable GLSL modules shared across multiple shaders (e.g., common functions, material utilities).  
  - Each module is documented separately. See: **[Shader Modules README](./src/Shaders/Modules/GLSL_modules_readme.md)**  

- **Built-in Shaders:**
  - **Model Shader:** A simple built-in shader for rendering 3D models with basic material support.
  - **Skybox Shader:** Simple environment rendering for background and scene context.  
  - **Axes Shader:** Renders orientation axes at the screen center, useful for spatial reference and debugging.

---

## Dependencies

This project uses vcpkg to manage its libraries. To ensure all features (like PNG and JPEG support) work correctly, you need the following packages:

### Required Packages

- **SDL3:** Core framework for windowing and input.  
- **SDL3_image:** Extension for loading various image formats.  
- **GLEW:** OpenGL Extension Wrangler for modern GL function loading.  
- **GLM:** Mathematics library for graphics (vectors, matrices).  
- **ImGui:** Immediate mode UI for debugging and tools.  
- **tinyobjloader:** Lightweight OBJ loader with MTL material support.

---

## Installation

Run the following command in your terminal to install all necessary dependencies with the correct features:

```bash
vcpkg install sdl3 sdl3-image[png,jpeg,tiff,webp] glew glm imgui[sdl3-binding,opengl3-binding,docking-experimental] tinyobjloader
```
