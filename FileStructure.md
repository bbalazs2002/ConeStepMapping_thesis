# Fájlstruktúra

Jelölések:
- ✅ Marad (változatlan)
- 🔧 Módosul
- 🆕 Új fájl

---

```
src/
├── main.cpp                                              ✅
│
├── Interface/
│   ├── IGraphicsApp.h                                    ✅
│   ├── IDrawable.h                                       ✅
│   ├── ICommand.h                                        🆕
│   ├── IGUIVisitor.h                                     🆕
│   ├── IGUIVisitable.h                                   🆕
│   ├── IModelRendererVisitor.h                           🆕
│   └── IModelRendererVisitable.h                         🆕
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
│   │   └── MaterialManager.h                             🆕
│   │
│   ├── Renderer/
│   │   ├── SkyboxRenderer.h                              🆕
│   │   └── AxesRenderer.h                                🆕
│   │
│   ├── GUI/
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
│   ├── Material/
│   │   └── Material.h                                    🔧
│   │
│   ├── RayMarching/
│   │   ├── IRayMarchingTechnique.h                       🆕
│   │   ├── LinearSearch.h                                🆕
│   │   └── ConeStepMapping.h                             🆕
│   │
│   └── Transform/
│       └── Transform.h                                   🔧  (volt: Transformation.h)
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
│   │   └── MaterialManager.cpp                           🆕
│   │
│   ├── Renderer/
│   │   ├── SkyboxRenderer.cpp                            🆕
│   │   └── AxesRenderer.cpp                              🆕
│   │
│   ├── GUI/
│   │   └── ImGuiVisitor.cpp                              🆕
│   │
│   ├── RendererVisitor/
│   │   └── OpenGLRendererVisitor.cpp                     🆕
│   │
│   ├── Models/
│   │   ├── ModelBase.cpp                                 🆕  (volt csak header)
│   │   ├── Model.cpp                                     🔧
│   │   ├── Mesh.cpp                                      🔧
│   │   └── RayMarchedSurface.cpp                         🔧
│   │
│   ├── Material/
│   │   └── Material.cpp                                  🆕  (volt csak header)
│   │
│   ├── RayMarching/
│   │   ├── LinearSearch.cpp                              🆕
│   │   └── ConeStepMapping.cpp                           🆕
│   │
│   └── Transform/
│       └── Transform.cpp                                 🔧  (volt: Transformation.cpp)
│
├── Utils/                                                ✅ (teljes mappa változatlan)
│   ├── Camera.h / .cpp
│   ├── CameraManipulator.h / .cpp
│   ├── GLUtils.hpp / .cpp
│   ├── Log.h
│   ├── ModelLoader.h / .cpp
│   ├── ProgramBuilder.h / .cpp
│   └── SDL_GLDebugMessageCallback.h / .cpp
│
└── Shaders/                                              ✅ (teljes mappa változatlan)
    ├── Axes/
    ├── Conemap/
    ├── Models/
    ├── Modules/
    ├── RayMarching/
    └── Skybox/
```

---

## Implementációs irányelvek

### Forward declaration szabály
Minden header csak forward declarationt használ külső osztályokra,
a teljes definíció csak a .cpp fájlban szerepel #include-dal.
Kivétel: örökölt szülőosztályok és interfészek (teljes definíció szükséges).

### Inline getterek és setterek
Minden getter és setter a headerben kerül definícióra:
```cpp
// Transform.h
inline const glm::vec3& GetLocation() const { return m_location; }
inline void SetLocation(const glm::vec3& loc) { m_location = loc; }
```
Komplex logikát tartalmazó metódusok (pl. SetHeightmap, SetInterpolateTexture)
továbbra is .cpp-ben maradnak.

### const referencia paraméterek
Minden Render() és Update() hívásban a paraméterek const referenciák:
```cpp
void OpenGLRendererVisitor::Visit(const Model& model);
void OpenGLRendererVisitor::Visit(const RayMarchedSurface& rms);
```

### GPU szinkronizáció
- CommandQueue::Execute() a MyApp::Update() legelső utasítása
- Erőforrást törlő/módosító Command-ok előtt glFenceSync() használata
- Érintett Command-ok: SetHeightmapCommand, DeleteObjectCommand,
  SetMaterialCommand, SetTechniqueCommand

### Destruktorok
- Material destruktora törli a GPU textúra handle-ket (glDeleteTextures)
- MaterialManager::Release() ellenőrzi a shared_ptr use_count-ot
- Minden OpenGL handle-t tároló osztálynak explicit destruktorra van szüksége

### Új modell típus hozzáadásának lépései
1. Új osztály létrehozása (ModelBase leszármazottja) – új fájl
2. AcceptGUIVisitor() és AcceptRendererVisitor() implementálása
3. IGUIVisitor::Visit() bővítése az új típussal
4. IModelRendererVisitor::Visit() bővítése az új típussal
5. ImGuiVisitor::Visit() implementálása
6. OpenGLRendererVisitor::Visit() implementálása
7. MyApp::Init()-ben példányosítás és SceneManager-hez adás