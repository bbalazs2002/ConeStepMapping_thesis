# ConeStepMapping – Architecture Reference

> Source of truth: `docs/Architecture.md` (design); `docs/*.puml` (diagrams)

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
| # | `m_conemapGenerator : unique_ptr<ConemapGenerator>` |
| # | `m_techniques : map<string, shared_ptr<IRayMarchingTechnique>>` |
| # | `m_imguiVisitor : ImGuiVisitor` |
| # | `m_camera : Camera` |
| # | `m_cameraManipulator : CameraManipulator` |
| `(+,#)` | `m_windowSize : ivec2` |
| `(+,!)` | `m_elapsedTime : float` |
| # | `m_showAxes : bool = true` |
| # | `m_selectedIndex : int = -1` |
| # | `m_debugIndex : int = 0` |
| # | `m_heightMaps : const vector<string>` |
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

### `DeleteObjectCommand` *(concrete ICommand)*
| Member | Signature |
|---|---|
| - | `m_sceneManager : SceneManager&` |
| - | `m_target : shared_ptr<ISceneObject>` |
| + | `Execute() : void` |

### `SetHeightmapCommand` *(concrete ICommand)*
| Member | Signature |
|---|---|
| - | `m_surface : shared_ptr<RayMarchedModel>` |
| - | `m_textureManager : TextureManager&` |
| - | `m_path : string` |
| - | `m_generator : ConemapGenerator*` |
| + | `Execute() : void` |

**`Execute()`** loads the heightmap texture from `m_path` via `TextureManager::GetOrLoad()`, then calls `m_surface->SetHeightmap(texture, m_generator)`.

`m_surface` is a `shared_ptr` to keep the target alive until execution. `m_generator` is nullable — `nullptr` means skip conemap generation; lifetime guaranteed by `MyApp`.

### `SetTechniqueCommand` *(concrete ICommand)*
| Member | Signature |
|---|---|
| - | `m_surface : shared_ptr<RayMarchedModel>` |
| - | `m_technique : shared_ptr<IRayMarchingTechnique>` |
| + | `Execute() : void` |

### `SetLocationCommand` *(concrete ICommand)*
| Member | Signature |
|---|---|
| - | `m_target : shared_ptr<ModelBase>` |
| - | `m_location : vec3` |
| + | `Execute() : void` |

### `SetRotationCommand` *(concrete ICommand)*
| Member | Signature |
|---|---|
| - | `m_target : shared_ptr<ModelBase>` |
| - | `m_rotation : quat` |
| + | `Execute() : void` |

### `SetScaleCommand` *(concrete ICommand)*
| Member | Signature |
|---|---|
| - | `m_target : shared_ptr<ModelBase>` |
| - | `m_scale : vec3` |
| + | `Execute() : void` |

### `SetMaxStepsCommand` *(concrete ICommand)*
| Member | Signature |
|---|---|
| - | `m_surface : shared_ptr<RayMarchedModel>` |
| - | `m_maxSteps : int` |
| + | `Execute() : void` |

### `SetEpsilonCommand` *(concrete ICommand)*
| Member | Signature |
|---|---|
| - | `m_surface : shared_ptr<RayMarchedModel>` |
| - | `m_epsilon : float` |
| + | `Execute() : void` |

### `SetMaterialCommand` *(concrete ICommand)*
| Member | Signature |
|---|---|
| - | `m_mesh : shared_ptr<Mesh>` |
| - | `m_material : shared_ptr<Material>` |
| + | `Execute() : void` |

Material per-`Mesh` van tárolva — a command `Mesh`-t céloz, nem `Model`-t.

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

**`ReloadAll()` (`Ctrl+F5`):** relinks each program **in place** — same `GLuint`, new compiled code. Does not call `glDeleteProgram`/`glCreateProgram`. This is essential: every class that caches a `programID` at construction (`LinearSearch`, `ConeStepMapping`, `ConemapGenerator`, `SkyboxRenderer`, `AxesRenderer`) would otherwise hold a dangling handle after the first reload. Safe because `GLUtils::LinkProgram(id, true)` already detaches+deletes the attached shader objects after every link call, regardless of success — relinking the same program object leaks nothing.

### `SceneManager`
| Member | Signature |
|---|---|
| - | `m_updatables : vector<shared_ptr<IUpdatable>>` |
| (+,!) | `m_sceneObjects : vector<shared_ptr<ISceneObject>>` |
| - | `m_rendererVisitables : vector<shared_ptr<IModelRendererVisitable>>` |
| - | `m_selected : const ISceneObject* = nullptr` |
| + | `Add(s : shared_ptr<ISceneObject>) : void` |
| + | `Remove(s : shared_ptr<ISceneObject>) : void` *(clears `m_selected` if the removed object was selected)* |
| + | `Update(info : SUpdateInfo) : void` |
| + | `Render(v : IModelRendererVisitor&) : void` *(calls `v.SetSelected(m_selected)` before the loop)* |
| + | `RenderGUI(v : IGUIVisitor&) : void` |
| + | `GetSceneObjects() : const vector<shared_ptr<ISceneObject>>&` |
| + | `SetSelected(selected : const ISceneObject*) : void` |
| + | `GetSelected() : const ISceneObject*` |
| + | `Clear() : void` |

**Ownership:** `SceneManager` is the primary owner of scene objects. `MyApp` creates them via `make_shared<ConcreteType>(...)` and calls `Add()`. `MyApp` may retain its own `shared_ptr` to objects it needs to reference later (e.g. for ImGui controls). The object stays alive as long as at least one `shared_ptr` holds it — this guarantees that `DeleteObjectCommand` keeps the target alive until `Execute()` runs and GL resources are safely freed after GPU sync.

`m_sceneObjects` doubles as the GUI-visitable list — `ISceneObject` extends `IGUIVisitable`, so `RenderGUI()` iterates this vector directly. No separate `m_guiVisitables` vector needed.

### `MaterialManager`
| Member | Signature |
|---|---|
| - | `m_materials : unordered_map<size_t, weak_ptr<Material>>` |
| + | `GetOrCreate(mat : Material) : shared_ptr<Material>` |
| - | `CollectGarbage() : void` |

Key: `Material::Hash()`. Cache uses `weak_ptr` — manager does not prevent destruction. `CollectGarbage()` private; `GetOrCreate()` hívja a hívás elején. Nem fut rendereléskor — csak scene setup idején, amikor egy objektum material-t kér le.

### `TextureManager`
| Member | Signature |
|---|---|
| - | `m_cache : unordered_map<string, weak_ptr<Texture>>` |
| + | `GetOrLoad(path : path, flip : bool) : shared_ptr<Texture>` |
| + | `GetOrLoadCubemap(faces : array<path, 6>, flip : bool) : shared_ptr<Texture>` |
| - | `CollectGarbage() : void` |
| + | `GetCachedCount() : size_t` |

Key: file path string (`GetOrLoad`) or the 6 face paths joined with `"|"` (`GetOrLoadCubemap`) — both share `m_cache`; the joined-key format cannot collide with a single-path key. Cache uses `weak_ptr`. `CollectGarbage()` private; both lookup methods call it at the start.

### `SkyboxRenderer` *(RAII)*
| Member | Signature |
|---|---|
| - | `m_programID : GLuint` |
| - | `m_cubemap : shared_ptr<Texture>` |
| - | `m_gpuObject : OGLObject` |
| + | `SkyboxRenderer(programID : GLuint, cubemap : shared_ptr<Texture>)` *(constructor — builds the cube VAO; texture is supplied, not loaded)* |
| + | `~SkyboxRenderer()` *(destructor — frees the VAO; does NOT delete the program or the texture — `ShaderManager`/`shared_ptr<Texture>` own those)* |
| + | `Render(cam : ICamera&) : void` |

**Texture ownership:** `SkyboxRenderer` does not load image files itself. `MyApp::Init()` calls `TextureManager::GetOrLoadCubemap(faces, flip)` and passes the resulting `shared_ptr<Texture>` into the constructor — same Flyweight-cached pattern as every other texture in the codebase (heightmap, conemap, material textures). This keeps `SkyboxRenderer` a pure renderer (shader + geometry + a reference to GPU data it doesn't own), consistent with how `RayMarchedModel` holds `shared_ptr<Texture>` for its conemap.

### `AxesRenderer` *(RAII)*
| Member | Signature |
|---|---|
| - | `m_programID : GLuint` |
| + | `AxesRenderer(programID : GLuint)` *(constructor — loads GL resources)* |
| + | `~AxesRenderer() = default` *(does NOT delete the program; ShaderManager owns it)* |
| + | `Render(cam : ICamera&) : void` |

**Shader-program ownership:** `SkyboxRenderer`/`AxesRenderer` take a plain `GLuint programID`, matching `LinearSearch`/`ConeStepMapping`/`ConemapGenerator` — none of these classes know `ShaderManager` exists. `MyApp::Init()` is the single mediator: it calls `ShaderManager::Load()` for every program and hands out the resulting IDs to whichever class needs them. `ShaderManager` remains the sole owner of program lifetime (`DeleteAll()` in `MyApp::Clean()`); consumer classes never call `glDeleteProgram`.

---

## Camera

> `ICamera` és `Camera` egyaránt a `src/Utils/` mappában található. Az interfész a Utils zárt rendszerének része, így más kameraimplementációk is megvalósíthatják anélkül, hogy a Utils belső részeitől függnének.

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
| + | `Visit(target : RayMarchedModel&) : void` |

### `IGUIVisitable` *(interface)*
| Member | Signature |
|---|---|
| + | `AcceptGUIVisitor(v : IGUIVisitor&) : void` |

### `ISceneObject` *(interface)* — extends `IGUIVisitable`
| Member | Signature |
|---|---|
| + | `GetName() : const string&` *(pure virtual)* |
| + | `~ISceneObject() = default` *(virtual destructor)* |

Minden `SceneManager`-be kerülő objektumnak meg kell valósítania ezt az interfészt. `ModelBase` megvalósítja — így `Model` és `RayMarchedModel` is automatikusan eleget tesz ennek. `SceneManager::GetSceneObjects()` ezen a típuson keresztül ad vissza listát, nem konrét `ModelBase`-en — ezért `MyApp` névhez és GUI-látogatáshoz nem kell cast, csak `RayMarchedModel`-specifikus vezérlőkhöz.

### `ImGuiVisitor` *(concrete)* — realizes `IGUIVisitor`
| Member | Signature |
|---|---|
| - | `m_commandQueue : ICommandQueue&` |
| + | `ImGuiVisitor(queue : ICommandQueue&)` |
| + | `Visit(target : Model&) : void` |
| + | `Visit(target : RayMarchedModel&) : void` |
| - | `VisitModelBase(target : ModelBase&) : void` |

**Invariant:** never modifies objects directly — creates `ICommand` instances and pushes to `ICommandQueue`. Depends on the interface, not the concrete `CommandQueue`. Reference member guarantees non-null; lifetime guaranteed by `MyApp`.

`shared_ptr` access for commands: `Visit()` calls `target.shared_from_this()` (available because `ModelBase` inherits `enable_shared_from_this<ModelBase>`); for `RayMarchedModel`-specific commands, the result is `dynamic_pointer_cast<RayMarchedModel>(...)`. `shared_from_this()` is only valid when the object is already managed by a `shared_ptr`, which is guaranteed since all scene objects are added via `SceneManager::Add(shared_ptr<ISceneObject>)`.

Direct modification (without commands) is used only for properties that have no corresponding command and do not require deferred execution: `SetShow()`, `SetWireframe()`, `SetLightDir()`, `SetNormalMult()`, boolean render flags, and `RayMarchDebugConfig` fields.

---

## Renderer Visitor

### `IModelRendererVisitor` *(interface)*
| Member | Signature |
|---|---|
| + | `Visit(target : const Model&) : void` |
| + | `Visit(target : const RayMarchedModel&) : void` |
| + | `SetSelected(selected : const ISceneObject*) : void` *(default no-op — `SceneManager::Render()` calls this before iterating; concrete visitors override to store the selected pointer)* |

### `IModelRendererVisitable` *(interface)*
| Member | Signature |
|---|---|
| + | `AcceptRendererVisitor(v : IModelRendererVisitor&) : void` |

### `OpenGLRendererVisitor` *(concrete)* — realizes `IModelRendererVisitor`
| Member | Signature |
|---|---|
| - | `m_camera : const ICamera&` |
| - | `m_selected : const ModelBase* = nullptr` |
| + | `OpenGLRendererVisitor(cam : const ICamera&)` |
| + | `~OpenGLRendererVisitor() = default` |
| + | `SetSelected(selected : const ModelBase*) : void` |
| + | `Visit(target : const Model&) : void` |
| + | `Visit(target : const RayMarchedModel&) : void` |
| - | `VisitModelBase(target : const ModelBase&) : void` |

**`VisitModelBase()`** calls `target.GetProgramID()` virtually — works correctly for both `Model` and `RayMarchedModel` without knowing the concrete type. Camera data (`viewProj`, `cameraPos`) is read live from `m_camera` in every `Visit()` call — no manual sync needed. Reference member guarantees non-null; lifetime guaranteed by `MyApp`.

**`SetSelected()`** is called once per frame by `MyApp::Render()` before `SceneManager::Render(*this)`, passing the raw pointer of the currently selected `ModelBase` (or `nullptr` if nothing is selected). `Visit(const Model&)` then checks `&target == m_selected` to decide whether to run the selection highlight pass.

---

## Scene hierarchy

### `IUpdatable` *(interface)*
| Member | Signature |
|---|---|
| + | `Update(info : SUpdateInfo) : void` |
| + | `~IUpdatable() = default` *(virtual destructor)* |

`SceneManager` három specializált vektort tart fenn: `m_sceneObjects`, `m_updatables`, `m_rendererVisitables`. Az `Add(shared_ptr<ISceneObject>)` híváskor egyszer futnak a `dynamic_pointer_cast` hívások az `IUpdatable` és `IModelRendererVisitable` vektorokhoz; `m_sceneObjects`-ba cast nélkül kerül be az objektum. A per-frame loopok (`Update`, `Render`, `RenderGUI`) mindig csak a specializált vektoron futnak. Az `IDrawable` interfész eltávolítva — szerepét `ISceneObject` vette át mint belépési pont. Az `m_show` ellenőrzés az `OpenGLRendererVisitor::Visit()` metódusokba került.

### `ModelBase` *(abstract)* — realizes `ISceneObject`, `IModelRendererVisitable`, `IUpdatable`; inherits `enable_shared_from_this<ModelBase>`
| Member | Signature |
|---|---|
| `(+,+)` | `m_name : string` |
| `(+,+)` | `m_show : bool` |
| `(+,+)` | `m_drawMode : int` |
| `(+,#)` | `m_transform : Transform` |
| + | `GetProgramID() : GLuint` *(pure virtual)* |
| + | `GetName() : const string&` *(override; satisfies ISceneObject)* |
| + | `Update(info : SUpdateInfo) : void` *(no-op default; override if needed)* |
| + | `GetTransform() : Transform&` |
| + | `AcceptGUIVisitor(v : IGUIVisitor&) : void` *(pure virtual; from ISceneObject → IGUIVisitable)* |
| + | `AcceptRendererVisitor(v : IModelRendererVisitor&) : void` *(pure virtual)* |

**Meshing:** meshek hozzáadása `AddMesh()`-en keresztül, tipikusan `MyApp::Init()`-ben egy loader utility segítségével.

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

### `Model` *(concrete)* — inherits `ModelBase`
| Member | Signature |
|---|---|
| + | `Model(name : string = "unnamed")` |
| # | `m_programID : GLuint = 0` |
| # | `m_selectedProgramID : GLuint = 0` |
| `(+,+)` | `m_wireframe : bool = false` |
| + | `GetProgramID() : GLuint override` |
| + | `SetProgram(id : GLuint) : void` |
| + | `GetSelectedProgramID() : GLuint` |
| + | `SetSelectedProgram(id : GLuint) : void` |
| + | `SetWireFrame(wf : bool) : void` |
| + | `AddMesh(mesh : shared_ptr<Mesh>) : void` |
| + | `AcceptGUIVisitor(v : IGUIVisitor&) : void override` |
| + | `AcceptRendererVisitor(v : IModelRendererVisitor&) : void override` |

**Program ID pairs** on `Model`: `m_programID` (normal render) and `m_selectedProgramID` (wireframe overlay for selected state) follow the same ownership pattern — `ShaderManager` owns the GL programs, `MyApp::Init()` fetches both via `Get()` and assigns them via `SetProgram()`/`SetSelectedProgram()`. If `m_selectedProgramID` is `0` (not assigned), `OpenGLRendererVisitor::Visit()` skips the outline pass silently.

### `Mesh` *(util)*
| Member | Signature |
|---|---|
| - | `m_GPU : OGLObject` |
| `(+,+)` | `m_material : shared_ptr<Material>` |
| `(+,+)` | `m_drawMode : int = GL_TRIANGLES` |
| + | `Build(mesh : MeshObject) : void` |
| + | `Render() : void` |
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
| `<u>-</u>` | `s_lastTextureTargets : GLuint[4]` |
| `<u>-</u>` | `s_hasUploadedData : bool = false` |

**Invariant:** destructor `= default` — `shared_ptr<Texture>` members handle GPU cleanup.

### `Texture` *(RAII)*
| Member | Signature |
|---|---|
| - | `m_id : GLuint` |
| - | `m_type : GLenum` *(`GL_TEXTURE_2D` or `GL_TEXTURE_CUBE_MAP`)* |
| + | `GetID() : GLuint` |
| + | `IsValid() : bool` |
| + | `GetType() : GLenum` |

**Invariant:** non-copyable, movable (both `m_id` and `m_type` transfer on move). Destructor calls `glDeleteTextures`. OpenGL 4.5 DSA (`glCreateTextures`, `glTextureParameteri`, `glBindTextureUnit`).

**`GetType()`:** lets external code (e.g. a future ImGui texture preview) branch on 2D-vs-cubemap without the `Texture` class itself needing any type-specific public API — binding/sampling stay uniform (`glBindTextureUnit` is target-agnostic at bind time), only consumers that actually need to *display* the texture differently (a cubemap can't be shown directly via `ImGui::Image`) query this.

**Three constructors**, one GPU-resource shape each: `Texture(path, flip)` (file → `GL_TEXTURE_2D`, mipmapped, `GL_REPEAT`), `Texture(width, height, internalFormat)` (empty, compute shader output, single mip, `GL_CLAMP_TO_EDGE`), `Texture(faces[6], flip)` (6 files → `GL_TEXTURE_CUBE_MAP`, single mip, `GL_CLAMP_TO_EDGE`, enables `GL_TEXTURE_CUBE_MAP_SEAMLESS`).

---

## Ray Marching

### `RayMarchedModel` *(concrete)* — inherits `ModelBase`
| Member | Signature |
|---|---|
| + | `RayMarchedModel(name : string = "unnamed")` |
| `(+,!)` | `m_conemap : shared_ptr<Texture>` |
| `(+,!)` | `m_technique : shared_ptr<IRayMarchingTechnique>` |
| `(+,+)` | `m_maxSteps : int = 64` |
| `(+,+)` | `m_epsilon : float` |
| `(+,+)` | `m_lightDir : vec3` |
| `(+,+)` | `m_normalMult : float` |
| `(+,+)` | `m_discardFragments : bool` |
| `(+,+)` | `m_displayNonConverged : bool` |
| `(+,+)` | `m_showFlags : bool` |
| `(+,+)` | `m_debugConfig : RayMarchDebugConfig` |
| + | `GetProgramID() : GLuint override` |
| + | `SetTechnique(t : shared_ptr<IRayMarchingTechnique>) : void` |
| + | `SetHeightmap(heightmap : shared_ptr<Texture>, gen : ConemapGenerator*) : void` |
| + | `AddMesh(mesh : shared_ptr<Mesh>) : void` |
| + | `AcceptGUIVisitor(v : IGUIVisitor&) : void override` |
| + | `AcceptRendererVisitor(v : IModelRendererVisitor&) : void override` |

**`SetHeightmap()`** runs the compute shader via `gen->Generate(*heightmap)` and assigns the result to `m_conemap`. The heightmap itself is not retained; only the conemap is kept. If `gen` is `nullptr`, logs an error and returns early — `m_conemap` is left unchanged.

### `ConemapGenerator` *(utility)*
| Member | Signature |
|---|---|
| + | `ConemapGenerator(programID : GLuint)` |
| - | `m_programID : GLuint` |
| + | `Generate(heightmap : const Texture&) : shared_ptr<Texture>` |

Wraps the conemap compute shader. `Generate()` dispatches the compute shader on the heightmap and returns the resulting conemap as a new `Texture`. Owned by `MyApp`; passed by reference wherever conemap generation is needed.

### `IRayMarchingTechnique` *(interface)*
| Member | Signature |
|---|---|
| + | `GetProgramID() : GLuint` |
| + | `SetUniforms(surface : const RayMarchedModel&) : void` |
| + | `GetName() : string` |

**Shader paths** are not part of the interface. They are provided as constructor arguments and registered with `ShaderManager::Load()` in `MyApp::Init()`. `ShaderManager` owns the compiled programs; techniques hold the resulting `GLuint`.

**`SetUniforms()`** is where the two techniques diverge in C++: each uploads its own technique-specific shader uniforms. `OpenGLRendererVisitor::Visit(const RayMarchedModel&)` calls them in this order:
```
glUseProgram(technique->GetProgramID());
VisitModelBase(target);               // camera + world transform uniforms
technique->SetUniforms(target);       // technique-specific uniforms
// draw call — iterates m_meshes
```

### `LinearSearch` *(concrete)* — realizes `IRayMarchingTechnique`

| Member | Signature |
|---|---|
| - | `m_programID : GLuint` |
| + | `LinearSearch(programID : GLuint)` |
| + | `GetProgramID() : GLuint override` |
| + | `SetUniforms(surface : const RayMarchedModel&) : void override` |
| + | `GetName() : string override` → `"Linear Search"` |

**`SetUniforms()`** binds the conemap to unit 4, then uploads: `maxSteps`, `normalMult`, `lightDir`, `discardFragments`, `displayNonConverged`.

### `ConeStepMapping` *(concrete)* — realizes `IRayMarchingTechnique`

| Member | Signature |
|---|---|
| - | `m_programID : GLuint` |
| + | `ConeStepMapping(programID : GLuint)` |
| + | `GetProgramID() : GLuint override` |
| + | `SetUniforms(surface : const RayMarchedModel&) : void override` |
| + | `GetName() : string override` → `"Cone Step Mapping"` |

**`SetUniforms()`** binds the conemap to unit 4 (logs error if conemap is null), then uploads the same uniforms as `LinearSearch` plus `epsilon` and `dbg.showSteps`, `dbg.showCones`, `dbg.showEnterExit`, `dbg.showFlags` from `RayMarchDebugConfig`.

Each technique owns its own dedicated shader — no `if/else` branching inside shaders.

---

## Inheritance hierarchy

```
ModelBase (abstract) ── ISceneObject (interface) ── IGUIVisitable (interface)
                     ── IModelRendererVisitable (interface)
                     ── IUpdatable (interface)
    ├── Model (concrete)             [GetProgramID → m_programID]
    └── RayMarchedModel (concrete)   [GetProgramID → m_technique->GetProgramID()]
```

`RayMarchedModel` inherits directly from `ModelBase` and owns its own meshes (bármilyen geometria, pl. .obj-ből). A ray marching technique a heightmap alapján displace-eli a felületet renderelés közben.

---

## Key data types (`src/Headers/Types.h`)

Részletes diagram: `docs/ClassDiagram_Types.puml`

```cpp
struct ShaderStage {
    GLenum      type;   // GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc.
    std::string path;
};

struct SUpdateInfo { /* delta time, elapsed time */ };
```

### `RayMarchDebugConfig` — CPU-oldali debug konfiguráció (Phase 11+)

Nincs std430 layout-kényszer — kizárólag CPU-oldali. A GPU a debug SSBO-kból olvassa a konfigurációt (nem uniform struct-ból).

```cpp
struct RayMarchDebugConfig {
    bool showDebug     = false;
    bool showSteps     = false;
    bool showEnterExit = false;
    bool showCones     = false;
    bool showRay       = false;
    bool showHitPoint  = false;
    int  primitiveID   = -1;
    int  technique     = 0;   // OpenGLRendererVisitor frissíti GetTechniqueID()-vel
};
```

### `RayMarchDebugState` — globális debug állapot

MyApp tulajdona (értékszerű tag). `OpenGLRendererVisitor` nem tulajdonló mutatón éri el.

```cpp
struct RayMarchDebugState {
    GLuint debugVisualSSBO    = 0;   // binding 0 — lépés-pozíciók + CPU config
    GLuint debugNumericalSSBO = 0;   // binding 1 — numerikus trace-adatok
    Camera debugCamera;
    RayMarchDebugConfig config;
    const RayMarchedModel* target = nullptr;
};
```

A debug SSBO-k részletes elrendezése: `docs/SSBO_Debug_Layout.md`. Diagram: `docs/diagrams/ClassDiagram_Debug.puml`.

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
| Ownership | `unique_ptr` = exclusive owner; `shared_ptr` = shared (scene objects, techniques, command targets); raw `*` = non-owning observer only when lifetime is structurally guaranteed (e.g. value member of `MyApp`) or intentionally nullable |
| GL lifecycle | Objects needing GL context use RAII (constructor/destructor); `MyApp` holds them as `unique_ptr`, creates in `Init()` |

---

## Known open issues

| Issue | Location | Notes |
|---|---|---|
| `Transform` parent-child incomplete | `Transform::SetParent()` | No child registry, no dangling pointer protection |
| `Frag_Model.frag` fényszámítás nélküli | `src/Shaders/Models/Frag_Model.frag` | Csak `materialData` diffúz+emisszió színét adja ki, nem hívja a `Light` modul `LightCalculate()`-jét — egyenletes, árnyalás nélküli megjelenítés. Szándékosan elhalasztva: egyelőre nincs fényforrás-kezelés (C++ oldali `Light`/`LightManager` és a hozzá tartozó SSBO feltöltés sincs megírva). Pótlandó, amint a fényforrás-rendszer elkészül. |
| `RayMarchedModel` kiemelés hiányzik | `OpenGLRendererVisitor::Visit(const RayMarchedModel&)` | A `Visit(const Model&)`-ban megvalósított wireframe overlay kiemelés analóg kiterjesztése `RayMarchedModel`-re elmaradt. Pótlandó. A `RayMarchedModel`-nek is szüksége lesz `m_selectedProgramID` + `GetSelectedProgramID()`/`SetSelectedProgram()` tagokra. |
