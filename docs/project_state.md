---
name: project-state
description: Current implementation state of the ConeStepMapping thesis — architecture, source layout, phase history, known issues
metadata:
  type: project
---

## Mi ez a projekt?

ELTE szakdolgozat: Cone Step Mapping (CSM) technika implementálása és vizualizálása OpenGL 4.5 + SDL3 + Dear ImGui segítségével. A cél egy interaktív alkalmazás, ahol heightmap alapján conemap-et generálunk compute shaderrel, majd ray marching technikával (Linear Search / Cone Step Mapping) renderelünk displacement-tel.

**Why:** a CSM hatékonyabb, mint a lineáris keresés, mert a cone-sugár garantáltan átugorja az "üres" textúratartományokat.

---

## Teljes forrásfa (src/)

```
src/
  Interfaces/       ICommand, ICommandQueue, IGUIVisitable, IGUIVisitor,
                    IModelRendererVisitable, IModelRendererVisitor,
                    IRayMarchingTechnique, ISceneObject, IUpdatable,
                    IGraphicsApp
  Headers/
    Command/        CommandQueue, DeleteObjectCommand, SetHeightmapCommand,
                    SetTechniqueCommand, SetLocation/Rotation/ScaleCommand,
                    SetMaxStepsCommand, SetEpsilonCommand, SetMaterialCommand,
                    SetSelectedCommand, CreateRMObjectCommand, CreateObjObjectCommand
    Manager/        SceneManager, ShaderManager, MaterialManager, TextureManager
    Model/          ModelBase, Model, RayMarchedModel, Mesh
    Material/       Material
    Texture/        Texture
    Transform/      Transform
    RayMarching/    LinearSearch, ConeStepMapping, ConemapGenerator
    Renderer/       SkyboxRenderer, AxesRenderer, DebugRenderer
    RendererVisitor/ OpenGLRendererVisitor
    GUIVisitor/     ImGuiVisitor
    Debug/          RayMarchDebugState
    Types.h, MyApp.h
  Sources/          tükrözi a Headers/ struktúrát
    MyApp/          MyApp.cpp, InitClean.cpp, GUI.cpp, Debug.cpp,
                    EventHandlers.cpp, CreateModels.cpp    ← MyApp szétbontva
  Utils/            Camera, CameraManipulator, ModelLoader, GLUtils, stb.
  Shaders/
    Models/         Vert_Model, Frag_Model, Vert_ModelSelected, Frag_ModelSelected
    RayMarching/    Vert_RM, Geom_RM_abcd, Frag_LinearSearch, Frag_ConeStepMapping
    Conemap/        Comp_Conemap (compute shader — eredeti + konzervatív permutáció)
    Modules/        Camera, Transform, Color, Material shader modulok
    Skybox/, Axes/, Debug/
```

---

## Osztályhierarchia

```
ISceneObject → IGUIVisitable
ModelBase (abstract) ── ISceneObject, IModelRendererVisitable, IUpdatable
                     ── enable_shared_from_this<ModelBase>
    ├── Model          — mesh-based, Vertex layout, m_programID + m_selectedProgramID
    └── RayMarchedModel — ray marching, VertexMergedNorm layout, GetProgramID() → technique
```

**IDrawable eltávolítva** — szerepét ISceneObject vette át.

---

## Megvalósított design patternek

| Pattern | Hol |
|---|---|
| Command | ImGui → CommandQueue → Execute() az Update() elején |
| Strategy | IRayMarchingTechnique: LinearSearch / ConeStepMapping |
| Visitor (×2) | ImGuiVisitor (GUI), OpenGLRendererVisitor (render) |
| Flyweight (×2) | MaterialManager (hash), TextureManager (path, weak_ptr cache) |
| RAII | Texture, SkyboxRenderer, AxesRenderer, DebugRenderer |

---

## MyApp felépítése (Phase 16 állapot)

`src/Sources/MyApp/` — szétbontott fájlok:
- `MyApp.cpp` — konstruktorok, Update, Render, UpdateTechniquePrograms
- `InitClean.cpp` — Init(), Clean()
- `GUI.cpp` — RenderGUI(), RenderDebugPanel()
- `Debug.cpp` — SetupDebugCallback(), InitDebugSSBOs(), CleanupDebugSSBOs(), ExportDebugLog()
- `EventHandlers.cpp` — összes SDL eseménykezelő
- `CreateModels.cpp` — CreateDefaultRayMarchedModel(), CreateModelFromOBJ()

**Fontos**: `MyApp.cpp` tartalmaz `#include "Interfaces/ICommand.h"` — szükséges, mert `~MyApp() = default` ott van definiálva, és a `CommandQueue` destruktora `ICommand` teljes típusát igényli.

---

## GUI Panel struktúra (Phase 16)

- **Global** panel (bal, első tab): Resolution, Show axes, Light Dir (globális, minden RayMarchedModel-re), Skybox (mappa input + Apply gomb), debug export
- **Models** panel (bal, második tab): modell lista, Deselect, Add RayMarched, Add .obj Model (path input + gomb), Delete Selected (piros); kijelölt modell kontrolljai jobbra: transform, Ray Marching (TreeNode), Technique combo, Heightmap combo, Conemap (TreeNode)
- **Debug** panel (alul): Settings tab (debug togglek, Primitive ID, debug kamera) + Values tab (SSBO adatok, step táblázat)

---

## Command-ok létrehozáshoz/törléshez

- `CreateRMObjectCommand(SceneManager&, shared_ptr<RayMarchedModel>)` — Execute(): Add()
- `CreateObjObjectCommand(SceneManager&, shared_ptr<Model>)` — Execute(): Add()
- `DeleteObjectCommand(SceneManager&, shared_ptr<ISceneObject>)` — Execute(): Remove()
- `SetSelectedCommand(SceneManager&, shared_ptr<ISceneObject>)` — Execute(): SetSelected()

Az "Add" gombok a `CreateDefaultRayMarchedModel()` / `CreateModelFromOBJ()` factory metódusokkal hozzák létre az objektumot (GL thread, RenderGUI()-ban), majd a parancsot beteszik a CommandQueue-ba → Execute() az Update()-ben adja hozzá a SceneManager-hez.

---

## Vertex típusok — FONTOS különbség

```cpp
struct Vertex           { vec3 pos; vec3 normal; vec2 texcoord; }
                          // texcoord = location 2
struct VertexMergedNorm { vec3 pos; vec3 normal; vec3 mergedNormal; vec2 texcoord; }
                          // texcoord = location 3 !
```

- **`Model` (.obj betöltés)**: `LoadFromOBJ<Vertex>` + `Build<Vertex>` — a `model` shader location 2-n várja a texcoord-ot
- **`RayMarchedModel`**: `LoadFromOBJ<VertexMergedNorm>` + `MergeNormals` + `Build<VertexMergedNorm>` — a ray marching shader location 2-n a mergedNormal-t olvassa

**Ha Model-hez VertexMergedNorm-ot használsz, a textúrák torzulnak** (rossz UV).

---

## OBJ textúra betöltés

`CreateModelFromOBJ()` helyes mintája (Phase 8/9 alapján):
```cpp
auto data = ModelLoader::LoadFromOBJ<Vertex>(path, "./");  // NEM VertexMergedNorm
auto texFolder = filesystem::path(path).parent_path();

// Per matMesh:
auto loadTex = [&](const std::string& texName) {
    if (texName.empty()) return nullptr;
    auto absPath = ModelLoader::ResolveTexturePath(texName, texFolder);
    return m_textureManager.GetOrLoad(absPath, false);   // flip=false
};
mat.SetDiffuseTex(loadTex(tmat.diffuse_texname));
// ... specular, emissive, normal ugyanígy
```

`flip=true` tükrözi a textúrát — **ne használd** OBJ modelleknél.

---

## Debug rendszer (Phase 11–13)

Két SSBO:
- `debugVisualSSBO` (binding 0): step pozíciók textúra-térben + CPU config (showDebug, showSteps, stb., debug kamera)
- `debugNumericalSSBO` (binding 1): 2×uvec4 indirect draw command + numerikus trace adatok (mátrixok, sugár, step táblázat)

**Primitive ID filter fix** (Phase 16): az indirect command slotokat minden frame elején nullázni kell (`{0,1,0,0}`), különben a GS által nem felülírt keret "stale" adatot rajzol ki.

**1 frame latency**: szándékos — a GS az N. frame-ben ír SSBO-ba, a DebugRenderer az (N+1). frame-ben rajzol.

---

## Wireframe selection overlay

Mindkét modell típusnál: `m_selectedProgramID` (GLuint) + `GetSelectedProgramID()` / `SetSelectedProgram()`.
- Shader: `model_selected` (Color modul — narancs `vec3(1, 0.6, 0)`)
- `OpenGLRendererVisitor::Visit()`: ha `&target == m_selected && selProg != 0` → második draw pass `GL_LINE` + `glLineWidth(3)` + cull_face disabled

---

## Heightmap / Conemap flow

1. `TextureManager::GetOrLoad(path, false)` → `shared_ptr<Texture>` (heightmap)
2. `ConemapGenerator::Generate(heightmap)` → `shared_ptr<Texture>` (conemap, compute shader)
3. `RayMarchedModel::SetHeightmap(heightmap, gen)` — heightmap NEM kerül tárolásra, csak a conemap
4. `IRayMarchingTechnique::SetUniforms()` — binds conemap to unit 4 + feltölti az uniformokat

Shader permutációk: `ls_HN`, `ls_HB`, `csm_HN_CN`, `csm_HB_CN`, `csm_HN_CB`, `csm_HB_CB` (H=height interp, C=cone interp, N=nearest, B=bilinear).

---

## Ismert nyitott problémák (Architecture.md alapján)

- `Transform::SetParent()` — nem teljes, nincs child registry
- `Frag_Model.frag` — nincs fényszámítás (LightCalculate() nincs meghívva), egyelőre nincs Light rendszer
