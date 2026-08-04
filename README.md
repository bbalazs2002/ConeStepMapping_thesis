# Cone Step Mapping — Thesis Application

An interactive 3D rendering application developed as a BSc thesis project at ELTE (Eötvös Loránd University). The application implements and compares **Cone Step Mapping** against **Linear Search** ray marching for parallax occlusion mapping on heightmap-driven surfaces.

---

## What it does

- Loads heightmap textures and renders displaced surfaces via ray marching in real time
- Supports two ray marching techniques selectable at runtime: **Linear Search** and **Cone Step Mapping**
- Generates conemaps on the GPU from heightmaps for accelerated ray traversal
- Loads and renders arbitrary 3D OBJ models with full MTL material and texture support
- Provides an ImGui-based GUI for scene management, technique parameters, and debug visualization

---

## Key Features

- **Cone Step Mapping** — GPU-accelerated ray marching using precomputed cone data for fewer steps per fragment
- **Linear Search** — reference technique for direct comparison and performance benchmarking
- **Conemap generation** — on-the-fly conemap computation from loaded heightmaps
- **Scene management** — add, remove, and select `RayMarchedModel` and `Model` objects at runtime
- **Material & texture cache** — `MaterialManager` and `TextureManager` with `weak_ptr`-based garbage collection
- **Modular shader system** — GLSL modules included at compile time (Camera, Transform, Light, Color, etc.)
- **Debug visualization** — dedicated shaders for ray paths, cone geometry, step counts, hit points, and enter/exit depth
- **Command queue** — deferred OpenGL resource commands executed before each render pass, avoiding mid-frame state mutations
- **Shader hot-reload** — Ctrl+F5 recompiles and relinks all shader programs without restarting
- **Visitor pattern rendering** — `OpenGLRendererVisitor` and `ImGuiVisitor` decouple scene traversal from rendering logic

---

## Tech Stack

| Category | Library / Tool |
|---|---|
| Windowing & input | SDL3 |
| Graphics API | OpenGL 4.5 Core |
| Extension loading | GLEW |
| Mathematics | GLM |
| UI | Dear ImGui (docking branch) |
| Model loading | tinyobjloader |
| Package manager | vcpkg (manifest mode) |
| Test framework | Google Test |
| CI | GitHub Actions |

---

## Architecture

### Library split

All source files except `main.cpp` are compiled into a static library `CSM_lib`. Both the main executable and the test binaries link against it. This avoids duplicate compilation and lets tests reach any internal class without extra boilerplate.

```
src/main.cpp          → CSM_thesis.exe
src/**/*.cpp          → CSM_lib.lib  ←  tests_no_gl.exe
                                     ←  tests_gl.exe
```

### Core subsystems

| Subsystem | Description |
|---|---|
| `MyApp` | Top-level application class; owns all managers and drives the event loop |
| `SceneManager` | Owns the list of `ISceneObject` instances; dispatches visitor traversal |
| `ShaderManager` | Loads, compiles, links, and hot-reloads GLSL programs |
| `TextureManager` | Loads textures via SDL_image; `weak_ptr` cache keyed on `path\|flip` |
| `MaterialManager` | Creates and caches `Material` objects; GC via `weak_ptr` |
| `CommandQueue` | Collects deferred `ICommand` objects; executed once per `Update()` tick |
| `OpenGLRendererVisitor` | Issues all draw calls; visited by each scene object |
| `ImGuiVisitor` | Builds per-object ImGui panels; pushes commands to the queue |
| `ConemapGenerator` | Generates conemap textures from heightmaps on the GPU |

### Shader structure

```
src/Shaders/
├── Modules/          # Reusable GLSL includes (Camera, Transform, Light, Color, …)
├── RayMarching/      # Ray march vertex + fragment shaders
├── Models/           # Standard model shader + selection wireframe overlay
├── Debug/            # Five debug visualization shaders (rays, cones, steps, …)
├── Skybox/
└── Axes/
```

See [docs/GLSL_modules_readme.md](docs/GLSL_modules_readme.md) for module documentation.

---

## Building

### Prerequisites

- Visual Studio 2022 (MSVC) with C++20
- CMake ≥ 3.20
- [vcpkg](https://github.com/microsoft/vcpkg) with `VCPKG_ROOT` set in your environment

### Steps

```powershell
# 1. Install dependencies (manifest mode — reads vcpkg.json automatically)
vcpkg install --triplet x64-windows

# 2. Configure
cmake -B build `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DCMAKE_BUILD_TYPE=Release `
  -A x64

# 3. Build
cmake --build build --config Release --parallel

# 4. Run
.\build\Release\CSM_thesis.exe
```

### Building with tests

```powershell
cmake -B build `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DCMAKE_BUILD_TYPE=Release `
  -DBUILD_TESTS=ON `
  -A x64

cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

---

## Testing

The test suite is split into two executables based on OpenGL dependency:

| Target | Runner | What it covers |
|---|---|---|
| `tests_no_gl` | GitHub-hosted (`windows-latest`) | Transform, CommandQueue, MaterialManager, ModelLoader, SceneManager |
| `tests_gl` | Self-hosted (`[self-hosted, windows, gl]`) | TextureManager, Mesh build, scene integration with live GL context |

GL tests initialize a hidden 1×1 SDL3 window with an OpenGL 4.5 Core context before running. If the driver reports < 4.5, the suite exits with code 0 (skip, not failure).

Test plan and expected results: [docs/Testing/TestingPlan.md](docs/Testing/TestingPlan.md)

### CI pipeline

Two GitHub Actions jobs run on every push to `main`:

1. **No-GL Tests** — clones and bootstraps vcpkg, installs all dependencies, builds only `tests_no_gl`, runs CTest with label `NoGL`
2. **GL Tests** — runs on a self-hosted Windows runner with GPU; requires `VCPKG_ROOT` set in the runner environment (`.env` file in the runner directory)

---

## Project layout

```
.
├── src/
│   ├── Headers/          # Class declarations (most components are header-only)
│   ├── Interfaces/       # Pure abstract interfaces (IGraphicsApp, ICommand, …)
│   ├── Shaders/          # GLSL programs and reusable modules
│   ├── Sources/          # .cpp implementation files
│   ├── Utils/            # ModelLoader, TextureLoader, GLUtils, Log, …
│   └── main.cpp
├── tests/
│   ├── no_gl/            # GTest suite — no OpenGL required
│   └── gl/               # GTest suite — requires live GL context
├── docs/
│   ├── Architecture.md   # Full class inventory and design decisions
│   ├── Testing/          # Testing plan
│   └── *.md              # Debug layout, GLSL modules, implementation notes
├── Assets/               # Heightmaps, 3D models, textures (not committed — gitignored)
├── CMakeLists.txt
└── vcpkg.json
```

---

## Documentation

| Document | Contents |
|---|---|
| [docs/Architecture.md](docs/Architecture.md) | Complete class hierarchy, member tables, design invariants |
| [docs/GLSL_modules_readme.md](docs/GLSL_modules_readme.md) | Shader module API and include conventions |
| [docs/Debug.md](docs/Debug.md) | Debug visualization system and SSBO layout |
| [docs/ImplementationDecisions.md](docs/ImplementationDecisions.md) | Key design choices and their rationale |
| [docs/Testing/TestingPlan.md](docs/Testing/TestingPlan.md) | Black-box, unit, integration, memory, and performance test plan |
