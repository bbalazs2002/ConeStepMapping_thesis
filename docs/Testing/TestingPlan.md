# ConeStepMapping – Tesztelési Terv

> **Cél:** A végleges alkalmazás rendszeres és dokumentált tesztelése, amely alap a szakdolgozat tesztelési fejezetéhez.
>
> **Teszt környezet:** Windows 11, MSVC 2022, OpenGL 4.5, NVIDIA GeForce RTX / Intel UHD
>
> **Státusz jelölések:**  ✅ Átment  ❌ Hibás  ⚠️ Részlegesen  🔲 Nem fut még

---

## 1. Fekete doboz tesztek (Black-box / Funkcionalitás)

Az alkalmazást kizárólag felhasználói felületen és vizuálisan teszteljük. Nincs belső kódbetekintés.

### 1.1 Alkalmazás indítás

| # | Teszt leírása | Elvárt eredmény | Státusz |
|---|---|---|---|
| BB-01 | Alkalmazás elindítása | Ablak megnyílik, ImGui panelek megjelennek, alapértelmezett RayMarchedModel látható a viewportban | 🔲 |
| BB-02 | Kezdeti állapot ellenőrzése | Global / Models / Debug panelok mind elérhetők; Models listában legalább egy "Surface 1" elem | 🔲 |
| BB-03 | OpenGL Debug callback aktiválás | `SetupDebugCallback()` regisztrálva, GL hibák a consolon megjelennek (szándékos `GL_INVALID_ENUM` teszttel verifikálható) | 🔲 |

### 1.2 Kamera kezelés

| # | Teszt leírása | Elvárt eredmény | Státusz |
|---|---|---|---|
| BB-04 | Egér jobb gomb + húzás | Kamera forog a jelenet körül | 🔲 |
| BB-05 | Egér görgő | Kamera közelít / távolodik | 🔲 |
| BB-06 | Középső gomb + húzás | Kamera pan mozgás | 🔲 |
| BB-07 | Ablak átméretezés | Viewport és projekciós mátrix frissül, arányok helyesek | 🔲 |
| BB-08 | F5 — Shader reload | Ctrl+F5 lenyomásra shaderek újra fordulnak, program fut tovább, a kép frissül | 🔲 |

### 1.3 Scene objektum kezelés

| # | Teszt leírása | Elvárt eredmény | Státusz |
|---|---|---|---|
| BB-09 | "Add RayMarched" gomb | Új RayMarchedModel megjelenik a listában ("Surface N") és a viewportban | 🔲 |
| BB-10 | Több RayMarchedModel hozzáadása | Mindegyik egyedi névvel jelenik meg, mind renderelődik | 🔲 |
| BB-11 | OBJ modell betöltés érvényes path-szal | Modell megjelenik a listában és a viewportban textúrákkal | 🔲 |
| BB-12 | OBJ modell betöltés érvénytelen path-szal | Hibaüzenet, crash nincs, alkalmazás folytatódik | 🔲 |
| BB-13 | Modell kijelölése a listából | Az ImGui lista kiemelés látszik; a viewportban narancs wireframe jelenik meg a modell körül | 🔲 |
| BB-14 | Kijelölés törlése ("Deselect") | Wireframe eltűnik, listában nincs kiemelt elem | 🔲 |
| BB-15 | Modell törlése ("Delete Selected") | Modell eltűnik listából és viewportból, memória felszabadul | 🔲 |
| BB-16 | Az utolsó objektum törlése | Üres jelenet, crash nincs | 🔲 |

### 1.4 Transform vezérlők

| # | Teszt leírása | Elvárt eredmény | Státusz |
|---|---|---|---|
| BB-17 | Location X/Y/Z csúszka módosítása | A kijelölt modell valós időben mozog a viewportban | 🔲 |
| BB-18 | Rotation (Euler) vezérlő módosítása | A modell forog a kívánt tengely körül | 🔲 |
| BB-19 | Scale X/Y/Z módosítása | A modell mérete arányosan változik | 🔲 |
| BB-20 | Negatív scale alkalmazása | Modell tükrözött, normálisok is helyesek | 🔲 |

### 1.5 Ray Marching beállítások (RayMarchedModel)

| # | Teszt leírása | Elvárt eredmény | Státusz |
|---|---|---|---|
| BB-21 | Technika váltás: Linear Search → Cone Step Mapping | Rendering módszer vált, vizuális különbség megfigyelhető a step táblában | 🔲 |
| BB-22 | Max Steps csökkentése (pl. 8-ra) | Felerősödnek az artefaktek, lépésszám limit érvényesül | 🔲 |
| BB-23 | Epsilon növelése | Kevésbé pontos metszéspontok, de gyorsabb konvergencia | 🔲 |
| BB-24 | Heightmap váltás másik textúrára | Új heightmap alapján új conemap generálódik, displacement megváltozik | 🔲 |
| BB-25 | "Discard Fragments" toggle | Ki: nem konvergált sugarak fekete/alapszín helyett megjelennek; Be: eltűnnek | 🔲 |
| BB-26 | "Display Non-Converged" toggle | Nem konvergált pixelek piros kiemelése; Be/Ki kapcsolgatás stabil | 🔲 |
| BB-27 | "Show Flags" toggle | Zöld/piros flag vizualizáció a ray marching állapotaihoz | 🔲 |
| BB-28 | "Normal Multiplier" módosítása | Displacement mélység láthatóan arányosan változik | 🔲 |

### 1.6 Global beállítások

| # | Teszt leírása | Elvárt eredmény | Státusz |
|---|---|---|---|
| BB-29 | Light Direction módosítása | Minden RayMarchedModel megvilágítása szinkronban változik | 🔲 |
| BB-30 | "Show Axes" toggle | Koordinátatengelyek megjelennek / eltűnnek | 🔲 |
| BB-31 | Skybox mappa megadása + Apply | Új skybox betöltődik, viewportban megjelenik | 🔲 |
| BB-32 | Érvénytelen Skybox path | Hibaüzenet, előző skybox megmarad, crash nincs | 🔲 |

### 1.7 Debug panel

| # | Teszt leírása | Elvárt eredmény | Státusz |
|---|---|---|---|
| BB-33 | "Show Debug" bekapcsolása | Debug overlay megjelenik a kijelölt pixelen | 🔲 |
| BB-34 | "Show Steps" bekapcsolása | Step pozíciók pontokként vizualizálva | 🔲 |
| BB-35 | "Show Ray" bekapcsolása | Ray iránya megjelenik a debug kamera nézetben | 🔲 |
| BB-36 | Primitive ID filter beállítása | Csak a megadott primitív debuggolódik | 🔲 |
| BB-37 | Debug frame export | Fájl létrejön, strukturált numerikus adatot tartalmaz | 🔲 |
| BB-38 | Debug kamera mozgatása | A debug nézet függetlenül mozog a fő kamerától | 🔲 |

---

## 2. Fehér doboz tesztek (White-box)

### 2.1 Unit tesztek

Az alábbi egységek izoláltan tesztelhetők, OpenGL kontextus **nélkül** (ahol jelezve van):

#### 2.1.1 `Transform` — *nincs GL függőség*

| # | Teszt leírása | Ellenőrzendő | Státusz |
|---|---|---|---|
| UT-01 | `GetMatrix()` dirty flag | Első hívásra számol, második hívásra cache-ből ad vissza (`m_dirty == false`) | 🔲 |
| UT-02 | `SetLocation()` megjelöli dirty | Setter után `IsDirty() == true`, `GetMatrix()` után `IsDirty() == false` | 🔲 |
| UT-03 | `GetWorldMatrix()` szülő nélkül | `GetWorldMatrix() == GetMatrix()` | 🔲 |
| UT-04 | `GetWorldMatrix()` szülővel | Szülő transzlációja beleszámít a gyerek világmátrixába (`parent.T * child.T`) | 🔲 |
| UT-05 | Identitás alapértelmezés | `Transform t; t.GetMatrix() == glm::identity<glm::mat4>()` | 🔲 |
| UT-06 | Kváternió forgatás | 90°-os Y-tengely körüli forgatás után az X-tengely Z irányba mutat | 🔲 |

#### 2.1.2 `MaterialManager` — *nincs GL függőség*

| # | Teszt leírása | Ellenőrzendő | Státusz |
|---|---|---|---|
| UT-07 | Cache hit: ugyanaz az anyag kétszer | `GetOrCreate(mat)` kétszer hívva → ugyanazt a `shared_ptr`-t adja vissza | 🔲 |
| UT-08 | Cache miss: különböző anyag | Más névvel/színnel rendelkező anyag → új `shared_ptr` | 🔲 |
| UT-09 | Weak ptr garbage collection | Ha a visszakapott `shared_ptr` kiengedik, a következő `GetOrCreate` új objektumot hoz létre | 🔲 |

#### 2.1.3 `TextureManager` — *szükséges GL kontextus*

| # | Teszt leírása | Ellenőrzendő | Státusz |
|---|---|---|---|
| UT-10 | Cache hit ugyanarra a path-ra | Ugyanaz a `shared_ptr<Texture>` kerül vissza | 🔲 |
| UT-11 | Flip=true vs flip=false | Ugyanaz a path, különböző flip paraméterrel → **különböző** `Texture` objektum | 🔲 |
| UT-12 | Nem létező fájl | Visszatér (pl. üres/null texture), nem dob kivételt | 🔲 |
| UT-13 | Weak ptr GC | Kiengedett textúra slot újra betöltődik a következő `GetOrLoad`-ra | 🔲 |

#### 2.1.4 `CommandQueue`

| # | Teszt leírása | Ellenőrzendő | Státusz |
|---|---|---|---|
| UT-14 | Push + Execute sorrendje | Két Push után Execute → mindkét parancs az insertion order-ben fut le | 🔲 |
| UT-15 | Execute után queue üres | `Execute()` után újabb `Execute()` nem csinál semmit | 🔲 |
| UT-16 | `Clear()` eldobja a parancsokat | `Clear()` után `Execute()` nem fut le egyik parancs sem | 🔲 |
| UT-17 | Push null-ptr kezelése | `Push(nullptr)` ne okozzon crash-t | 🔲 |

#### 2.1.5 `ModelLoader` — *nincs GL függőség*

| # | Teszt leírása | Ellenőrzendő | Státusz |
|---|---|---|---|
| UT-18 | `RelToAbsPath()` relatív path | Abszolút utat ad vissza `PROJECT_ROOT` prefix-szel | 🔲 |
| UT-19 | `RelToAbsPath()` abszolút path | Változatlanul adja vissza az utat | 🔲 |
| UT-20 | `ResolveTexturePath()` relatív tex | Texfolder-hez relatív path-t ad vissza abszolútan | 🔲 |
| UT-21 | `ResolveTexturePath()` abszolút tex | Változatlanul adja vissza | 🔲 |
| UT-22 | `MergeNormals()` átlagolás | T-junction vertex (két különböző normális, ugyanaz a pozíció): a mergedNormal a két normális normalizált összege | 🔲 |
| UT-23 | `MergeNormals()` output méret | `outputVertices.size() == inputVertices.size()` | 🔲 |
| UT-24 | `MergeNormals()` UV érintetlenség | Minden `texcoord` azonos az inputtal | 🔲 |
| UT-25 | `LoadFromOBJ<Vertex>()` érvényes fájl | `matMesh` nem üres, `materials` kitöltve | 🔲 |
| UT-26 | `LoadFromOBJ<Vertex>()` nem létező fájl | `matMesh` üres, nem dob kivételt | 🔲 |
| UT-27 | `LoadFromOBJ<VertexMergedNorm>()` transformFunc nélkül | `std::runtime_error` kivételt dob | 🔲 |

### 2.2 Integrációs tesztek

Két vagy több egység együttes működése, GL kontextus szükséges.

| # | Teszt leírása | Ellenőrzendő | Státusz |
|---|---|---|---|
| IT-01 | `CreateRMObjectCommand::Execute()` → `SceneManager::Add()` | SceneManager `GetSceneObjects().size()` növekszik 1-gyel | 🔲 |
| IT-02 | `CreateObjObjectCommand::Execute()` → `SceneManager::Add()` | Model megjelenik a listában és renderer visitálható | 🔲 |
| IT-03 | `DeleteObjectCommand::Execute()` → `SceneManager::Remove()` | Objektum eltűnik, `GetSelected()` null ha az volt a kijelölt | 🔲 |
| IT-04 | `SetTechniqueCommand` → `RayMarchedModel::SetTechnique()` | `GetTechnique()->GetName()` a várt technikát adja | 🔲 |
| IT-05 | `SetHeightmapCommand` → `ConemapGenerator::Generate()` → `RayMarchedModel` | `GetConemap() != nullptr` a parancs végrehajtása után | 🔲 |
| IT-06 | `SetLocationCommand` → `Transform::SetLocation()` | `GetWorldMatrix()` az új pozíciót tükrözi | 🔲 |
| IT-07 | Több parancs egy frame-ben | 3 különböző command Push-olva → mind végrehajtódik, sorrendben | 🔲 |
| IT-08 | `SceneManager::Clear()` után Add | Új objektum hozzáadható, crash nincs | 🔲 |
| IT-09 | `ImGuiVisitor` + `SceneManager::RenderGUI()` | Visitor minden ISceneObject-et meglátogat; GUI-változtatás command-ot generál | 🔲 |
| IT-10 | `OpenGLRendererVisitor` + `SceneManager::Render()` | Minden `IModelRendererVisitable` renderelődik; kijelölt model wireframe pass kap | 🔲 |
| IT-11 | `TextureManager` + `ModelLoader` + `CreateModelFromOBJ()` | Textúrák betöltödnek, GL texture ID-k érvényesek, UV helyes (nem tükrözött) | 🔲 |
| IT-12 | Heightmap cache: ugyanaz a fájl két RayMarchedModelben | `TextureManager` cache hit; csak egy GL textúra objektum jön létre | 🔲 |

### 2.3 Rendszertesztek

A teljes pipeline futtatása, valódi GL kontextussal.

| # | Teszt leírása | Ellenőrzendő | Státusz |
|---|---|---|---|
| ST-01 | Teljes frame ciklus: `Init → Update → Render → RenderGUI` | Crash nincs, hibátlan kimenet | 🔲 |
| ST-02 | 1000 egymást követő frame | Stabilitás, nincs memória-növekedés, GL error log üres | 🔲 |
| ST-03 | Shader reload (`ShaderManager::ReloadAll()`) | A programID-k azonosak maradnak, a renderelt kép frissül, crash nincs | 🔲 |
| ST-04 | Debug SSBO frame-reset | Primitive ID filter váltáskor az előző frame adatai nem maradnak visible ("stale data" bug) | 🔲 |
| ST-05 | RayMarchedModel + LinearSearch teljes pipeline | Heightmap → Conemap → Shader SetUniforms → Render; helyes displacement látható | 🔲 |
| ST-06 | RayMarchedModel + ConeStepMapping teljes pipeline | Azonos heightmap, CSM technikával; step count kisebb mint Linear Search-nél | 🔲 |
| ST-07 | Model (OBJ) teljes pipeline | OBJ betölt → textúra betölt (flip=false) → vertex layout Vertex → shader location 2 = texcoord | 🔲 |
| ST-08 | Konzervatív conemap generálás | `ConemapGenerator::SetConservative(true)` után generált conemap konzervatív algoritmust futtat | 🔲 |
| ST-09 | Model törlés + render | Törölt model után `SceneManager::Render()` nem dereferál dangling pointer-t | 🔲 |
| ST-10 | Több RayMarchedModel különböző technikával | Mindkettő helyesen renderelődik, uniformok nem keverednek | 🔲 |

---

## 3. Memória szivárgás tesztek

### 3.1 Eszközök

- **Visual Studio Diagnostic Tools** (Debug → Performance Profiler → Memory Usage): heap snapshot diff
- **Visual Studio Address Sanitizer** (ASAN): `/fsanitize=address` MSVC flag
- **Dr. Memory** (opcionális, Windows): futásidejű heap leak detektor
- **GL eszköz**: `glDeleteTextures` / `glDeleteBuffers` / `glDeleteVertexArrays` hívások ellenőrzése debugger breakpointtal

### 3.2 Tesztek

| # | Teszt leírása | Mit kell ellenőrizni | Státusz |
|---|---|---|---|
| ML-01 | Alkalmazás normális kilépés | Heap snapshot: nincs leak a `new`/`malloc` lefoglalt blokkokban | 🔲 |
| ML-02 | RayMarchedModel életciklus | Create → Add → Delete: `shared_ptr` ref count 0-ra csökken, destruktor lefut | 🔲 |
| ML-03 | `Mesh` GL erőforrások | `Mesh` destruktora hívja `glDeleteVertexArrays` + `glDeleteBuffers` (VAO/VBO/IBO) | 🔲 |
| ML-04 | `Texture` GL erőforrások | `Texture` destruktora hívja `glDeleteTextures`; TextureManager weak_ptr GC után szabad | 🔲 |
| ML-05 | `SceneManager::Clear()` | Minden `shared_ptr` referencia kiengedve (scene objects + interface vectors szinkronban) | 🔲 |
| ML-06 | `TextureManager` cache GC | Expired `weak_ptr`-ek `CollectGarbage()` hívásra törlődnek, nincs "zombie" bejegyzés | 🔲 |
| ML-07 | `MaterialManager` cache GC | Ld. ML-06 — Material cache ugyanolyan weak_ptr mintát követ | 🔲 |
| ML-08 | `ConemapGenerator::m_lastConemap` | `SetHeightmap()` után a generator megtartja a `shared_ptr`-t preview-ra — ez szándékos, de dokumentálandó: **nem leak** | 🔲 |
| ML-09 | CommandQueue `Execute()` utáni clear | `unique_ptr<ICommand>` példányok az Execute végén felszabadulnak | 🔲 |
| ML-10 | Debug SSBO életciklus | `InitDebugSSBOs()` → `CleanupDebugSSBOs()` → GL buffer ID-k 0-ra reszetelve | 🔲 |
| ML-11 | OBJ betöltés hibás fájl | `catch(...)` ágban a részlegesen felépített `Model` shared_ptr kiengedve, GL erőforrás nem szivárog | 🔲 |
| ML-12 | Skybox loader erőforrás | `SkyboxRenderer` destruktora felszabadítja a cubemap textúrát és az egyéb GL objektumokat | 🔲 |

### 3.3 Elvárás

**Elfogadási kritérium:** az alkalmazás normális kilépés után 0 heap leak és 0 GL resource leak mutat. Az ML-08 (`m_lastConemap`) tudatos design döntés, nem hibás.

---

## 4. Futásidő és hatékonyság tesztek

### 4.1 FPS mérés módszertana

- ImGui overlay FPS counter: `1.0f / info.dt`
- Teszt környezet: fix kamera pozíció, fix jelenet (1 db RayMarchedModel), 5 másodperces átlag
- Mérési feltétel: Release build, Vsync kikapcsolva

### 4.2 Technika összehasonlítás

| # | Konfiguráció | Elvárt FPS (indikatív) | Mért FPS | Státusz |
|---|---|---|---|---|
| PERF-01 | LinearSearch, MaxSteps=64 | Baseline | — | 🔲 |
| PERF-02 | ConeStepMapping, MaxSteps=64 | ≥ PERF-01 (CSM átugorja az üres zónákat) | — | 🔲 |
| PERF-03 | LinearSearch, MaxSteps=128 | < PERF-01 | — | 🔲 |
| PERF-04 | ConeStepMapping, MaxSteps=128 | ≈ PERF-02 (cone skip miatt kisebb hatás) | — | 🔲 |
| PERF-05 | LinearSearch, MaxSteps=8 | > PERF-01 (kevés lép, de artefaktek) | — | 🔲 |
| PERF-06 | 4 db RayMarchedModel egyszerre (LSM) | ~PERF-01 / 4 (lineáris skálázás várható) | — | 🔲 |
| PERF-07 | 4 db RayMarchedModel egyszerre (CSM) | > PERF-06 | — | 🔲 |

### 4.3 Részletes profiling

| # | Teszt | Mérési módszer | Elvárás | Státusz |
|---|---|---|---|---|
| PERF-08 | Debug SSBO overhead | FPS debug=OFF vs debug=ON | < 5% teljesítményveszteség debug OFF esetén | 🔲 |
| PERF-09 | Conemap generálás ideje | `std::chrono` wrapper a `Generate()` köré | Mérhető blokkolás (compute shader dispatch), de egyszer fut per heightmap | 🔲 |
| PERF-10 | `Transform::GetMatrix()` dirty flag hatékonyság | Breakpoint számláló: cache hit arány 1000 frame alatt | Megváltozatlan transform esetén 0 újraszámolás | 🔲 |
| PERF-11 | `TextureManager` cache hit arány | Log üzenetek megszámlálása: "Loading texture" vs "Cache hit" | Ugyanaz a textúra csak egyszer töltődik be | 🔲 |
| PERF-12 | `MergeNormals()` futásideje | Chrono mérés komplex OBJ modellen (50k+ vertex) | Elfogadható betöltési idő (< 500ms) | 🔲 |
| PERF-13 | Frame time stabilitás 1000 frame-en | Std. deviáció az átlagtól | < 10% szórás stabil jelenetben | 🔲 |

### 4.4 LinearSearch vs ConeStepMapping tudományos összehasonlítás

Az alábbi mérés a szakdolgozat eredménytáblájához szükséges:

| Heightmap típus | Technika | MaxSteps | Átlag step / pixel (debug SSBO) | FPS |
|---|---|---|---|---|
| Kő textúra (magas frekv.) | Linear Search | 64 | — | — |
| Kő textúra (magas frekv.) | Cone Step Mapping | 64 | — | — |
| Sima heightmap (alacsony frekv.) | Linear Search | 64 | — | — |
| Sima heightmap (alacsony frekv.) | Cone Step Mapping | 64 | — | — |
| Sima heightmap | Linear Search | 128 | — | — |
| Sima heightmap | Cone Step Mapping | 128 | — | — |

> **Megjegyzés:** Az "Átlag step / pixel" érték a `debugNumericalSSBO` step táblájából olvasható ki a debug panelen.

---

## 5. Edge case és határérték tesztek

| # | Teszt leírása | Elvárt eredmény | Státusz |
|---|---|---|---|
| EC-01 | MaxSteps = 0 | Rendering fut (0 lépés = azonnal miss), crash nincs | 🔲 |
| EC-02 | MaxSteps = 1 | Egy lépés, gyenge minőség, de stabil | 🔲 |
| EC-03 | Epsilon = 0.0f | Nagyon pontos metszéspontok, potenciális infinite loop veszély a shaderben — ellenőrizni | 🔲 |
| EC-04 | Scale = 0 az egyik tengelyen | Degenerált modell, nincs crash, normál rendering degradált | 🔲 |
| EC-05 | OBJ fájl anyag nélkül (.mtl hiányzik) | Default szürke anyag, textúra nélkül töltődik be | 🔲 |
| EC-06 | OBJ fájl textúra referenciával, de hiányzó textúra fájllal | Hibaüzenet, default anyag, crash nincs | 🔲 |
| EC-07 | Heightmap nélküli RayMarchedModel renderelése | Nincs crash (conemap null), esetleg default/fekete output | 🔲 |
| EC-08 | 100 db objektum hozzáadása | Stabil működés, GUI scrollozható, renderelés folytatódik | 🔲 |
| EC-09 | `SetParent()` hívása `Transform`-on | Kompilálás közben `#pragma message` warning jelenik meg; futáskor szülő transzformáció alkalmazódik | 🔲 |
| EC-10 | Shader reload hibás shader kóddal | Compiláció sikertelen, hibaüzenet a logban, az **előző** működő program aktív marad | 🔲 |
| EC-11 | Ablak minimalizálása + visszaállítása | Rendering helyreáll, state megmarad | 🔲 |
| EC-12 | Gyors Add/Delete sorozat (10x egymás után) | Stabil, nincs race condition (minden command queued) | 🔲 |

---

## 6. Ismert limitációk (nem tesztelendők)

Ezek dokumentált, tudatos design döntések vagy halasztott implementációk:

| Limitáció | Leírás | Referencia |
|---|---|---|
| `Transform::SetParent()` | Szülő nem tud gyerekeiről; dangling pointer veszély ha a szülő törlődik | `Transform.h` megjegyzések |
| Fényszámítás | `Frag_Model.frag` nem hív fényszámítást — nincs Light rendszer | `Architecture.md` open issues |
| Alpha cutout | Nincs implementálva — előfeltétel a cube-sphere heightmap eszközhöz | Memory: project_deferred_cube_sphere |
| Cube-sphere heightmap | Python script és UV mapping halasztva | Memory: project_deferred_cube_sphere |

---

## 7. Futtatási útmutató

### 7.1 Build konfiguráció

```
Konfiguráció:   Debug (memória tesztek) / Release (teljesítmény tesztek)
Compiler flags: /W4 (Debug), /O2 /DNDEBUG (Release)
ASAN (opcionális): /fsanitize=address a CMakeLists.txt-ben
```

### 7.2 Javasolt tesztsorrend

1. **BB-01–BB-03** — Alkalmazás indítás ✓
2. **UT-01–UT-27** — Unit tesztek (kód-review szintű, részben manuális)
3. **IT-01–IT-12** — Integrációs tesztek (futtatással)
4. **BB-04–BB-38** — Teljes fekete doboz
5. **ST-01–ST-10** — Rendszertesztek
6. **ML-01–ML-12** — Memória profilozás (Debug build, Diagnostic Tools)
7. **PERF-01–PERF-13** — Teljesítmény mérések (Release build)
8. **EC-01–EC-12** — Edge case-ek

### 7.3 Hibák rögzítése

Minden ❌ státuszú tesztnél rögzíteni kell:
- A konkrét hibaüzenetet / hibás viselkedés leírását
- Reprodukciós lépéseket
- Prioritást (Blocker / Major / Minor)
