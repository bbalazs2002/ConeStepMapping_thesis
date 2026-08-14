# Implementation Decisions

Tudatos implementációs döntések és az indoklásuk. A szakdolgozat vonatkozó fejezetéhez nyersanyag.

---

## Material — textúra ID float-ba csomagolva

**Hol:** `Material::UploadMaterialToShader()` → `glm::vec4(color, static_cast<float>(textureID))`

**Döntés:** Az anyag diffúz/spekuláris/emissziós színét és a hozzá tartozó textúra OpenGL ID-ját egyetlen `vec4` uniform-ba csomagoljuk (RGB = szín, W = textúra ID). Így egy shader uniform-hívás váltja ki a kettőt.

**Korlát:** A `float` 23-bites mantisszával csak ≈16,7 millióig (2²³) ábrázol egész számot pontosan. Ha az OpenGL driver ennél nagyobb textúra ID-t ad ki, az W komponens értéke pontatlan lesz.

**Miért elfogadható:** OpenGL implementációk jellemzően kis egész számokkal kezdik az ID-kiosztást, és egy alkalmazás élettartama alatt nem szokott 16 millió fölé menni. A projekt hatókörében (egy scene, néhány tucat textúra) ez a határ elérhetetlen.

**Alternatíva:** Külön `uint` uniform a textúra ID-nak, vagy a textúra jelenlétét egy `bool`/`int` uniform jelzi, a kötést pedig mindig explicit `glBindTextureUnit` végzi. Nagyobb kódbase-ben ez a biztonságosabb út.

---

## Texture — move szemantika, copy tiltva

**Hol:** `Texture.h/cpp` — move konstruktor, move assignment, `= delete` copy

**Döntés:** A `Texture` nem másolható, csak mozgatható. A másolás tiltott (`= delete`), mert az OpenGL textúra ID egyedi erőforrás: ha két C++ objektum tárolná ugyanazt az ID-t, a destruktoruk kétszer hívná meg a `glDeleteTextures`-t ugyanarra az ID-ra, ami OpenGL hiba.

A move konstruktor és move assignment az ID *tulajdonjogát* adja át:
- Az átvevő megkapja az ID-t (`m_id = other.m_id`)
- A forrás "kiürül" (`other.m_id = 0`), így a destruktora kihagyja a törlést

**Miért `other.m_id = 0` közvetlen eléréssel:** A `private` hozzáférés C++-ban osztályszintű, nem példányszintű — egy `Texture` metódus bármely `Texture` példány privát tagjához hozzáférhet. Ez nem sérti az OCP-t (ami a viselkedés kiterjeszthetőségéről szól), és nem sérti az egységbezárást sem.

**Miért nincs `Invalidate()` segédmetódus:** Egyetlen erőforrástag (`m_id`) esetén egy külön metódus fölösleges indirekcó lenne. Ha a jövőben több tag kerülne nullázásra (pl. sampler ID), érdemes bevezetni.

**Gyakorlati következmény:** `std::vector<Texture>` működik — a vector átméretezéskor a move operátort használja másolás helyett.

---

## Texture — két konstruktor: fájlból és compute outputhoz

**Hol:** `Texture::Texture(path, flip)` és `Texture::Texture(width, height, internalFormat)`

**Döntés:** A `Texture` osztály két különböző GPU-textúra-létrehozási módot fed le egyazon RAII wrapperben:
- Fájlból töltés: mipmap-generálással, `GL_REPEAT` wrappiggel.
- Compute shader output: egyetlen MIP-szint, `GL_CLAMP_TO_EDGE`, nincs pixeladat — a compute shader írja tele `glBindImageTexture` via `ConemapGenerator`.

**Miért egy osztály:** A két eset életciklusa azonos (OpenGL ID, RAII destruktor, move szemantika), csak a létrehozás módja tér el. Külön osztály felesleges plusz absztrakciót jelentene.

**Korlát:** A hívónak kell tudnia, hogy a compute konstruktorral létrehozott textúrát nem szabad `GL_LINEAR_MIPMAP_LINEAR` filterrel használni (nincs mipmap). Ez implicit elvárás — a `ConemapGenerator` felelőssége betartani.

---

## Material — `s_hasUploadedData` flag a textúra unit védelmére

**Hol:** `Material::ClearMaterialFromShader()` és `UploadMaterialToShader()`

**Döntés:** Az `UploadMaterialToShader` az elején meghívja a `ClearMaterialFromShader`-t, hogy az előző anyag textúra kötéseit feloldja. `ClearMaterialFromShader` az `s_lastTextureTargets` tömbben tárolt unit-okra hív `glBindTextureUnit(..., 0)`-t.

**Probléma:** `s_lastTextureTargets` alapértelmezett értéke `{0, 0, 0, 0}`. Az első `UploadMaterialToShader` hívás előtt ezek az értékek érvénytelenek — ha `ClearMaterialFromShader` lefut, a 0-s texture unit-on lévő bármit feloldja, ami nem az anyagrendszer kötötte oda.

**Megoldás:** `s_hasUploadedData` statikus flag, alapértéke `false`. A `ClearMaterialFromShader` csak akkor fut le ténylegesen, ha már volt legalább egy `UploadMaterialToShader` hívás. Az `UploadMaterialToShader` a targets másolása után állítja `true`-ra.

---

## Debug vizualizáció — SSBO-alapú konfiguráció, uniform struct nélkül (Phase 11)

**Hol:** `Types.h`, `RayMarchDebugState.h`, `OpenGLRendererVisitor`, `Geom_RM.geom`

**Korábbi megközelítés (Phase 10 előtt):** A debug flag-ek (`showDebug`, `showSteps`, stb.) és a sugár adatai egy `RayMarchDebugUniforms` C++ structban éltek, amelyet `glUniform*` hívásokkal töltöttek fel a shaderbe. Ez SSBO-UBO layout problémákat és függőségeket okozott: a `SetUniforms()` paraméterként kellett kapja az `ICamera&`-t, ami összekötötte az `IRayMarchingTechnique` interfészt a kamera-hierarchiával.

**Phase 11 döntése:** Minden debug konfiguráció (flag-ek, primitív ID, téchnique ID, sugár adatok) a `debugVisualSSBO` és `debugNumericalSSBO` SSBO-kba kerül — nincs külön debug uniform struct, nincs `glUniform*` hívás a debug adatokhoz.

```
debugVisualSSBO:
  [5] vec4(showDebug, showSteps, showEnterExit, showCones)  — CPU írja
  [6] vec4(showRay, showHitPoint, primitiveID, technique)   — CPU írja
  [7] vec4(rayStart, 1)                                     — CPU írja
  [8] vec4(rayStart+rayDir, 1)                              — CPU írja
  [0] vec4(count, 0, 0, 0)                                  — GS írja
  [1-4] mat4 invM (texture→scene transzformáció)            — GS írja
  [9+] vec4(ui, 1) × N  (lépések UV pozíciói)               — GS írja
```

**Miért fontos:** Az `IRayMarchingTechnique::SetUniforms()` szignatúrája változatlan marad, az `OpenGLRendererVisitor` a saját `m_debugState*` pointerén keresztül tölti fel az SSBO config slotokat. Nincs kereszt-függőség a téchnique és a kamera között.

**A `RayMarchDebugUniforms` struct eltávolításra került** `Types.h`-ból; a flag-ek `bool`-ként élnek `RayMarchDebugConfig`-ban (csak CPU-oldal, nincs std430 alignment-kényszer).

---

## SetHeightmap — conemap generálás SetHeightmapCommand-ból

**Hol:** `SetHeightmapCommand::Execute()` → `m_surface->SetHeightmap(texture, m_generator)`

**Döntés:** A `SetHeightmap()` opcionális `ConemapGenerator*` paramétert kap. Ha `nullptr`, csak a heightmap változik, a conemap érintetlen marad. Ha nem `nullptr`, a compute shader azonnal lefut és beállítja a conemapet is.

**Miért pointer és nem referencia:** A nullptr eset szándékos — lehetővé teszi, hogy a heightmap előzetesen betölthető legyen conemap-generálás nélkül (pl. ha a generator még nem inicializált, vagy az explicit újragenerálást a felhasználó halasztani akarja).

**Hibakezelés:** Ha a renderelés során a `ConeStepMapping` technique aktív, de `m_conemap == nullptr`, az implementáció hibát logol.

---

## SceneManager — automatikus szétválogatás Add()-ban

**Hol:** `SceneManager::Add(shared_ptr<ISceneObject>)`

**Döntés:** A `SceneManager` nem tárolja a bejövő `shared_ptr<ISceneObject>`-t egyetlen egységes vektorban. Ehelyett az `Add()` híváskor egyszer lefutnak a `dynamic_pointer_cast` hívások, és az objektum azokba a specializált vektorokba kerül, amelyeknek az interfészét megvalósítja:
- `m_sceneObjects` — minden `ISceneObject` bekerül ide (ez egyúttal a GUI-bejárás forrása is, ld. lent)
- `m_updatables` — ha `IUpdatable`
- `m_rendererVisitables` — ha `IModelRendererVisitable`

**Miért:** A per-frame loopok (`Update`, `Render`, `RenderGUI`) így castolás nélkül futnak le — az elemek típusa a cast időpontjában már ellenőrzött. Egy egységes vektor fenntartása és per-frame `dynamic_cast` felesleges indirekcó lenne.

**`m_sceneObjects` mint GUI-lista:** Az `ISceneObject` kibővíti az `IGUIVisitable` interfészt, ezért a `RenderGUI()` loop közvetlenül az `m_sceneObjects` vektort járja be — nincs szükség külön `m_guiVisitables` vektorra.

**Következmény:** A `Remove()` az objektum azonosítását `dynamic_cast<void*>(obj.get())`-tel végzi (a legfelterjesztettebb objektumcímet adja vissza, többszörös öröklés esetén is helyes).

**Invariáns:** Egy `Add()` és egy `Remove()` hívás atomikusan frissíti az összes érintett vektort. Mivel az interfész-megvalósítás C++-ban öröklésből fakad és futás közben nem változhat, a vektorok mindig konzisztensek. Futás közbeni dinamikus hozzáadás és törlés biztonságos, ha `CommandQueue`-n keresztül történik: az `Execute()` az `Update()` legelején fut, mielőtt bármely per-frame loop elindul.

---

## MaterialManager és TextureManager — lazy CollectGarbage a lookup metódusban

**Hol:** `MaterialManager::GetOrCreate()`, `TextureManager::GetOrLoad()` → `CollectGarbage()` hívás a lookup előtt

**Döntés:** A `CollectGarbage()` mindkét managernél `private` — kizárólag a saját lookup metódusuk hívja, a hívás legelején. Külső vezérlőre (pl. `MyApp::Update()`) nincs szükség.

**Miért nem destruktorban:** A domain objektum (`Material`, `Texture`) destruktorában hívni a manager `CollectGarbage()`-t kétirányú függőséget vezetne be, ami körkörös csatolás. A `weak_ptr` szemantikája önmagában helyes — az expired bejegyzések nem okoznak hibás viselkedést.

**Miért nem kell azonnali cleanup delete után:** Az expired `weak_ptr` bejegyzések soha nem adnak vissza lejárt objektumot. A bejegyzés fennmaradása csak elhanyagolható memóriaköltsége van, ami a következő lookup híváskor automatikusan megszűnik.

**Mikor fut:** Kizárólag scene setup idején, amikor egy objektum material-t vagy textúrát rendel hozzá magához. Rendereléskor nem fut — a renderer a már lekért `shared_ptr`-t közvetlenül használja.

---

## ModelBase — enable_shared_from_this az ImGuiVisitor parancsokhoz

**Hol:** `ModelBase` osztálydefiníció — `public std::enable_shared_from_this<ModelBase>`

**Döntés:** A `ModelBase` örökli az `std::enable_shared_from_this<ModelBase>`-t, hogy az `ImGuiVisitor::Visit()` metódusai egy `Model&` vagy `RayMarchedModel&` referenciából `shared_ptr`-t tudjanak előállítani a command konstruktorok számára.

**Miért szükséges:** A `SetLocationCommand`, `SetRotationCommand`, `SetScaleCommand`, `SetMaxStepsCommand`, `SetEpsilonCommand` mind `shared_ptr` célpontot vár. A `Visit()` metódus csak referenciát kap (a visitor interfész szabja meg), ezért a `shared_ptr`-t belülről kell előállítani — ez kizárólag `shared_from_this()` segítségével lehetséges biztonságosan.

**Miért nem raw pointer a comandokban:** `CommandQueue::Execute()` frame elején fut; ha a target közben törlődne (pl. `DeleteObjectCommand`), raw pointer dangling lenne.

**Feltétel:** A `shared_from_this()` kizárólag akkor érvényes, ha az objektum már `shared_ptr` által kezelt. Ez teljesül, mert minden scene objektumot `SceneManager::Add(shared_ptr<ISceneObject>)` vesz át.

**Többszörös öröklés:** A `ModelBase` egyéb interface-alaposztályai (`ISceneObject`, `IGUIVisitable`, stb.) nem öröklik az `enable_shared_from_this`-t, így pontosan egy `enable_shared_from_this` alap szerepel a hierarchiában — ez a C++ szabvány által megkövetelt feltétel.

---

## Texture unit kiosztás — material és ray marching textúrák szétválasztása

**Hol:** `LinearSearch::SetUniforms`, `ConeStepMapping::SetUniforms`, `Material::UploadMaterialToShader`

**Döntés:** A texture unitok két tartományra vannak osztva:
- **0–3**: Material rendszer (diffuse, specular, emission, normal) — a `Material::UploadMaterialToShader` default hívása ezt a tartományt foglalja le
- **4**: Heightmap — mindkét ray marching technique ide köti
- **5**: Conemap — a `ConeStepMapping` technique ide köti

**Miért:** Egy `RayMarchedModel` meshén egyszerre lehet Material (pl. diffúz szín és textúra a felszín megjelenítéséhez) és heightmap/conemap (a ray marching geometriájához). Ha azonos unitokra kötnénk őket, a `Material::UploadMaterialToShader` felülírná a technique által kötött textúrákat (vagy fordítva). A szétválasztás garantálja, hogy a két rendszer egymástól függetlenül működik, és a Material bővítésekor (pl. több textúra) sem keletkezik ütközés.

**Kapcsolódó:** A ray marching shaderekben a `heightmap` sampler unit 4-re, a `conemap` sampler unit 5-re kell kötni (`layout(binding = 4)` vagy explicit `glUniform1i`).

---

## OpenGLRendererVisitor — GL állapotmentés és visszaállítás Visit(Model)-ben

**Hol:** `OpenGLRendererVisitor::Visit(const Model&)`

**Döntés:** A metódus minden esetben — wireframe módtól függetlenül — elmenti a `GL_CULL_FACE` és `GL_POLYGON_MODE` aktuális értékét, explicit beállítja a kívánt állapotot, majd a metódus végén visszaállítja az eredetit.

**Miért mindig:** Ha csak wireframe esetén állítanánk és állítanánk vissza, az előző draw call által hátrahagyott állapot befolyásolhatná a renderelést. Az explicit beállítás minden ágban (wireframe: `GL_LINE` + cull off; solid: `GL_FILL` + cull on) kiszámítható, sorrendtől független viselkedést garantál.

**Core profile korlát:** OpenGL core profile-ban (`3.2+`) a `glPolygonMode` kizárólag `GL_FRONT_AND_BACK` face paramétert fogad el — `GL_FRONT` és `GL_BACK` `GL_INVALID_ENUM` hibát generál. Az előlap és hátlap polygon módja nem állítható be egymástól függetlenül. A visszaállítás szintén `GL_FRONT_AND_BACK`-kel történik.

**Javított hiba:** a `GL_POLYGON_MODE` lekérdezés (`glGetIntegerv`) **mindig 2 értéket ír** (előlap + hátlap módja) — ez akkor is így van, ha a beállítás csak `GL_FRONT_AND_BACK`-kel történhet. Az eredeti implementáció egyetlen `GLint`-be kérdezte le (`glGetIntegerv(GL_POLYGON_MODE, &prevPolygonMode)`), ami 4 bájtos stack-túlcsordulást okozott (MSVC Run-Time Check Failure #2, "Stack around the variable ... was corrupted") — futás közben jelentkezett, amint az első `Model` renderelődni kezdett. A helyes forma: `GLint prevPolygonMode[2]; glGetIntegerv(GL_POLYGON_MODE, prevPolygonMode);`, majd a visszaállításnál `prevPolygonMode[0]`-t használva (a két elem értéke garantáltan azonos, mert csak együtt állíthatók).

---

## Command célpontok — shared_ptr a raw pointer helyett

**Hol:** `SetHeightmapCommand::m_surface`, `SetTechniqueCommand::m_surface`

**Döntés:** A parancsobjektumok `shared_ptr<RayMarchedModel>`-t tárolnak `RayMarchedModel*` helyett.

**Miért:** A `CommandQueue::Execute()` a következő frame elején fut le. Ha ugyanabban a frame-ben egy `DeleteObjectCommand` is bekerül a sorra, és a törlés korábban hajtódik végre, a raw pointer dangling lenne. A `shared_ptr` garantálja, hogy a célpont életben marad a command végrehajtásáig.

**Megjegyzés:** A `DeleteObjectCommand` szintén `shared_ptr<ISceneObject>`-t tárol ugyanezért az okért.

---

## `interface` kulcsszó — forward deklarációk konvenciója

**Hol:** Minden header, amely interfész típust forward-deklarál (pl. `SkyboxRenderer.h`, `AxesRenderer.h`, `OpenGLRendererVisitor.h`, `SetTechniqueCommand.h`, `MyApp.h`)

**Döntés:** Az `interface` kulcsszót nemcsak definícióknál, hanem forward deklarációknál is következetesen alkalmazzuk:

```cpp
// Definíció — mindig interface:
interface ICamera {
    virtual glm::mat4 GetViewMatrix() const = 0;
    // ...
};

// Forward deklaráció — szintén interface, soha nem class:
interface ICamera;
```

**Miért:** A CMakeLists.txt az `interface=class` makrót globálisan definiálja, így az `interface` kulcsszó minden fordítási egységben `class`-szá cserélődik. Az MSVC name mangling megkülönbözteti a `class X` és `struct X` típusokat — ha a forward deklaráció és a tényleges definíció eltérő kulcsszót használ (pl. `struct` vs `class`), a linker eltérő szimbólumnevet lát a deklaráló és az implementáló fordítási egységben, és LNK2019 hivatkozatlan szimbólum hibát dob.

Az `interface IX;` forward deklaráció a makrón keresztül mindig `class IX;`-re cserélődik, így garantáltan egyezik az `interface IX { ... };` definícióból keletkező `class IX { ... };`-sal.

**Miért `class` és nem `struct`:** Az IntelliSense `struct`-ként definiált interfész típusokat tévesen hibásnak jelöli meg. Az `interface=class` makró ezt kiküszöböli, miközben az MSVC name mangling szempontjából is konzisztens marad.

**Szabály:** Minden `I`-vel kezdődő típus (`ICamera`, `IRayMarchingTechnique`, `ISceneObject`, stb.) forward deklarációja `interface IX;` alakú. Egyéb (nem-interface) típusok forward deklarációja változatlanul `class X;`.

---

## Shader-program tulajdonjog — `GLuint`, nem `ShaderManager&`

**Hol:** `SkyboxRenderer`, `AxesRenderer` konstruktorai (korábban `ShaderManager&`-t kaptak, most `GLuint programID`-t — ugyanúgy, mint `LinearSearch`, `ConeStepMapping`, `ConemapGenerator`)

**Döntés:** A GL-erőforrást használó osztályok egyike sem ismeri a `ShaderManager`-t — mindegyik csak egy kész `GLuint programID`-t kap a konstruktorban. A `MyApp::Init()` egyedüli felelőssége az összes `ShaderManager::Load()` hívás és az eredmény ID-k szétosztása a megfelelő osztályoknak.

**Miért:** A `MyApp` ezzel mediátorként működik — ismeri, mely shaderfájlok tartoznak melyik osztályhoz, de maguk az osztályok nem függnek a betöltés mechanizmusától. Korábban `SkyboxRenderer`/`AxesRenderer` `ShaderManager&`-t kapott és maga hívta a `Get()`-et a konstruktorban — ez funkcionálisan azonos eredményt adott (egyszer lekért, cache-elt `GLuint`), csak szélesebb, indokolatlan függőséggel a teljes manager interfészre. Az egységesítés konzisztenssé teszi mind az 5 GL-erőforrást használó osztályt.

**Tulajdonjog:** A `ShaderManager` az egyedüli tulajdonosa a program élettartamának (`DeleteAll()` a `MyApp::Clean()`-ből). Egyik fogyasztó osztály sem hív `glDeleteProgram`-ot — `AxesRenderer` destruktora korábban tévesen ezt tette, ez javítva lett.

---

## `ShaderManager::ReloadAll()` — helyben újralinkelés a `GLuint` stabilitásáért

**Hol:** `ShaderManager::ReloadAll()` (`Ctrl+F5` hot-reload)

**Probléma:** Az eredeti implementáció minden reloadkor `glDeleteProgram` + `glCreateProgram`-mal **új** `GLuint`-ot hozott létre. Minden osztály, amely a régi ID-t cache-elte konstruáláskor (`LinearSearch`, `ConeStepMapping`, `ConemapGenerator`, `SkyboxRenderer`, `AxesRenderer`), az első reload után érvénytelen/törölt handle-t tartott volna — ez némán hibás renderelést okozott volna (vagy GL hibát debug kontextusban).

**Döntés:** A `ReloadAll()` ugyanazt a `GLuint`-ot (`entry.id`) linkeli újra — nem hoz létre újat. A `ProgramBuilder` ugyanúgy csatolja az újonnan fordított shader objektumokat a meglévő program objektumhoz, majd `Link()`-el.

**Miért biztonságos, nincs erőforrás-szivárgás:** A `GLUtils::LinkProgram(id, true)` minden sikeres **és** sikertelen linkelés után is lefuttatja a csatolt shader objektumok `glDetachShader`+`glDeleteShader` takarítását (ez nincs a link-status-hoz kötve). Tehát minden reload körben az adott körben létrehozott shader objektumok törlődnek — nem halmozódnak fel. Maga a program objektum (`entry.id`) tudatosan nem törlődik újra — ugyanazt linkeljük újra, ez szabványos OpenGL hot-reload technika.

**Következmény:** Minden `GLuint programID`-t cache-elő osztály automatikusan helyesen működik tovább `ReloadAll()` után, módosítás nélkül.

---

## `SkyboxRenderer` textúra-tulajdonjog — kész `shared_ptr<Texture>`, nem fájlútvonalak

**Hol:** `SkyboxRenderer` konstruktora (korábban `array<path, 6> faces`-t kapott és maga töltötte be a cubemapet; most `shared_ptr<Texture> cubemap`-et kap)

**Probléma:** Ha a `SkyboxRenderer` maga építi fel a cubemap textúrát a fájlútvonalakból, az megkerüli a `TextureManager`-t — minden más textúra (heightmap, conemap, material textúrák) a `TextureManager::GetOrLoad()` Flyweight cache-én megy keresztül, a skybox textúrája viszont nem lenne se cache-elve, se egységesen kezelve.

**Döntés:** A `Texture` osztály kapott egy harmadik konstruktort: `Texture(const array<path, 6>& faces, bool flip)`, ami `GL_TEXTURE_CUBE_MAP` típusú textúrát épít fel (ugyanazzal a `glCreateTextures`+`glTextureStorage2D`+`glTextureSubImage3D` logikával, amit korábban a `SkyboxRenderer` maga végzett). A `TextureManager` kapott egy `GetOrLoadCubemap(faces, flip)` metódust, ami a 6 fájlútvonalat `"|"` karakterrel összefűzve kulcsként ugyanazt az `m_cache`-t használja, mint `GetOrLoad()`. A `MyApp::Init()` hívja ezt, és az eredmény `shared_ptr<Texture>`-t adja át a `SkyboxRenderer` konstruktorának.

**Miért egy cache-en (`m_cache`) keresztül, nem külön cubemap cache-sel:** A kulcsformátum (hat útvonal összefűzve `"|"`-szal) garantáltan nem ütközik egyetlen `GetOrLoad()` által használt egyetlen-útvonal kulccsal sem — külön map fenntartása felesleges duplikáció lenne.

**Következmény:** A `SkyboxRenderer` már nem hív `glDeleteTextures`-t, nem ismeri az `ImageRGBA`/`ImageFromFile` API-t — tisztán renderelő osztály marad (shader + geometria + egy nem-tulajdonolt textúra-referencia), ugyanúgy, ahogy a `RayMarchedModel` is `shared_ptr<Texture>`-ként tárolja a conemapját.

---

## Kijelölt modell kiemelése — wireframe overlay, Color modul

**Hol:** `OpenGLRendererVisitor::Visit(const Model&)` — extra rajzolási menet; `Model::m_selectedProgramID`; `src/Shaders/Models/Vert_ModelSelected.vert` + `Frag_ModelSelected.frag`

**Referencia:** a felhasználó korábbi projektje (`temp/3D_visualization-master`), ahol a `Model::RenderSelection()` ugyanezt a technikát alkalmazza.

**Döntés:** Ha a renderelt modell egyezik az `m_selected` pointerrel, a `Visit(const Model&)` egy második rajzolási menetet futtat: ugyanaz a VAO/mesh, de `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)`, vastag vonalszélesség (`glLineWidth(3.0f)`), `GL_CULL_FACE` kikapcsolva. A "Selected" shader párja mindössze a Camera és Transform modult használja a vetítéshez, és a Color modult az egységes kiemelő színhez (narancs, `vec3(1.0, 0.6, 0.0)`). Sem stencil buffer, sem csúcspont-kitolás nem kell.

**A Color modul első valódi felhasználója:** a `Frag_ModelSelected.frag` az egyetlen file a projektben, amely ténylegesen `#include`-olja a `Color/Color_uniforms.glsl` + `Color.glsl` modulpárt.

**Program ID-k szimmetriája:** a `Model` tárolja mind a rendes (`m_programID`), mind a kiemelő (`m_selectedProgramID`) program handle-t — azonos lifecycle, azonos betöltési minta (`ShaderManager::Load()` → `MyApp::Init()` → `model->SetSelectedProgram()`). A `SetSelectedProgram()` elhagyható: ha `m_selectedProgramID == 0`, a `Visit()` kihagyja az overlay menetet (silent no-op).

**`SetSelected()` hívásának helye:** `MyApp::Render()` minden frame-ben meghívja az `m_rendererVisitor->SetSelected(selected)` függvényt a `SceneManager::Render()` előtt. Az aktuális kijelölést a `m_selectedIndex` és `m_sceneManager.GetSceneObjects()` alapján számolja ki. Ez egy frame-enkénti, olcsó pointer-szinkronizáció; nem szükséges külön callback vagy Command.

**GL állapotmentés:** a GL `POLYGON_MODE` lekérdezésénél 2-elemű tömböt (`GLint[2]`) használunk a korábbi bugfix ([`OpenGLRendererVisitor — GL állapotmentés`](#openglrenderervisitor--gl-állapotmentés-és-visszaállítás-visitmodel-ben)) nyomán.

---

## `Texture` — cubemap wrap mód és szűrés explicit beállítása

**Hol:** `Texture::Texture(const array<path, 6>& faces, bool flip)` — a cubemap konstruktor

**Döntés:** A cubemap konstruktor explicit `GL_CLAMP_TO_EDGE` wrap módot és `GL_LINEAR` szűrést állít be — a prototípusban ezek nem szerepeltek, a default `GL_REPEAT` maradt érvényben.

**Miért:** Immutable storage-nál (`glTextureStorage2D` 1 mip szinttel) az alapértelmezett `GL_REPEAT` nem okoz textúra-inkomplettséget, de cubemap-mintavételezésnél a `CLAMP_TO_EDGE` a szabványos beállítás a szemközti lapok határán megjelenő varrat-artifaktok elkerüléséhez. Az explicit beállítás egységessé teszi a `Texture` osztály viselkedését: a 2D konstruktorok szintén explicit szűrési és wrap módot állítanak be, nem támaszkodnak OpenGL alapértelmezésekre.

---

## `IRayMarchingTechnique::GetTechniqueID()` — ISP tradeoff, trivial virtual

**Hol:** `Interfaces/IRayMarchingTechnique.h` + `LinearSearch`, `ConeStepMapping`

**Döntés:** A `GetTechniqueID()` metódus az `IRayMarchingTechnique` interfészre kerül (nem kap külön interfészt, nem `dynamic_cast` oldja meg). `LinearSearch` visszatérési értéke `0`, `ConeStepMapping`-é `1`.

**Miért szükséges:** Az `OpenGLRendererVisitor::Visit(const RayMarchedModel&)` a rajzolás előtt feltölti a `debugVisualSSBO[6].w` slotot a téchnique azonosítójával, hogy a debug UI megjelenítse, melyik algoritmus futott. A visitornak nincs más módja lekérdezni ezt — a `RayMarchedModel` csak az interfészen keresztül ismeri a téchnique-ot.

**Alternatívák és elutasításuk:**
- `dynamic_cast<LinearSearch*>`: futásidejű RTTI, törékeny (új téchnique esetén minden call-site frissítést igényel), elkerülhető polimorfizmussal.
- `MyApp`-szintű szinkronizáció `SetTechniqueCommand`-ból: a command már ismeri az új téchnique-ot, beírhatja a debug state-be — de ez mellékhatás a command-ban és nem skálázódik (parallel visitors esetén race condition).
- Külön `ITechniqueIdentifiable` interfész: egyetlen metódust különít le, amit mindkét implementáció úgyis hordoz — overhead a haszonhoz képest.

**ISP megfontolás:** Technikailag az interfész kibővül egy olyan metódussal, amelyet debug kontextuson kívül nem hívnak. Ez elfogadható kompromisszum, mert (a) csak 2 implementáció létezik, (b) a metódus triviális one-liner, (c) az interfész többi metódusával szemben nem változik a szemantika.

---

## `Texture::m_type` — a GL target nyilvántartása ImGui előnézethez

**Hol:** `Texture.h/cpp` — új `m_type : GLenum` tagváltozó, `GetType()` getter

**Döntés:** A `Texture` eltárolja, milyen GL target-tel jött létre (`GL_TEXTURE_2D` vagy `GL_TEXTURE_CUBE_MAP`), és ezt egy publikus `GetType()` getterrel kiadja. Mindhárom konstruktor beállítja a tagváltozó-inicializáló listában (a két 2D-konstruktor `GL_TEXTURE_2D`-t, a cubemap-konstruktor `GL_TEXTURE_CUBE_MAP`-et) — a move konstruktor/assignment is átviszi.

**Miért most:** Jelenleg a `Texture` minden fogyasztója (`Material::UploadMaterialToShader`, `SkyboxRenderer::Render`) típus-agnosztikus marad — a `glBindTextureUnit` nem igényli a target megadását bind időben, mert azt a `glCreateTextures` rögzítette létrehozáskor. A típusinformációra egy tervezett **ImGui textúra-előnézet** funkció miatt van szükség: egy 2D textúra közvetlenül megjeleníthető `ImGui::Image`-dzsel, egy cubemap viszont nem — az előnézet logikájának el kell tudnia dönteni, melyik esetről van szó.

**Miért nem külön enum class:** A `GLenum` közvetlenül a GL target-értéket tárolja (`GL_TEXTURE_2D`/`GL_TEXTURE_CUBE_MAP`), nincs szükség saját enum-ra és az oda-vissza konverzióra — ez konzisztens azzal, ahogy a projekt máshol is közvetlenül GL enumokkal dolgozik (pl. `ShaderStage::type : GLenum`).

---

## `RayMarchDebugState` tulajdonjog — `MyApp`, nem `OpenGLRendererVisitor` (Phase 13)

**Hol:** `MyApp::m_debugState : RayMarchDebugState` (értékszerű tag); `OpenGLRendererVisitor::m_debugState : RayMarchDebugState*` (nem tulajdonló pointer)

**Döntés:** A debug SSBO handle-eket és a `RayMarchDebugConfig` flag-eket egyetlen `RayMarchDebugState` struct fogja össze, amelyet a `MyApp` értékként tárol. Az `OpenGLRendererVisitor` csak egy nem tulajdonló `RayMarchDebugState*` mutatót kap (`SetDebugState()`-en keresztül), és `nullptr`-ellenőrzéssel védetten ír a config slotokba.

**Miért nem a Visitorban:** Az SSBO handle-ek életciklusa az alkalmazáshoz kötődik (a `MyApp::InitDebugSSBOs()` hozza létre, `CleanupDebugSSBOs()` törli), nem a rendererhez. A `DebugRenderer` és a `RenderDebugPanel()` is hozzáférnek a state-hez — ha a Visitorban élne, ez körkörös vagy fölösleges függőséget okozna. Az alkalmazásban tárolt, megosztott mutatóval szétoszható minimális jogokkal.

---

## Debug render szinkronizáció — barrier elhagyása, egy frame késés (Phase 13)

**Hol:** `OpenGLRendererVisitor::Visit(const RayMarchedModel&)` — `glMemoryBarrier` törölve; `DebugRenderer::Render()` — nincs szinkronizációs hívás

**Döntés:** A GS által az SSBO-ba írt adatokat (lépés-pozíciók, metadata, indirect command-ok) a `DebugRenderer` szinkronizáció nélkül olvassa a következő frame-ben. Az SSBO-k az előző frame adatait tartalmazzák — ez egy frame látencia a debug megjelenítésben.

**Miért elfogadható:** A debug eszköz célja az algoritmus megértése, nem valós idejű pontosság. Egy frame késés vizuálisan észrevehetetlen álló kamera mellett, és nem rontja az értelmezhetőséget. A `glMemoryBarrier` elhagyása elkerüli a CPU-GPU szinkronizációs pontot (a driver befejeztetheti a GPU munkát a főszálon), ami mérhetően csökkenti a frame-időt.

**Első frame biztonsága:** Az `InitDebugSSBOs()` az indirect command slotokat `{0, 1, 0, 0}` értékre inicializálja. Így az első frame-ben 0 vertex kerül kirajzolásra — nem garbage count alapján futnak a draw hívások.

**Alternatíva (elutasítva):** `glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT)` a draw call után biztosítaná a friss adatokat, de szinkronizációs buborékot szúr be a renderelési menetbe — pontosan az a teljesítményköltsége, amit el szerettünk volna kerülni.

---

## Indirect rendering a debug draw call-okhoz — GPU-oldali lépésszám (Phase 13)

**Hol:** `DebugRenderer::Render()` — `glDrawArraysIndirect`; `debugNumericalSSBO` — `uvec4 indirectSteps` + `uvec4 indirectCones` a buffer elején; `Geom_RM.geom` — GS írja a command mezőket

**Döntés:** A `glDrawArrays(GL_POINTS, 0, stepCount)` hívás helyett `glDrawArraysIndirect`-et használunk, ahol a draw paramétereket (`{count, instanceCount, first, baseInstance}`) maga a GS írja a `debugNumericalSSBO` elejére. A CPU a lépésszámot nem olvassa ki az SSBO-ból.

**Mechanizmus:** A `debugNumericalSSBO` egyszerre kötődik `GL_SHADER_STORAGE_BUFFER`-ként (GS ír bele, debug VS olvas belőle) és `GL_DRAW_INDIRECT_BUFFER`-ként (a `glDrawArraysIndirect` onnan olvassa a draw paramétereket). OpenGL ezt explicit tiltja — egy buffer object egyszerre több target-hez köthet.

```
indirectSteps @ offset  0: { stepCount,     1, 0, 0 }
indirectCones @ offset 16: { 3, stepCount, 0, 0 }
```

**Miért instanced `GL_LINE_STRIP`:** Minden kúp egy önálló instance — 3 vertex (bal szárpont → apex → jobb szárpont) GL_LINE_STRIP-ként. Ezzel az apex nem duplikálódik (4 vertex helyett 3), és az instance-ok között nincs összekötő él. Az indirect command: `{count=3, instanceCount=stepCount, 0, 0}`.

**Teljesítmény:** A `glGetNamedBufferSubData` (teljes CPU-GPU stall) kikerül a renderelési útvonaláról. Ehelyett az SSBO-ba írt integer közvetlenül az indirect buffer `count` mezőjeként kerül felhasználásra, nulla CPU érintéssel.

---

## `uvec4` prefix a `debugNumericalSSBO`-ban — vegyes típusú SSBO (Phase 13)

**Hol:** `RayMarch_common.glsl` — SSBO deklaráció; `MyApp::InitDebugSSBOs()` — bufferméret és inicializálás; `OpenGLRendererVisitor` — CPU írási offset +32 bájt

**Döntés:** Az indirect command mező nem `vec4`-ként kerül a `debugNumerical[]` flexible array-be, hanem külön `uvec4` tagként, a `vec4[]` előtt:

```glsl
layout(std430, binding = 1) buffer DebugNumericalSSBO {
    uvec4 indirectSteps;   // offset  0
    uvec4 indirectCones;   // offset 16
    vec4  debugNumerical[]; // offset 32 — flexible array
};
```

**Miért `uvec4` és nem `vec4`:** Az indirect buffer `count` mezője `GLuint` (unsigned integer). Ha `float`-ként tárolnánk és a GS `uint` értéket ír bele `floatBitsToUint` nélkül, a GPU az integer bitek float-ként értelmezné és fordítva — a draw hívás garbage `count`-tal futna. A `uvec4` garantálja, hogy a GPU és a CPU ugyanazokat a bitmintákat integer-ként értelmezi.

**std430 mixed-type layout:** Az OpenGL std430 szabvány engedélyezi vegyes skalártípusok (`uint`, `float`) kombinálását egy bufferben, ha az igazítási szabályokat betartjuk. Az `uvec4` 16 bájtosan igazított, a `vec4[]` flexible array szintén — nincs padding szükséges közöttük.

**Következmény a CPU-oldalon:** Minden `glNamedBufferSubData` és `glGetNamedBufferSubData` hívás a `debugNumerical[]` elemekre `+32 bájt` offsettel dolgozik (2 × `sizeof(glm::uvec4)`).

---

## Több debug vertex shader `u_mode` uniform helyett (Phase 13)

**Hol:** `Shaders/Debug/` — 5 különálló VS + 1 közös FS; `DebugRenderer` — 5 program handle

**Döntés:** Az 5 debug geometriatípus (sugár, belépési/kilépési pontok, lépések, kúpok, találati pont) mindegyike külön vertex shadert kap. Egyetlen "god shader" `u_mode` uniformmal nincs.

**Miért:** GPU-oldali feltételes elágazás (`if (u_mode == ...)`) a vertex shaderben minden vertex-re lefut, és megakadályozza a speciális GPU-hardver optimalizálásokat (pl. early-out, predicated execution). Külön programok esetén a driver pontosan a szükséges kódot futtatja — nincs felesleges ág.

**Memóriaköltség:** 5 program 1 helyett több GPU memóriát foglal. Ez elhanyagolható a debug mód körülményei között (a debug render csak akkor fut, ha `showDebug == true`), és a szakdolgozatban demonstrálja a teljesítmény-tudatos shader-szervezési elveket.

**Közös fragment shader:** Az összes debug geometria azonos fragment shaderrel renderelődik (`Frag_Debug.frag`): `v_col` bemeneti változó → `vec4(v_col, 1.0)` kimenet. A szín meghatározása a vertex shaderek feladata — nincs duplikált FS kód.

---

## GPU vertex-pulling a debug shaderekben (Phase 13)

**Hol:** `Vert_DebugSteps.vert`, `Vert_DebugCones.vert`, `Vert_DebugRay.vert`, stb. — nincs `layout(location = N) in` attribútum; `DebugRenderer` — üres VAO

**Döntés:** A debug vertex shadereknek nincs egyetlen vertex attribútuma sem. Pozíciót, színt és egyéb adatokat mind `gl_VertexID` alapján indexelik közvetlenül az SSBO-kból:

```glsl
vec3 texPos = debugVisual[9 + gl_VertexID].xyz;   // lépés pozíciója
```

A `DebugRenderer` egy üres VAO-val (`glCreateVertexArrays`) köt be, ami kötelező OpenGL core profile-ban (legalább egy VAO kell kötve lenni), de attribútum-layout nincs hozzárendelve.

**Miért:** A debug geometria struktúrája frame-ről frame-re változik (stepCount), és az SSBO-ban már rendelkezésre áll — külön VBO feltöltése és attribútum-pointer beállítása redundáns lenne. A vertex-pulling emellett elkerüli a CPU→GPU adatátvitelt a debug geometriához.

**Kapcsolódik:** Az indirect rendering ([ld. fent](#indirect-rendering-a-debug-draw-call-okhoz--gpu-oldali-lépésszám-phase-13)) és a vertex-pulling együtt biztosítja, hogy a debug render CPU-oldali munkája minimális: csak program-switch + uniform (`u_viewProj`) + draw call.

---

## V alakú kúpábrázolás körök helyett (Phase 13)

**Hol:** `Vert_DebugCones.vert` — 3 vertex lépésenként, instanced GL_LINE_STRIP; a kúp sugara és iránya textúra-térből számítódik

**Döntés:** A cone step mapping kúpait nem teljes kör kerületként, hanem V alakú törtvonaként jelenítjük meg: az apex (a felszín magassága az aktuális UV pozícióban) és két szárpont (a sugár irányában ±R távolságra) által alkotott két vonalszakasz.

```
apex:      (xy,  h)           — textúra-térben
bal szár:  (xy - uvDir·R, z)  — ahol R = (z - h) / tan
jobb szár: (xy + uvDir·R, z)
```

**Miért nem kör:** A kúp kör-keresztmetszete a textúra UV síkján (z=const sík) lenne helyes geometriailag. Azonban a debug render célja az algoritmus megértése — a kúp félszög és a lépésirány közötti viszony azonnal leolvasható a V alakból anélkül, hogy N szegmenses körzetet kellene renderelni. A V egyszerűbb, gyorsabb és jól megkülönböztethető a lépés-pontoktól (GL_POINTS) és a sugártól (folyamatos vonal).

**Textúra-tér vs. scene-tér:** A kúp számítása textúra-térben történik, majd az `invM` mátrixon (textúra→scene transzformáció, GS írja az SSBO-ba) keresztül kerül a scene-térbe. A CPU nem ismeri a transzformációt — a shaderben van az egész számítás.

---

## `ISceneObject` — `IDrawable` felváltása egységes scene object interfésszzel

**Hol:** `Interfaces/ISceneObject.h`; `SceneManager`, `ModelBase`, `CommandQueue` minden érintett paramétere

**Döntés:** A korábbi `IDrawable` interfész eltávolításra került; helyét az `ISceneObject` vette át mint az elsődleges scene object interfész. Az `ISceneObject` kiterjeszti az `IGUIVisitable`-t, és egyetlen kötelező metódust deklarál: `GetName() : const string&`.

**Miért:** Az `IDrawable` félrevezető név volt — a jelölt osztályok egy részét nem közvetlenül a SceneManager "rajzolta", hanem a Visitor mintán keresztül. Az `ISceneObject` pontosabban tükrözi a szerepet: "a scene-ben nyilvántartott, névvel rendelkező entitás, amelynek GUI-megjelenítése van." Az `IGUIVisitable` beépítése (extend, nem delegate) lehetővé teszi, hogy `m_sceneObjects` egyidejűleg szolgáljon scene-lista és GUI-bejárási forrás gyanánt — nincs szükség párhuzamos `m_guiVisitables` vektorra.

**Következmény:** `SceneManager::Add()` és `Remove()` paramétere `shared_ptr<ISceneObject>`; a `DeleteObjectCommand` szintén `shared_ptr<ISceneObject>`-t tárol. A `ModelBase` az `ISceneObject`-ből örököl (a korábbi `IDrawable` helyett).

---

## `MyApp` — nyers pointer és explicit életciklus-kezelés (`main.cpp`)

**Hol:** `main.cpp` 31. sor: `IGraphicsApp* app;` globális nyers pointer; 120. sor: `app = new MyApp();`; 230–231. sor: `app->Clean(); delete app;`

**Döntés:** Az alkalmazásobjektum nyers pointerrel (`IGraphicsApp*`) van kezelve, `unique_ptr` helyett.

**Miért:** Az életciklus sorrendje kötött és nem triviális:

1. `_CrtSetDbgFlag` → mem-leak detektálás bekapcsolva (37. sor)
2. SDL + GL context inicializálás (43–92. sor)
3. `app = new MyApp()` + `app->Init()` (120–122. sor)
4. Fő eseményhurok
5. **`app->Clean()`** — explicit GL erőforrás-felszabadítás (230. sor)
6. **`delete app`** (231. sor)
7. ImGui leállítás, `SDL_GL_DestroyContext`, `SDL_DestroyWindow` (235–240. sor)

A `Clean()` OpenGL hívásokat tartalmaz (`glDeleteBuffers`, `glDeleteProgram`, stb.), amelyeknek a GL kontextus megsemmisítése **előtt** kell lefutniuk. Ha `unique_ptr`-t használnánk, az automatikus destruktor a scope elhagyásakor fut — ami a leállítási blokk előtt vagy után következhet be a scope szerkezetétől függően. A `reset()` explicit hívása a leállítás sorrendbe illesztéséhez ugyanolyan „manuális" beavatkozás, mint a raw pointer, de kevésbé olvasható.

**`_CrtDbg` és a `delete` kapcsolata:** A `_CRTDBG_LEAK_CHECK_DF` flag hatására az MSVC runtime a program leállásakor (az összes statikus destruktor után) összesíti a szivárgásokat. Az explicit `delete app` garantálja, hogy a `MyApp` destruktora és a `Clean()` a normális program-flow részeként fut le, nem a `_CrtDumpMemoryLeaks` után — így a riport pontosan a valódi szivárgásokat mutatja, nem az alkalmazás-életciklus végéig szándékosan élő objektumokat.

**Globális változó:** Az `app` globálisan van deklarálva. Ez eredetileg az esemény-callback-ek miatt volt szükséges (ahol nincs hozzáférés a `main` lokális változóihoz); a jelenlegi eseményhurok `main`-en belül fut, így technikailag lokális változó is lehetne. Refaktorálás nem történt, mert a viselkedés azonos.

---

## `RayMarchedModel` — a heightmap nem tárolt, csak a conemap

**Hol:** `RayMarchedModel::SetHeightmap()` → `ConemapGenerator::Generate()` → `m_conemap`; nincs `m_heightmap` tag

**Döntés:** A `SetHeightmap()` hívás azonnal elindítja a conemap-generálást (ha `ConemapGenerator*` meg van adva), majd a heightmap textúra `shared_ptr`-t elveti. A `RayMarchedModel` kizárólag a kész conemapet tárolja (`m_conemap : shared_ptr<Texture>`).

**Miért:** A heightmap a conemap előállításának bemenete, nem a renderelés bemenete. A `ConeStepMapping` technique a conemapet mintavételezi, nem a heightmapet — a heightmap megtartása csak plusz GPU-memória lenne felesleges hozzáféréssel. Ha a felhasználó újra generálni akar (pl. paraméter változtatás után), a heightmapet a `TextureManager` cache-éből töltjük vissza.

**Megjegyzés:** A `LinearSearch` technique-hoz a heightmap valóban szükséges lenne (a technique közvetlenül a heightmapet mintavételezi). Ez ismert hiányosság — `LinearSearch` aktív technique esetén a `SetHeightmap()` hívásnak meg kellene tartania a heightmapet is. Jelenlegi állapotban `LinearSearch`-höz is csak conemap generálás után működik a ray marching, amit az `Architecture.md` "Known open issues" táblája rögzít.
