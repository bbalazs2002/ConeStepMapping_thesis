# File Structure

Legend:
- ✅ Unchanged
- 🔧 Modified
- 🆕 New file

---

```
src/
├── main.cpp                                              ✅
│
├── Interfaces/
│   ├── IGraphicsApp.h                                    ✅
│   ├── IDrawable.h                                       ✅
│   ├── ICommand.h                                        🆕
│   ├── IGUIVisitor.h                                     🆕
│   ├── IGUIVisitable.h                                   🆕
│   ├── IModelRendererVisitor.h                           🆕
│   ├── IModelRendererVisitable.h                         🆕
│   └── IRayMarchingTechnique.h                           🆕
│
├── Headers/
│   ├── MyApp.h                                           🔧
│   ├── Types.h                                           🔧
│   ├── config.h                                          ✅
│   │
│   ├── Command/
│   │   ├── CommandQueue.h                                🆕
│   │   ├── SetHeightmapCommand.h                         🆕
│   │   ├── SetTechniqueCommand.h                         🆕
│   │   ├── SetMaxStepsCommand.h                          🆕
│   │   ├── SetEpsilonCommand.h                           🆕
│   │   ├── SetLocationCommand.h                          🆕
│   │   ├── SetRotationCommand.h                          🆕
│   │   ├── SetScaleCommand.h                             🆕
│   │   ├── SetMaterialCommand.h                          🆕
│   │   └── DeleteObjectCommand.h                         🆕
│   │
│   ├── Manager/
│   │   ├── SceneManager.h                                🆕
│   │   ├── ShaderManager.h                               🆕
│   │   ├── MaterialManager.h                             🆕
│   │   └── TextureManager.h                              🆕
│   │
│   ├── Renderer/
│   │   ├── SkyboxRenderer.h                              🆕
│   │   └── AxesRenderer.h                                🆕
│   │
│   ├── GUIVisitor/
│   │   └── ImGuiVisitor.h                                🆕
│   │
│   ├── RendererVisitor/
│   │   └── OpenGLRendererVisitor.h                       🆕
│   │
│   ├── Model/
│   │   ├── ModelBase.h                                   🔧
│   │   ├── Model.h                                       🔧
│   │   ├── Mesh.h                                        🔧
│   │   └── RayMarchedSurface.h                           🔧
│   │
│   ├── RayMarching/
│   │   ├── LinearSearch.h                                🆕
│   │   └── ConeStepMapping.h                             🆕
│   │
│   ├── Material/
│   │   └── Material.h                                    🔧
│   │
│   ├── Texture/
│   │   └── Texture.h                                     🆕
│   │
│   └── Transform/
│       └── Transform.h                                   🔧  (was: Transformation.h)
│
├── Sources/
│   ├── MyApp.cpp                                         🔧
│   │
│   ├── Command/
│   │   ├── CommandQueue.cpp                              🆕
│   │   ├── SetHeightmapCommand.cpp                       🆕
│   │   ├── SetTechniqueCommand.cpp                       🆕
│   │   ├── SetMaxStepsCommand.cpp                        🆕
│   │   ├── SetEpsilonCommand.cpp                         🆕
│   │   ├── SetLocationCommand.cpp                        🆕
│   │   ├── SetRotationCommand.cpp                        🆕
│   │   ├── SetScaleCommand.cpp                           🆕
│   │   ├── SetMaterialCommand.cpp                        🆕
│   │   └── DeleteObjectCommand.cpp                       🆕
│   │
│   ├── Manager/
│   │   ├── SceneManager.cpp                              🆕
│   │   ├── ShaderManager.cpp                             🆕
│   │   ├── MaterialManager.cpp                           🆕
│   │   └── TextureManager.cpp                            🆕
│   │
│   ├── Renderer/
│   │   ├── SkyboxRenderer.cpp                            🆕
│   │   └── AxesRenderer.cpp                              🆕
│   │
│   ├── GUIVisitor/
│   │   └── ImGuiVisitor.cpp                              🆕
│   │
│   ├── RendererVisitor/
│   │   └── OpenGLRendererVisitor.cpp                     🆕
│   │
│   ├── Models/
│   │   ├── ModelBase.cpp                                 🆕  (was header-only)
│   │   ├── Model.cpp                                     🔧
│   │   ├── Mesh.cpp                                      🔧
│   │   └── RayMarchedSurface.cpp                         🔧
│   │
│   ├── RayMarching/
│   │   ├── LinearSearch.cpp                              🆕
│   │   └── ConeStepMapping.cpp                           🆕
│   │
│   ├── Material/
│   │   └── Material.cpp                                  🆕  (was header-only)
│   │
│   ├── Texture/
│   │   └── Texture.cpp                                   🆕
│   │
│   └── Transform/
│       └── Transform.cpp                                 🔧  (was: Transformation.cpp)
│
├── Utils/                                                ✅ (entire folder unchanged)
│   ├── Camera.h / .cpp
│   ├── CameraManipulator.h / .cpp
│   ├── GLUtils.hpp / .cpp
│   ├── Log.h
│   ├── ModelLoader.h / .cpp
│   ├── ProgramBuilder.h / .cpp
│   └── SDL_GLDebugMessageCallback.h / .cpp
│
└── Shaders/                                              ✅ (entire folder unchanged)
    ├── Axes/
    ├── Conemap/
    ├── Models/
    ├── Modules/
    ├── RayMarching/
    └── Skybox/
```

---

## Implementation Guidelines

### Forward declaration rule
Headers use forward declarations for external classes; full definitions
only in .cpp files via #include.
Exception: inherited base classes and interfaces require full definitions.

### Inline getters and setters
All getters and setters are defined in the header for potential inlining:
```cpp
// Transform.h
inline const glm::vec3& GetLocation() const { return m_location; }
inline void SetLocation(const glm::vec3& loc) { m_location = loc; m_dirty = true; }
```
Methods with complex logic (e.g. SetHeightmap, SetInterpolateTexture) remain in .cpp.

### const reference parameters
All Render() and Update() calls use const references where only reading occurs:
```cpp
void OpenGLRendererVisitor::Visit(const Model& model);
void OpenGLRendererVisitor::Visit(const RayMarchedSurface& rms);
```

### GPU synchronization
- CommandQueue::Execute() is the first call in MyApp::Update()
- Use glFenceSync() before resource-destroying/modifying Commands
- Affected Commands: SetHeightmapCommand, DeleteObjectCommand,
  SetMaterialCommand, SetTechniqueCommand

### Destructors
- Texture destructor calls glDeleteTextures (RAII)
- Material destructor is = default (shared_ptr<Texture> members handle cleanup)
- TextureManager uses weak_ptr cache – textures are freed automatically
  when all shared_ptr owners release them
- Every class owning OpenGL handles needs an explicit destructor

### Transform caching
- GetMatrix() recomputes only when m_dirty == true, returns const ref to cache
- GetWorldMatrix() is not cached – may be a bottleneck for deep hierarchies
- Parent-child hierarchy is partially implemented (see Transform.h NOTE)

### Adding a new model type (checklist)
1. Create new class extending ModelBase – new file only
2. Implement AcceptGUIVisitor() and AcceptRendererVisitor()
3. Add Visit() overload to IGUIVisitor interface
4. Add Visit() overload to IModelRendererVisitor interface
5. Implement Visit() in ImGuiVisitor
6. Implement Visit() in OpenGLRendererVisitor
7. Instantiate and add to SceneManager in MyApp::Init()