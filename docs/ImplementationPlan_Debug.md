# Phase 11 — Debug rendszer implementációs terv

Alapja: `docs/Debug.md` (SSBO struktúrák), a prototípus `Glsl_common.glsl` / `Geom_Model_old.geom` / `RayMarchedSurface.cpp` / `MyApp.cpp` implementációja.

---

## Architekturális döntések

### Debug search a GS-ben fut

A jelenlegi architektúrában a **FS** végzi az intersection keresést. A debug SSBO-ba írás ezzel szemben a **GS**-ből történik, mert:

- A GS primitívenként **egyszer** fut — nincs fragment-konkurencia, nincs race condition az SSBO-írásban.
- A T és M mátrixok a GS-ben keletkeznek; a numerikus debug adatainak nagy része ezekből számítható.
- Az eredeti prototípus is a GS-ből hajtja a debug keresést.

### Debug SSBOs globálisak, nem modellenként

Egyszerre mindig csak egyetlen primitívet vizsgálunk. Az SSBO-kat `MyApp` hozza létre és tartja (nem `RayMarchedModel`). Az `OpenGLRendererVisitor` köti be és tölti fel őket minden `RayMarchedModel` renderelése előtt.

### FS-ben nincs debug kód — `DEBUG_FUNCTIONS` makró

A debug SSBO-deklarációk és az összes SSBO-t elérő függvény kizárólag a `#ifdef DEBUG_FUNCTIONS` blokkban él. A GS a fájl elején definiálja `#define DEBUG_FUNCTIONS`. A FS-be nem kerül SSBO-deklaráció és debug kód.

### Minden debug adat az SSBO-ban van — nincs debug uniform struct

A debug konfiguráció (showDebug, showSteps, showEnterExit, showCones, showRay, showHitPoint, primitiveID, technique) a `debugVisualSSBO` fejlécének két foglalt vec4 slotjába kerül, amelyeket a CPU ír render előtt. A GS ezeket az SSBO-ból olvassa, nem uniform-okból. Ezáltal:

- A `SetUniforms(const RayMarchedModel&)` szignatúrája **változatlan** marad — nem kap `ICamera&` paramétert.
- A GLSL shaderekben nem keletkezik debug uniform struct.
- Az `OpenGLRendererVisitor` saját maga kezeli a debug SSBO feltöltést a `technique->SetUniforms()` hívás előtt.
- Egyetlen szükséges új interfészmetódus: `IRayMarchingTechnique::GetTechniqueID()` (0=LS, 1=CSM), amit a renderer a SSBO config slotba ír.

### Debug kamera — meglévő `Camera` osztály, közvetlen manipuláció

Nincs dedikált `DebugCamera` osztály. A `RayMarchDebugState` a meglévő `Camera` osztály egy példányát tárolja. A nézési irányt `normalize(GetAt() - GetEye())` adja (`ICamera` megköveteli mindkét gettert).

A debug kamera `SetEye()`/`SetAt()` metódusait a `RenderDebugPanel()` közvetlenül hívja — ugyanúgy, ahogy a `CameraManipulator` kezeli a főkamerát. Nincs szükség külön Command osztályokra a kamera pozíciójának változtatásához.

### Debug primitív kiválasztása

A `debugVisualSSBO[6].z` slot tárolja a primitív ID-t (int-ként, float-ba castolva). Ha az érték negatív vagy nem egyezik a `gl_PrimitiveIDIn`-nel, a GS nem futtat debug kódot — nincs "első metsző" fallback. A felhasználó UI-ról állítja be a sorszámot; invalid értéknél az SSBO count-ja 0 marad, ez jelzi, hogy a kiválasztott ID nem metszett primitívet.

---

## SSBO struktúra és index térkép

### `debugVisualSSBO` (binding = 0)

| Index | Írja | Tartalom |
|---|---|---|
| `[0]` | GS | `vec4(count, 0, 0, 0)` — lépések száma |
| `[1..4]` | GS | `mat4` invM (texture2scene) |
| `[5]` | CPU | `vec4(showDebug, showSteps, showEnterExit, showCones)` — config flags |
| `[6]` | CPU | `vec4(showRay, showHitPoint, primitiveID, technique)` — config |
| `[7]` | CPU | `vec4(rayStart, 1)` — debug kamera eye |
| `[8]` | CPU | `vec4(rayStart + rayDir, 1)` — ray irány pontként |
| `[9+]` | GS | `vec4(ui, 1)` × N — lépés pozíciók texture-térben |

`SSBO_PADDING = 9`

### `debugNumericalSSBO` (binding = 1)

| Index | Írja | Tartalom |
|---|---|---|
| `[0]` | GS | `vec4(stepCount, flags, 0, 0)` — flags: bit0=maxSteps, bit1=exitedPrism, bit2=converged |
| `[1]` | CPU | `vec4(eye, 1)` — debug kamera pozíció (világ-tér) |
| `[2]` | CPU | `vec4(eye + dir, 0)` — debug ray iránya pontként |
| `[3..6]` | GS | `mat4` M (scene2texture) |
| `[7]` | GS | `vec4` M_eye (= M × eye) |
| `[8..11]` | GS | `mat4` T (scene2unit) |
| `[12]` | GS | `vec4` T_eye (= T × eye) |
| `[13]` | GS | `vec4(in, 1)` — belépési pont texture-térben |
| `[14]` | GS | `vec4(out, 1)` — kilépési pont texture-térben |
| `[15]` | GS | `vec4(v, 0)` — in → out vektor |
| `[16]` | GS | `vec4(0)` — extra |
| `[17 + i*2]` | GS | `vec4(ti, t, height, tan)` — i-edik lépés |
| `[17 + i*2 + 1]` | GS | `vec4(ui, 0)` — i-edik lépés pozíciója |

**SSBO méret:** mindkét SSBO 512 × `vec4` = 8 KB.

---

## Fájlonkénti változások

### 1. `Shaders/RayMarching/RayMarch_common.glsl`

```glsl
#define SSBO_PADDING 9

#ifdef DEBUG_FUNCTIONS

layout(std430, binding = 0) buffer DebugVisual    { vec4 dbgVis[]; };
layout(std430, binding = 1) buffer DebugNumerical { vec4 dbgNum[]; };

void dbgVisSet(int i, vec4 v) { dbgVis[i] = v; }
void dbgVisSet(int i, mat4 m) { for (int k = 0; k < 4; ++k) dbgVis[i+k] = m[k]; }
vec4 dbgVisGet(int i)         { return dbgVis[i]; }
void dbgNumSet(int i, vec4 v) { dbgNum[i] = v; }
void dbgNumSet(int i, mat4 m) { for (int k = 0; k < 4; ++k) dbgNum[i+k] = m[k]; }
vec4 dbgNumGet(int i)         { return dbgNum[i]; }

int g_debugStepIdx = SSBO_PADDING;

#define DBG_WRITE_STEP(sc, ui, ti, t, h, tn)                             \
    dbgVisSet(g_debugStepIdx + (sc), vec4((ui), 1.0));                   \
    dbgNumSet(17 + (sc) * 2,     vec4((ti), (t), (h), (tn)));            \
    dbgNumSet(17 + (sc) * 2 + 1, vec4((ui), 0.0))

#else

#define dbgVisSet(i, v)
#define dbgVisGet(i)    vec4(0)
#define dbgNumSet(i, v)
#define dbgNumGet(i)    vec4(0)
#define DBG_WRITE_STEP(sc, ui, ti, t, h, tn)

#endif  // DEBUG_FUNCTIONS
```

**`IntersectParams` — változatlan:**

```glsl
struct IntersectParams {
    vec3 enter;
    vec3 exit;
    vec3 cam;
};
```

**`findIntersection_*` — lépésenkénti írás határellenőrzéssel:**

```glsl
if (g_debugStepIdx + stepCount < 512)
    DBG_WRITE_STEP(stepCount, ui, ti, t, height, tan);
```

### 2. `Shaders/RayMarching/Geom_RM.geom`

A fájl legtetején:

```glsl
#define DEBUG_FUNCTIONS
```

A debug blokk az `EndPrimitive()` hívások után — a GS az SSBO config slotokból olvassa a beállításokat:

```glsl
if (dbgVisGet(5).x != 0.0) {  // showDebug
    int cfgPrimID    = int(dbgVisGet(6).z);
    int cfgTechnique = int(dbgVisGet(6).w);

    if (cfgPrimID < 0 || gl_PrimitiveIDIn != cfgPrimID) return;

    bool showEnterExit = dbgVisGet(5).z != 0.0;

    mat4 invM = inverse(M);
    mat4 invT = inverse(T);

    dbgNumSet(3, M);
    dbgNumSet(7, M * dbgNumGet(1));    // M_eye
    dbgNumSet(8, T);
    dbgNumSet(12, T * dbgNumGet(1));   // T_eye
    dbgVisSet(1, invM);                // texture2scene [1..4]

    vec4 p0     = T * dbgVisGet(7);   // debug ray start (unit-tér)
    vec4 p1     = T * dbgVisGet(8);   // debug ray irány pont (unit-tér)
    vec3 rayDir = normalize((p1 - p0).xyz);

    UnitIntersection unitInt = intersectUnitPrism(Ray(p0.xyz, rayDir));
    dbgVisSet(0, vec4(0));

    if (unitInt.found) {
        vec4 pNear = M * invT * (p0 + vec4(rayDir * unitInt.near, 0.0));
        vec4 pFar  = M * invT * (p0 + vec4(rayDir * unitInt.far,  0.0));

        dbgNumSet(13, pNear);
        dbgNumSet(14, pFar);
        dbgNumSet(15, vec4(normalize((pFar - pNear).xyz), 0.0));

        g_debugStepIdx = SSBO_PADDING;
        if (showEnterExit) dbgVisSet(g_debugStepIdx++, pNear);

        vec3 texEye = (M * dbgNumGet(1)).xyz;
        IntersectReturn dbgResult;
        if (cfgTechnique == 0)
            dbgResult = findIntersection_linearSearch(
                IntersectParams(pNear.xyz, pFar.xyz, texEye));
        else
            dbgResult = findIntersection_coneStepMapping(
                IntersectParams(pNear.xyz, pFar.xyz, texEye));

        g_debugStepIdx += dbgResult.stepCount;
        if (showEnterExit) dbgVisSet(g_debugStepIdx++, pFar);

        dbgVisSet(0, vec4(float(g_debugStepIdx - SSBO_PADDING), 0, 0, 0));
        dbgNumSet(0, vec4(float(dbgResult.stepCount), float(dbgResult.flags), 0, 0));
    }
}
```

### 3. `Headers/Types.h` — `RayMarchDebugConfig` és `RayMarchDebugState`

A `RayMarchDebugUniforms` struct helyett egyszerű C++ konfigurációs struct (nincs GPU-layout követelmény):

```cpp
struct RayMarchDebugConfig {
    bool showDebug     = false;
    bool showSteps     = false;
    bool showEnterExit = false;
    bool showCones     = false;
    bool showRay       = false;
    bool showHitPoint  = false;
    int  primitiveID   = -1;
    int  technique     = 0;   // 0=LS, 1=CSM — renderer frissíti GetTechniqueID()-vel
};

struct RayMarchDebugState {
    GLuint debugVisualSSBO    = 0;
    GLuint debugNumericalSSBO = 0;
    Camera debugCamera;
    RayMarchDebugConfig config;
    const RayMarchedModel* target = nullptr;
};
```

### 4. `Interfaces/IRayMarchingTechnique.h`

Egyetlen új virtuális metódus:

```cpp
virtual int GetTechniqueID() const = 0;  // 0=LS, 1=CSM
```

`LinearSearch` visszaad 0-t, `ConeStepMapping` 1-et. A `SetUniforms` szignatúrája **nem változik**.

### 5. Debug kamera kezelése `RenderDebugPanel()`-ben

Nincs új Command osztály. A `RenderDebugPanel()` közvetlenül módosítja a debug kamerát:

### 6. `Headers/MyApp.h` és `Sources/MyApp.cpp`

```cpp
RayMarchDebugState m_debugState;
void InitDebugSSBOs();
void CleanupDebugSSBOs();
void RenderDebugPanel();
```

**`InitDebugSSBOs()`** — az SSBO-k nullásan inicializálódnak, tehát `dbgVisGet(5).x == 0` → debug block nem fut addig, amíg a felhasználó be nem kapcsolja:

```cpp
void MyApp::InitDebugSSBOs()
{
    auto alloc = [](GLuint& ssbo) {
        glGenBuffers(1, &ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 512 * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);
    };
    alloc(m_debugState.debugVisualSSBO);
    alloc(m_debugState.debugNumericalSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
```

**`RenderDebugPanel()`:**

```cpp
void MyApp::RenderDebugPanel()
{
    if (!ImGui::Begin("Ray March Debug")) { ImGui::End(); return; }

    auto& dbg = m_debugState;
    auto& cfg = dbg.config;

    ImGui::Checkbox("Enable", &cfg.showDebug);

    if (cfg.showDebug) {
        ImGui::SeparatorText("Debug Camera");
        glm::vec3 eye = dbg.debugCamera.GetEye();
        glm::vec3 at  = dbg.debugCamera.GetAt();
        if (ImGui::DragFloat3("Eye", glm::value_ptr(eye), 0.01f))
            dbg.debugCamera.SetEye(eye);
        if (ImGui::DragFloat3("At",  glm::value_ptr(at),  0.01f))
            dbg.debugCamera.SetAt(at);
        if (ImGui::Button("Copy Main Camera")) {
            dbg.debugCamera.SetEye(m_camera.GetEye());
            dbg.debugCamera.SetAt(m_camera.GetAt());
        }

        ImGui::SeparatorText("Primitive");
        ImGui::DragInt("Primitive ID", &cfg.primitiveID, 1.0f, -1, 4096);
        ImGui::SameLine();
        if (ImGui::SmallButton("Any")) cfg.primitiveID = -1;

        ImGui::SeparatorText("Show");
        ImGui::Checkbox("Steps",      &cfg.showSteps);     ImGui::SameLine();
        ImGui::Checkbox("Cones",      &cfg.showCones);     ImGui::SameLine();
        ImGui::Checkbox("Enter/Exit", &cfg.showEnterExit); ImGui::SameLine();
        ImGui::Checkbox("Ray",        &cfg.showRay);

        ImGui::SeparatorText("Numerical Data");
        if (dbg.debugNumericalSSBO) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, dbg.debugNumericalSSBO);
            const auto* data = static_cast<const glm::vec4*>(
                glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY));
            if (data) {
                int steps = (int)data[0].x;
                int flags = (int)data[0].y;
                ImGui::Text("Steps: %d   Flags: 0x%02X", steps, flags);
                ImGui::Text("Eye   (%.3f, %.3f, %.3f)", data[1].x, data[1].y, data[1].z);
                ImGui::Text("In    (%.3f, %.3f, %.3f)", data[13].x, data[13].y, data[13].z);
                ImGui::Text("Out   (%.3f, %.3f, %.3f)", data[14].x, data[14].y, data[14].z);

                if (ImGui::BeginTable("Steps", 5,
                    ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, ImVec2(0, 180))) {
                    ImGui::TableSetupColumn("i");
                    ImGui::TableSetupColumn("ti");
                    ImGui::TableSetupColumn("t");
                    ImGui::TableSetupColumn("h");
                    ImGui::TableSetupColumn("ui");
                    ImGui::TableHeadersRow();
                    for (int i = 0; i < steps && (17 + i*2 + 1) < 512; ++i) {
                        glm::vec4 p  = data[17 + i*2];
                        glm::vec4 ui = data[17 + i*2 + 1];
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i);
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%.4f", p.x);
                        ImGui::TableSetColumnIndex(2); ImGui::Text("%.4f", p.y);
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%.4f", p.z);
                        ImGui::TableSetColumnIndex(4);
                        ImGui::Text("(%.3f,%.3f,%.3f)", ui.x, ui.y, ui.z);
                    }
                    ImGui::EndTable();
                }
                glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
            }
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }
    }
    ImGui::End();
}
```

### 7. `Sources/RendererVisitor/OpenGLRendererVisitor.cpp` — `Visit(const RayMarchedModel&)`

Az `OpenGLRendererVisitor` kap egy `const RayMarchDebugState*` pointert (`SetDebugState()` setter — `MyApp::Render()` hívja frame-enként).

A `glUseProgram()` **előtt** — SSBO-k kötése és fejléc feltöltése:

```cpp
if (m_debugState && m_debugState->debugVisualSSBO) {
    const auto& dbg = *m_debugState;
    const auto& cfg = dbg.config;

    // technique ID frissítése a config-ban (GetTechniqueID az aktív technique-től)
    // const_cast szükséges: m_debugState->config nem const (setter nélkül)
    const_cast<RayMarchDebugConfig&>(cfg).technique =
        target.GetTechnique() ? target.GetTechnique()->GetTechniqueID() : 0;

    glm::vec3 eye = dbg.debugCamera.GetEye();
    glm::vec3 dir = glm::normalize(dbg.debugCamera.GetAt() - eye);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, dbg.debugVisualSSBO);
    auto* vd = static_cast<glm::vec4*>(glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY));
    vd[5] = glm::vec4(cfg.showDebug ? 1.f : 0.f,  cfg.showSteps ? 1.f : 0.f,
                      cfg.showEnterExit ? 1.f : 0.f, cfg.showCones ? 1.f : 0.f);
    vd[6] = glm::vec4(cfg.showRay ? 1.f : 0.f, cfg.showHitPoint ? 1.f : 0.f,
                      (float)cfg.primitiveID,   (float)cfg.technique);
    vd[7] = glm::vec4(eye, 1.f);
    vd[8] = glm::vec4(eye + dir, 1.f);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, dbg.debugNumericalSSBO);
    auto* nd = static_cast<glm::vec4*>(glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY));
    nd[1] = glm::vec4(eye, 1.f);
    nd[2] = glm::vec4(dir, 0.f);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, dbg.debugVisualSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, dbg.debugNumericalSSBO);
}

// A technique::SetUniforms szignatúrája változatlan marad
glUseProgram(prog);
VisitModelBase(target);
technique->SetUniforms(target);
```

---

## Vizuális debug render pass (Phase 12)

A `debugVisualSSBO` tartalmának kirajzolása (GL_POINTS + GL_LINES) a `dbgVis[1..4]` invM mátrix alapján, a következő fázisra halasztva.

---

## Nyitott kérdések

1. **`const_cast` a renderer visitorban:** A `cfg.technique` értéke a `target.GetTechnique()->GetTechniqueID()` alapján frissül, de a `m_debugState` const pointer. Megoldás: `RayMarchDebugState*` ne legyen const (a renderer úgyis módosítja az SSBO-t), vagy `mutable` a `technique` mező.
2. **SSBO overflow nagy maxSteps esetén:** A `if (g_debugStepIdx + stepCount < 512)` feltétel védi a vizuális SSBO-t. A numerikus SSBO effektív korlátja: `17 + stepCount*2 + 1 < 512` → `stepCount < 247`. A debug panel jelzi, ha a visszaadott `stepCount == maxSteps`.
3. **`ImplementationDecisions.md`:** A `RayMarchDebugUniforms std430 layout` bejegyzés elavult — a struct nem GPU-struct többé. Frissítendő.

---

## Implementáció sorrendje

1. `IRayMarchingTechnique.h` — `GetTechniqueID()` hozzáadása
2. `Types.h` — `RayMarchDebugConfig` + `RayMarchDebugState` (régi `RayMarchDebugUniforms` törölhető)
3. `RayMarch_common.glsl` — `#ifdef DEBUG_FUNCTIONS` blokk + `DBG_WRITE_STEP` + `findIntersection_*` bővítése
4. `MyApp.h/cpp` — `m_debugState`, `InitDebugSSBOs()`, `CleanupDebugSSBOs()`, `RenderDebugPanel()`
5. `OpenGLRendererVisitor.h/cpp` — `m_debugState` pointer + SSBO feltöltés + kötés
6. `Geom_RM.geom` — `#define DEBUG_FUNCTIONS` + debug blokk (config SSBO-ból olvas)
7. `ImplementationDecisions.md` — elavult bejegyzés frissítése
8. Tesztelés: ismert debug sugár, numerikus táblázat ellenőrzése
9. Phase 12: vizuális debug render pass
