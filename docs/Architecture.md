# ConeStepMapping – Architecture Reference

> Source of truth: `ConeStepMapping_ClassDiagram_v14.drawio`  
> Design rationale: `temp/session.md`, `temp/design_review.md`

---

## Class Inventory

### Notation
| Symbol | Meaning |
|---|---|
| `(+,+)` | public getter + public setter |
| `(+,#)` | public getter + protected setter |
| `(+,!)` | public getter, no setter |
| `<u>underline</u>` | static member |
| `◆──` | Composition (filled diamond) |
| `◇──` | Aggregation (hollow diamond) |
| `──▷` | Realization (interface) |
| `──▶` | Inheritance |

---

## Application core

### `IGraphicsApp` *(interface)*
| Member | Signature |
|---|---|
| + | `Init() : bool` |
| + | `Update(info : SUpdateInfo)` |
| + | `Render()` |
| + | `RenderGUI()` |
| + | `Clean()` |
| + | `KeyboardDown(e : SDL_KeyboardEvent)` |
| + | `KeyboardUp(e : SDL_KeyboardEvent)` |
| + | `MouseMove(e : SDL_MouseMotionEvent)` |
| + | `MouseDown(e : SDL_MouseButtonEvent)` |
| + | `MouseUp(e : SDL_MouseButtonEvent)` |
| + | `MouseWheel(e : SDL_MouseWheelEvent)` |
| + | `Resize(w : int, h : int)` |
| + | `OtherEvent(e : SDL_Event)` |

### `MyApp` *(concrete)* — realizes `IGraphicsApp`

GL-kontextustól függő tagok (`unique_ptr`) `nullptr`-ek addig, amíg `Init()` el nem fut.

| Member | Signature |
|---|---|
| # | `m_commandQueue : CommandQueue` |
| # | `m_sceneManager : SceneManager` |
| # | `m_shaderManager : ShaderManager` |
| # | `m_materialManager : MaterialManager` |
| # | `m_textureManager : TextureManager` |
| # | `m_skyboxRenderer : unique_ptr<SkyboxRenderer>` |
| # | `m_axesRenderer : unique_ptr<AxesRenderer>` |
| # | `m_rendererVisitor : unique_ptr<OpenGLRendererVisitor>` |
| # | `m_techniques : map<string, shared_ptr<IRayMarchingTechnique>>` |
| # | `m_imguiVisitor : ImGuiVisitor` |
| # | `m_camera : Camera` |
| # | `m_cameraManipulator : CameraManipulator` |
| `(+,#)` | `m_windowSize : ivec2` |
| `(+,!)` | `m_elapsedTime : float` |
| + | `Init() : bool` |
| + | `Update(info : SUpdateInfo)` |
| + | `Render()` |
| + | `RenderGUI()` |
| + | `Clean()` |
| # | `SetupDebugCallback()` |

**Init() sorrendje:** GL kontextus létrehozása → `ShaderManager::Load()` hívások → technikák konstruálása és `m_techniques`-be regisztrálása → `unique_ptr` tagok konstruálása → jelenet feltöltése.

**`m_imguiVisitor`** értékelése: `ImGuiVisitor(m_commandQueue)` — GL kontextustól független, ezért sima tagváltozó (nem `unique_ptr`). Az `ImGuiVisitor` kapja az `ICommandQueue*`-t, soha nem a konkrét `CommandQueue*`-t.

---

## Command system

### `ICommandQueue` *(interface)*
| Member | Signature |
|---|---|
| + | `Push(cmd : unique_ptr<ICommand>) : void` |

### `ICommand` *(interface)*
| Member | Signature |
|---|---|
| + | `Execute() : void` |

### `CommandQueue` *(manager)* — realizes `ICommandQueue`
| Member | Signature |
|---|---|
| - | `m_queue : vector<unique_ptr<ICommand>>` |
| + | `Push(cmd : unique_ptr<ICommand>) : void` |
| + | `Execute() : void` |
| + | `Clear() : void` |

**Invariant:** `Execute()` is the first call in `MyApp::Update()` — ensures no OpenGL resource is touched during an active render pass. Must call `glFenceSync()` before resource-modifying commands. After `Execute()`, `unique_ptr`s are destroyed automatically.

### `SetHeightmapCommand` *(concrete ICommand)*
| Member | Signature |
|---|---|
| - | `m_surface : RayMarchedSurface*` |
| - | `m_path : string` |
| + | `Execute() : void` |

### `SetTechniqueCommand` *(concrete ICommand)*
| Member | Signature |
|---|---|
| - | `m_surface : RayMarchedSurface*` |
| - | `m_technique : shared_ptr<IRayMarchingTechnique>` |
| + | `Execute() : void` |

**`Execute()`** calls `m_surface->SetTechnique(m_technique)` — never assigns `m_technique` directly.

### `DeleteObjectCommand` *(concrete ICommand)*
| Member | Signature |
|---|---|
| - | `m_sceneManager : SceneManager*` |
| - | `m_target : shared_ptr<IDrawable>` |
| + | `Execute() : void` |

---

## Managers

### `ShaderManager`
| Member | Signature |
|---|---|
| - | `m_programs : map<string, GLuint>` |
| + | `Load(name : string, stages : vector<ShaderStage>) : GLuint` |
| + | `Get(name : string) : GLuint` |
| + | `ReloadAll() : void` |
| + | `DeleteAll() : void` |

### `SceneManager`
| Member | Signature |
|---|---|
| - | `m_drawables : vector<shared_ptr<IDrawable>>` |
| + | `Add(d : shared_ptr<IDrawable>) : void` |
| + | `Remove(d : shared_ptr<IDrawable>) : void` |
| + | `Update(info : SUpdateInfo) : void` |
| + | `Render(v : IModelRendererVisitor&) : void` |
| + | `RenderGUI(v : IGUIVisitor&) : void` |
| + | `Clear() : void` |

**Ownership:** `SceneManager` is the primary owner of scene objects. `MyApp` creates them via `make_shared<ConcreteType>(...)` and calls `Add()`. `MyApp` may retain its own `shared_ptr` to objects it needs to reference later (e.g. for ImGui controls). The object stays alive as long as at least one `shared_ptr` holds it — this guarantees that `DeleteObjectCommand` keeps the target alive until `Execute()` runs and GL resources are safely freed after GPU sync.

### `MaterialManager`
| Member | Signature |
|---|---|
| - | `m_materials : unordered_map<size_t, weak_ptr<Material>>` |
| + | `GetOrCreate(mat : Material) : shared_ptr<Material>` |
| + | `CollectGarbage() : void` |

Key: `Material::Hash()`. Cache uses `weak_ptr` — manager does not prevent destruction.

### `TextureManager`
| Member | Signature |
|---|---|
| - | `m_cache : unordered_map<string, weak_ptr<Texture>>` |
| + | `GetOrLoad(path : path, flip : bool) : shared_ptr<Texture>` |
| + | `CollectGarbage() : void` |
| + | `GetCachedCount() : size_t` |

Key: file path string. Cache uses `weak_ptr`.

### `SkyboxRenderer` *(RAII)*
| Member | Signature |
|---|---|
| - | `m_programID : GLuint` |
| - | `m_textureID : GLuint` |
| - | `m_gpuObject : OGLObject` |
| + | `SkyboxRenderer(sm : ShaderManager&)` *(constructor — loads GL resources)* |
| + | `~SkyboxRenderer()` *(destructor — frees GL resources)* |
| + | `Render(cam : ICamera&) : void` |

### `AxesRenderer` *(RAII)*
| Member | Signature |
|---|---|
| - | `m_programID : GLuint` |
| + | `AxesRenderer(sm : ShaderManager&)` *(constructor — loads GL resources)* |
| + | `~AxesRenderer()` *(destructor — frees GL resources)* |
| + | `Render(cam : ICamera&) : void` |

---

## Camera

### `ICamera` *(interface)*
| Member | Signature |
|---|---|
| + | `GetViewProj() : mat4` |
| + | `GetEye() : vec3` |
| + | `GetAt() : vec3` |
| + | `SetAspect(aspect : float) : void` |

### `Camera` *(concrete)* — realizes `ICamera`
| Member | Signature |
|---|---|
| `(+,!)` | `m_eye : vec3` |
| `(+,!)` | `m_at : vec3` |
| `(+,!)` | `m_up : vec3` |
| `(+,+)` | `m_aspect : float` |
| `(+,+)` | `m_fov : float` |
| + | `GetViewProj() : mat4` |
| + | `GetEye() : vec3` |
| + | `GetAt() : vec3` |
| + | `SetView(eye, at, up : vec3) : void` |
| + | `SetAspect(aspect : float) : void` |

---

## GUI Visitor

### `IGUIVisitor` *(interface)*
| Member | Signature |
|---|---|
| + | `Visit(target : Model&) : void` |
| + | `Visit(target : RayMarchedSurface&) : void` |

### `IGUIVisitable` *(interface)*
| Member | Signature |
|---|---|
| + | `AcceptGUIVisitor(v : IGUIVisitor&) : void` |

### `ImGuiVisitor` *(concrete)* — realizes `IGUIVisitor`
| Member | Signature |
|---|---|
| - | `m_commandQueue : ICommandQueue*` |
| + | `Visit(target : Model&) : void` |
| + | `Visit(target : RayMarchedSurface&) : void` |
| - | `VisitModelBase(target : ModelBase&) : void` |

**Invariant:** never modifies objects directly — creates `ICommand` instances and pushes to `ICommandQueue`. Depends on the interface, not the concrete `CommandQueue`.

---

## Renderer Visitor

### `IModelRendererVisitor` *(interface)*
| Member | Signature |
|---|---|
| + | `Visit(target : const Model&) : void` |
| + | `Visit(target : const RayMarchedSurface&) : void` |

### `IModelRendererVisitable` *(interface)*
| Member | Signature |
|---|---|
| + | `AcceptRendererVisitor(v : IModelRendererVisitor&) : void` |

### `OpenGLRendererVisitor` *(concrete, RAII)* — realizes `IModelRendererVisitor`
| Member | Signature |
|---|---|
| - | `m_programID : GLuint` |
| - | `m_camera : const ICamera*` |
| + | `OpenGLRendererVisitor(sm : ShaderManager&, cam : const ICamera&)` *(constructor — loads GL resources)* |
| + | `~OpenGLRendererVisitor()` *(destructor — frees GL resources)* |
| + | `Visit(target : const Model&) : void` |
| + | `Visit(target : const RayMarchedSurface&) : void` |
| - | `VisitModelBase(target : const ModelBase&) : void` |

**`VisitModelBase()`** calls `target.GetProgramID()` virtually — works correctly for both `Model` and `RayMarchedSurface` without knowing the concrete type. Camera data (`viewProj`, `cameraPos`) is read live from `m_camera` in every `Visit()` call — no manual sync needed.

---

## Scene hierarchy

### `IDrawable` *(interface)*
| Member | Signature |
|---|---|
| + | `Render(v : IModelRendererVisitor&) : void` |
| + | `Update(info : SUpdateInfo) : void` |
| + | `~IDrawable() = default` *(virtual destructor)* |

### `ModelBase` *(abstract)* — realizes `IDrawable`, `IGUIVisitable`, `IModelRendererVisitable`
| Member | Signature |
|---|---|
| `(+,+)` | `m_name : string` |
| `(+,+)` | `m_show : bool` |
| `(+,+)` | `m_drawMode : int` |
| `(+,#)` | `m_transform : Transform` |
| `(+,!)` | `m_deleteMarker : bool` |
| + | `GetProgramID() : GLuint` *(pure virtual)* |
| + | `Render(v : IModelRendererVisitor&) : void` |
| + | `Update(info : SUpdateInfo) : void` |
| + | `GetTransform() : Transform&` |
| + | `MarkForDeletion() : void` |
| + | `IsMarkedForDeletion() : bool` |
| + | `AcceptGUIVisitor(v : IGUIVisitor&) : void` |
| + | `AcceptRendererVisitor(v : IModelRendererVisitor&) : void` |

**`m_programID` nem tagváltozó** — `GetProgramID()` pure virtual, leszármazottak implementálják.

### `Transform` *(concrete)*
| Member | Signature |
|---|---|
| `(+,+)` | `m_location : vec3` |
| `(+,+)` | `m_rotation : quat` |
| `(+,+)` | `m_scale : vec3` |
| `(+,+)` | `m_parent : Transform*` |
| + | `GetMatrix() : const mat4&` *(dirty-flag cached)* |
| + | `GetWorldMatrix() : mat4` |
| + | `SetLocation(loc : vec3) : void` |
| + | `SetRotation(rot : quat) : void` |
| + | `SetScale(scale : vec3) : void` |
| + | `SetParent(parent : Transform*) : void` |

### `Model` *(abstract)* — inherits `ModelBase`
| Member | Signature |
|---|---|
| # | `m_programID : GLuint` |
| `(+,+)` | `m_objPath : string` |
| `(+,+)` | `m_wireframe : bool` |
| + | `GetProgramID() : GLuint override` *(returns m_programID)* |
| + | `Update(info : SUpdateInfo) : void` |
| + | `SetObjPath(path : string) : void` |
| + | `CleanGeometry() : void` |
| + | `AcceptGUIVisitor(v : IGUIVisitor&) : void` |
| + | `AcceptRendererVisitor(v : IModelRendererVisitor&) : void` |

### `Mesh` *(util)*
| Member | Signature |
|---|---|
| - | `m_GPU : OGLObject` |
| `(+,+)` | `m_material : shared_ptr<Material>` |
| + | `Build(mesh : MeshObject) : void` |
| + | `Render(p : MeshRenderParams) : void` |
| + | `GetVAO() : GLuint` |
| + | `GetVertexCount() : GLsizei` |

### `Material` *(util)*
| Member | Signature |
|---|---|
| `(+,+)` | `m_name : string` |
| `(+,+)` | `m_diffuseColor : vec3` |
| `(+,+)` | `m_specularColor : vec3` |
| `(+,+)` | `m_ambientColor : vec3` |
| `(+,+)` | `m_shininess : float` |
| `(+,+)` | `m_diffuseTex : shared_ptr<Texture>` |
| `(+,+)` | `m_specularTex : shared_ptr<Texture>` |
| `(+,+)` | `m_emissionTex : shared_ptr<Texture>` |
| `(+,+)` | `m_normalTex : shared_ptr<Texture>` |
| `<u>+</u>` | `UploadMaterialToShader(prog : GLuint, mat : shared_ptr<Material>, targets : GLuint[4]) : void` |
| `<u>+</u>` | `UploadMaterialToShader(prog : GLuint, mat : shared_ptr<Material>) : void` |
| `<u>+</u>` | `ClearMaterialFromShader() : void` |
| + | `operator==(other : const Material&) : bool` |
| `<u>+</u>` | `Hash(mat : const Material&) : size_t` |

**Invariant:** destructor `= default` — `shared_ptr<Texture>` members handle GPU cleanup.

### `Texture` *(RAII)*
| Member | Signature |
|---|---|
| - | `m_id : GLuint` |
| + | `GetID() : GLuint` |
| + | `IsValid() : bool` |

**Invariant:** non-copyable, movable. Destructor calls `glDeleteTextures`. OpenGL 4.5 DSA (`glCreateTextures`, `glTextureParameteri`, `glBindTextureUnit`).

---

## Ray Marching

### `RayMarchedSurface` *(abstract)* — inherits `ModelBase`
| Member | Signature |
|---|---|
| `(+,+)` | `m_hMapPath : string` |
| `#` | `m_HMapTextureID : GLuint` |
| `(+,!)` | `m_conemapTextureID : GLuint` |
| `(+,+)` | `m_technique : shared_ptr<IRayMarchingTechnique>` |
| `(+,+)` | `m_maxSteps : int` |
| `(+,+)` | `m_epsilon : float` |
| `(+,+)` | `m_lightDir : vec3` |
| `(+,+)` | `m_normalMult : float` |
| `(+,+)` | `m_discardFragments : bool` |
| `(+,+)` | `m_displayNonConverged : bool` |
| `(+,+)` | `m_showFlags : bool` |
| `(+,+)` | `m_interpolateTexture : bool` |
| `(+,+)` | `m_debugConfig : RayMarchDebugConfig` |
| + | `GetProgramID() : GLuint override` *(returns m_technique->GetProgramID())* |
| + | `Render(v : IModelRendererVisitor&) : void` |
| + | `AcceptGUIVisitor(v : IGUIVisitor&) : void` |
| + | `AcceptRendererVisitor(v : IModelRendererVisitor&) : void` |

### `IRayMarchingTechnique` *(interface)*
| Member | Signature |
|---|---|
| + | `GetProgramID() : GLuint` |
| + | `SetUniforms(surface : const RayMarchedSurface&) : void` |
| + | `GetName() : string` |

**Shader paths** are not part of the interface. They are provided as constructor arguments and registered with `ShaderManager::Load()` in `MyApp::Init()`. `ShaderManager` owns the compiled programs; techniques hold the resulting `GLuint`.

**`SetUniforms()`** is where the two techniques diverge in C++: each uploads its own technique-specific shader uniforms. `RayMarchedSurface::Render()` delegates entirely to the technique:
```
m_technique->SetUniforms(*this);
glUseProgram(m_technique->GetProgramID());
// draw call
```

### `LinearSearch` *(concrete)* — realizes `IRayMarchingTechnique`
### `ConeStepMapping` *(concrete)* — realizes `IRayMarchingTechnique`

Each technique owns its own dedicated shader — no `if/else` branching inside shaders.

---

## Inheritance hierarchy

```
IDrawable (interface)
    └── ModelBase (abstract) ── IGUIVisitable (interface)
                            └── IModelRendererVisitable (interface)
            ├── Model (abstract)          [GetProgramID → m_programID]
            └── RayMarchedSurface (abstract) [GetProgramID → m_technique->GetProgramID()]
```

`RayMarchedSurface` inherits directly from `ModelBase`, **not** from `Model` — it has no `Mesh` or `Material`. LSP is preserved.

---

## Key data types (`src/Headers/Types.h`)

```cpp
struct ShaderStage {
    GLenum      type;   // GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc.
    std::string path;
};

struct SUpdateInfo { /* delta time, elapsed time */ };
struct MeshRenderParams { /* per-draw overrides */ };
struct ModelBaseParams { /* constructor args for ModelBase */ };
struct ModelParams      { /* constructor args for Model     */ };
struct RayMarchedSurfaceParams { /* constructor args for RMS */ };
```

### `RayMarchDebugUniforms` — GPU-uploadable (std430 compatible)

Member order guarantees zero-padding when mirrored as a GLSL struct in std430 layout.

```cpp
struct RayMarchDebugUniforms {
    GLuint    showDebug;      // offset  0  (bool → GLuint: GLSL uint = 4 bytes)
    GLuint    showSteps;      // offset  4
    GLuint    showCones;      // offset  8
    GLuint    showEnterExit;  // offset 12
    glm::vec3 debugRayStart;  // offset 16  (vec3 base_align=16 in std430 ✓)
    GLuint    showRay;        // offset 28  (fills vec3 slot to 32 — no padding needed)
    glm::vec3 debugRayDir;    // offset 32  (vec3 base_align=16 ✓)
    GLuint    showHitPoint;   // offset 44  (fills vec3 slot to 48 — no padding needed)
    // sizeof = 48 bytes, zero wasted memory
};
```

Corresponding GLSL struct (`RayMarchDebug_uniforms.glsl`):
```glsl
struct RayMarchDebugUniforms {
    uint  showDebug;      // offset  0
    uint  showSteps;      // offset  4
    uint  showCones;      // offset  8
    uint  showEnterExit;  // offset 12
    vec3  debugRayStart;  // offset 16
    uint  showRay;        // offset 28
    vec3  debugRayDir;    // offset 32
    uint  showHitPoint;   // offset 44
};
```

### `RayMarchDebugConfig` — full C++ debug state

```cpp
struct RayMarchDebugConfig {
    RayMarchDebugUniforms uniforms;  // uploadable prefix
    GLuint debugSSBO   = 0;          // GL buffer handle, not uploaded as data
    GLuint pointsSSBO  = 0;          // GL buffer handle, not uploaded as data
};
// Upload: glNamedBufferSubData(ubo, 0, sizeof(uniforms), &m_debugConfig.uniforms);
```

---

## Implementation conventions

| Topic | Rule |
|---|---|
| OpenGL API | DSA everywhere — `glCreateTextures`, `glBindTextureUnit`, `glTextureParameteri` |
| Identity matrix | `glm::identity<T>()`, not `glm::mat4(1.0f)` |
| Parameters | `const` reference in all `Render()` and `Update()` calls |
| Getters/setters | Defined inline in header; complex logic in `.cpp` |
| Default ctor | `= default` when all members have in-class initializers |
| Static members | `inline static` with in-class initializer (C++17) |
| Forward declarations | In headers; full `#include` only in `.cpp` |
| GPU sync | `glFenceSync()` before resource-modifying Commands |
| Comments | English only |
| Ownership | `unique_ptr` = exclusive owner; `shared_ptr` = shared (scene objects, techniques); raw `*` = non-owning observer (SceneManager→MyApp pointers, Command targets already covered by shared_ptr) |
| GL lifecycle | Objects needing GL context use RAII (constructor/destructor); `MyApp` holds them as `unique_ptr`, creates in `Init()` |

---

## Known open issues

| Issue | Location | Notes |
|---|---|---|
| `Transform` parent-child incomplete | `Transform::SetParent()` | No child registry, no dangling pointer protection |
| `Material.h` uses raw `GLuint` | `src/Headers/Material/Material.h` | Design requires `shared_ptr<Texture>` — must migrate |
| `Texture.h/.cpp` empty | `src/Headers/Texture/` | Blocks Material migration |
| `IDrawable.h` missing `Update()` | `src/Interfaces/IDrawable.h` | Corrected version in `temp/Round1/IDrawable.h` |
| `CMakeLists.txt` wrong path | `target_include_directories` | `src/Interface` → `src/Interfaces` |
| `ShaderStage` not in `Types.h` | `src/Headers/Types.h` | Add per `temp/Round1/Types_ShaderStage_addition.txt` |
| `IRayMarchingTechnique` Round1 file has `GetShaderPaths()` | `temp/Round1/IRayMarchingTechnique.h` | Remove — not part of final interface |
| `ICommandQueue` missing from Round1 | `temp/Round1/` | New interface to add before CommandQueue implementation |
