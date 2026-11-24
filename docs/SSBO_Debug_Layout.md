# Debug SSBO-k — részletes elrendezés

A ray marching debug infrastruktúra két Shader Storage Buffer Object-et használ.
Mindkettőt a `MyApp::InitDebugSSBOs()` hozza létre, és a `CleanupDebugSSBOs()` törli.

Az SSBO-k adategységei `vec4` (16 bájt), kivéve a `debugNumericalSSBO` elejét,
ahol két `uvec4` mező az indirect draw command-okat tárolja.

A slotok egy része a CPU-ról töltődik fel (`OpenGLRendererVisitor::Visit(RayMarchedModel)`
hívja, rajzolás előtt), más részét a geometry shader írja a draw call alatt.
A GS-oldali írásokat az FS-ben a `#define DEBUG_FUNCTIONS` makró hiánya védi —
az SSBO deklarációk és a `DBG_WRITE_STEP` makró csak a GS-be kerülnek bele.

**Szinkronizáció:** draw call és a debug render között **nincs `glMemoryBarrier`**.
Az SSBO-k az előző frame adatait tartalmazzák, ami debug eszköznél elfogadható.
A `DebugRenderer` az indirect parancsokat közvetlenül a `debugNumericalSSBO`-ból
olvassa, így a lépésszámot nem kell a CPU-ra kiolvasni. A buffer inicializáláskor
az indirect slotok `{0, 1, 0, 0}` értéket kapnak, ezért az első frame-ben
0 vertex kerül kirajzolásra (nem garbage).

---

## `debugVisualSSBO` — binding 0

**Méret:** (9 + 512) × 16 bájt = **8 336 bájt** (521 db `vec4`)

A header (slot 0–8) fix célú; a lépés-pozíciók slot 9-től kezdődnek
(`SSBO_PADDING = 9` a GLSL-ben).

| Slot | Írja | Tartalom | Megjegyzés |
|------|------|----------|------------|
| 0 | GS | `vec4(stepCount, 0, 0, 0)` | GS által feljegyzett lépések száma |
| 1 | GS | `invM[0]` | `inverse(M)` 1. oszlopa (textúra→scene) |
| 2 | GS | `invM[1]` | `inverse(M)` 2. oszlopa |
| 3 | GS | `invM[2]` | `inverse(M)` 3. oszlopa |
| 4 | GS | `invM[3]` | `inverse(M)` 4. oszlopa |
| 5 | CPU | `vec4(showDebug, showSteps, showEnterExit, showCones)` | Vizualizációs flag-ek (`0.0` / `1.0`) |
| 6 | CPU | `vec4(showRay, showHitPoint, primitiveID, techniqueID)` | `primitiveID < 0` → GS nem ír; `techniqueID`: 0=LS, 1=CSM |
| 7 | CPU | `vec4(debugCamera.eye, 1.0)` | Debug sugár kiindulópontja (scene-tér) |
| 8 | CPU | `vec4(debugCamera.at, 1.0)` | Debug sugár iránya mint pont (scene-tér) |
| 9 + i | GS | `vec4(uᵢ, 1.0)` | Az *i*-edik lépés pozíciója textúra-térben (`i = 0..stepCount-1`) |

**`invM` szerepe:** a `DebugRenderer` textúra-térből scene-térbe transzformálja
a lépés-pozíciókat, hogy a 3D viewport-ban megjelenítse őket.

---

## `debugNumericalSSBO` — binding 1

**Méret:** 2 × 16 bájt + (28 + 512 × 2) × 16 bájt = **16 864 bájt**

A buffer elején két `uvec4` mező az indirect draw command-okat tárolja
(`std430` layout, vegyes típus a `vec4[]` flexible array előtt):

```
[byte  0..15]  uvec4 indirectSteps  =  { stepCount, 1,         0, 0 }
[byte 16..31]  uvec4 indirectCones  =  { 3,         stepCount, 0, 0 }
[byte 32+   ]  vec4  debugNumerical[]  ← 28 header slot + lépésenként 2 slot
```

Az indirect command formátuma (OpenGL `DrawArraysIndirectCommand`):
`{ count, instanceCount, first, baseInstance }` — mind `GLuint`.

A `debugNumerical[]` flexible array a `Vert_Debug*.vert` shaderekben `debugNumerical[i]`
indexeléssel érhető el (az SSBO deklaráció automatikusan a 32 bájtos prefix utánról
számolja az indexeket).

### Header (debugNumerical[0..27], byte-offset = 32)

| Slot | Byte-offset | Írja | Tartalom | Megjegyzés |
|------|-------------|------|----------|------------|
| 0 | 32 | GS | `vec4(stepCount, flags, 0, 0)` | `flags`: CSM terminációs bitek (ld. lent) |
| 1 | 48 | CPU | `vec4(debugCamera.eye, 1.0)` | Debug kamera pozíciója scene-térben |
| 2 | 64 | CPU | `vec4(debugCamera.at, 0.0)` | Debug kamera iránypont scene-térben |
| 3 | 80 | GS | `M[0]` | `M` (scene→texture) 1. oszlopa |
| 4 | 96 | GS | `M[1]` | `M` 2. oszlopa |
| 5 | 112 | GS | `M[2]` | `M` 3. oszlopa |
| 6 | 128 | GS | `M[3]` | `M` 4. oszlopa |
| 7 | 144 | GS | `vec4(TexEye, 0.0)` | Debug kamera pozíciója textúra-térben (`M × eye`, perspektív osztással) |
| 8 | 160 | GS | `T[0]` | `T` (scene→unit prism) 1. oszlopa |
| 9 | 176 | GS | `T[1]` | `T` 2. oszlopa |
| 10 | 192 | GS | `T[2]` | `T` 3. oszlopa |
| 11 | 208 | GS | `T[3]` | `T` 4. oszlopa |
| 12 | 224 | GS | `vec4(T_eye, 0.0)` | Debug kamera pozíciója unit prism-térben (`T × eye`, perspektív osztással) |
| 13 | 240 | GS | `vec4(TexEnter, 0.0)` | Belépési pont textúra-térben |
| 14 | 256 | GS | `vec4(TexExit, 0.0)` | Kilépési pont textúra-térben |
| 15 | 272 | GS | `vec4(TexDir, 0.0)` | Irányvektor textúra-térben (`normalize(TexExit - TexEnter)`) |
| 16 | 288 | GS | hit: `vec4(uvh, 1.0)` / miss: `vec4(0.0)` | Találati `(u,v,height)` hármas; `.w == 1.0` jelzi a hittet |
| 17 | 304 | GS | `vec4(v0.xyz, 0.0)` | Primitív 0. csúcsának scene-tér pozíciója |
| 18 | 320 | GS | `vec4(v1.xyz, 0.0)` | Primitív 1. csúcsának scene-tér pozíciója |
| 19 | 336 | GS | `vec4(v2.xyz, 0.0)` | Primitív 2. csúcsának scene-tér pozíciója |
| 20 | 352 | GS | `vec4(uv0, 0.0, 0.0)` | 0. csúcs UV-ja |
| 21 | 368 | GS | `vec4(uv1, 0.0, 0.0)` | 1. csúcs UV-ja |
| 22 | 384 | GS | `vec4(uv2, 0.0, 0.0)` | 2. csúcs UV-ja |
| 23 | 400 | GS | `vec4(d, mu)` | Apex-fit: a 0. csúcs felső (extrudált) párja + $\mu$ |
| 24 | 416 | GS | `vec4(e, nu)` | Apex-fit: az 1. csúcs felső párja + $\nu$ |
| 25 | 432 | GS | `vec4(f, rho)` | Apex-fit: a 2. csúcs felső párja + $\rho$ |
| 26 | 448 | GS | `vec4(P_apex, usedApex)` | Legkisebb-négyzetes apex pont + flag (`1.0`=apex módszer, `0.0`=affin fallback) |
| 27 | 464 | GS | `vec4(k, wB, wC, 0.0)` | Apex-fit: a projektív mátrix $k, w_\beta, w_\gamma$ együtthatói |

Slot 17–27 csak akkor tartalmaz érvényes adatot, ha az adott primitívre lefutott a debug
blokk (`showDebug` aktív és `gl_PrimitiveIDIn == cfgPrimID`, vagy `cfgPrimID < 0`).
A 23–26 oszlopok a `d,e,f,P_apex,mu,nu,rho` mezőket **akkor is** tartalmazzák, ha az apex
fit végül elutasításra került (`usedApex = 0.0`) — ekkor a kiszámolt, de elvetett értékeket
mutatják, hogy a guard elutasításának oka (melyik érték esett ki a tartományból) utólag
megfigyelhető legyen. Ha a normálisok már a `determinant(S)` teszten elbuktak (az apex
pont ki sem számolódott), a 23–27 oszlopok mind `0.0`-k, `usedApex = 0.0` mellett.

### Lépés-adatok (debugNumerical[28+], lépésenként 2 slot)

Az *i*-edik lépés (`i = 0..stepCount-1`) adatai a `28 + i×2` és `28 + i×2 + 1`
indexeken vannak (byte-offset = `32 + (28 + i×2) × 16`). Az indexet a GLSL-oldalon a
`DBG_STEP_BASE` makró (`RayMarch_common.glsl`) rögzíti — ha ez változik, a
`Vert_DebugCones.vert` és `Vert_DebugHit.vert` saját, hardkódolt `28`-asait, valamint a
CPU-oldali olvasást (lent) is frissíteni kell.

| Index | Tartalom | Megjegyzés |
|-------|----------|------------|
| `28 + i×2`     | `vec4(tᵢ, t, height, tan)` | `tᵢ`: lépés hossza a sugár mentén; `t`: kumulált paraméter; `height`: mintavételezett magasság; `tan`: kúp félszög tangens (LS-nél `0.0`) |
| `28 + i×2 + 1` | `vec4(uᵢ, 0.0)` | Lépés pozíciója textúra-térben |

### CPU-oldali kiolvasás

`RenderDebugPanel()` (Values tab) és `ExportDebugLog()` `glGetNamedBufferSubData`-val
olvassa ki az adatokat. A byte-offset mindig `2 × sizeof(uvec4) = 32` bájttal kezdődik:

```cpp
// header kiolvasás (28 vec4):
glGetNamedBufferSubData(ssbo, 2*sizeof(glm::uvec4), 28*sizeof(glm::vec4), hdr);

// lépések kiolvasása (readCount × 2 vec4):
glGetNamedBufferSubData(ssbo,
    2*sizeof(glm::uvec4) + 28*sizeof(glm::vec4),
    readCount*2*sizeof(glm::vec4), stepData.data());
```

---

## CSM terminációs flag-ek (`flags`, debugNumerical[0].y)

| Bit | Feltétel | Jelentés |
|-----|----------|----------|
| bit 0 | `stepCount > maxSteps` | Elfogyott a lépéskeret |
| bit 1 | `t >= maxT` | Sugár kilépett a prizmából |
| bit 2 | `ti <= 1e-6` | Konvergált (lépésméret nulla közelbe csökkent) |

Hit (`wasHit = true`) feltétele: bit 2 igaz **és** bit 0 hamis.

---

## A GS-oldali írás menete

```
[draw call előtt — CPU]
  debugVisualSSBO[5..8]                  ← RayMarchDebugConfig flag-ek + debug kamera
  debugNumericalSSBO @ byte 48..79       ← debug kamera eye/at (debugNumerical[1..2])

[GS, minden primitívre — primitív-izoláció]
  if showDebug && cfgPrimID >= 0 && gl_PrimitiveIDIn != cfgPrimID:
    return  ← a teljes primitív geometriáját sem emittáljuk (csak a kiválasztott látszik)

[draw call — GS, a kiválasztott primitívre egyszer]
  if showDebug && (cfgPrimID < 0 || gl_PrimitiveIDIn == cfgPrimID):
    g_debugStepIdx = 0
    findIntersection_*(...)  ← DBG_WRITE_STEP hívások növelik g_debugStepIdx-et
    debugNumericalSSBO[0..1] ← indirectSteps + indirectCones  ← LEGELŐSZÖR
    debugVisualSSBO[0]       ← stepCount
    debugVisualSSBO[1..4]    ← invM
    debugNumericalSSBO[0]    ← stepCount + flags              ← (debugNumerical[0])
    debugNumericalSSBO[3..22] ← M, T, geometriai adatok, primitív csúcsok/UV
    debugNumericalSSBO[23..27] ← apex-fit diagnosztika (d,e,f,P_apex,mu,nu,rho,k,wB,wC,usedApex)

[debug render — DebugRenderer::Render(), barrirer nélkül]
  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, debugNumericalSSBO)
  glDrawArraysIndirect(GL_POINTS, offset=0)            ← indirectSteps
  glDrawArraysIndirect(GL_LINES,  offset=16)           ← indirectCones
  glDrawArrays(GL_LINES, 0, 4/8/16)                   ← ray / hit / enterExit (fix)
```

---

## Kapcsolódó fájlok

| Fájl | Szerep |
|------|--------|
| `Headers/Debug/RayMarchDebugState.h` | SSBO handle-ok + `RayMarchDebugConfig` |
| `Headers/Renderer/DebugRenderer.h` | `DebugRenderer` osztály deklaráció |
| `Sources/MyApp/Debug.cpp` — `InitDebugSSBOs()`, `ExportDebugLog()` | Buffer létrehozás, méretszámítás, indirect command inicializálás, szöveges log-export |
| `Sources/MyApp/GUI.cpp` — `RenderDebugPanel()` | Values tab: élő SSBO-kiolvasás és megjelenítés (köztük az "Apex-fit diagnostics" panel) |
| `Shaders/RayMarching/RayMarch_common.glsl` | SSBO deklarációk (`#ifdef DEBUG_FUNCTIONS`), `DBG_WRITE_STEP` makró + `DBG_STEP_BASE` konstans; `uvec4` prefix + `vec4[]` flexible array |
| `Shaders/RayMarching/Geom_RM_apex.geom` | `#define DEBUG_FUNCTIONS`, primitív-izoláció, apex-fit számítás, debug metszésszámítás, header-írás + `indirectSteps`/`indirectCones` kitöltése. (`Geom_RM_abcd.geom`: a korábbi, mindig-affin technika — ugyanazt a közös `RayMarch_common.glsl`-t és `DBG_STEP_BASE`-t használja, de csak a `[17..22]` slotokig ír; a `[23..27]` apex-fit slotok ennél a technikánál íratlanok maradnak.) |
| `Shaders/Debug/Vert_Debug*.vert` | GPU vertex-pulling: SSBOs-ból olvasnak, `u_viewProj` uniform |
| `Shaders/Debug/Frag_Debug.frag` | Közös fragment shader (`v_col` → kimenet) |
| `Sources/RendererVisitor/OpenGLRendererVisitor.cpp` | CPU config feltöltés + bind (nincs barrier) |
| `Sources/Renderer/DebugRenderer.cpp` | Explicit SSBO bind + state save/restore + draw calls |
