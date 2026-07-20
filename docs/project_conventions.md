---
name: project-conventions
description: Coding conventions, design decisions and gotchas for the ConeStepMapping thesis project
metadata:
  type: project
---

## OpenGL konvenciók

- **DSA everywhere**: `glCreateTextures`, `glBindTextureUnit`, `glTextureParameteri`, `glNamedBufferSubData`, stb. (OpenGL 4.5+)
- `glm::identity<T>()` — NEM `glm::mat4(1.0f)`
- Shader reload: `ShaderManager::ReloadAll()` (Ctrl+F5) — program in-place relinkel, azonos `GLuint` marad; nem `glDeleteProgram`/`glCreateProgram`

## `interface` kulcsszó

A CMakeLists.txt `-Dinterface=struct` definiálja. **Mindig** `interface IX { ... };` és `interface IX;` (forward decl) alakot kell használni — soha nem `class IX` vagy `struct IX`. Oka: MSVC name mangling megkülönbözteti `class X` és `struct X`-et, a makró garantálja az egyezést.

## C++ konvenciók

- Getterek/setterek: inline a headerben; komplex logika .cpp-ben
- `= default` konstruktor: ha minden tagnak van in-class inicializálója
- `inline static` tagok in-class inicializálóval (C++17)
- Forward declaration a headerben, teljes `#include` csak .cpp-ben
- `const&` paraméter minden `Render()` és `Update()` hívásban
- Kommentek: angolul

## Ownership szabályok

- `unique_ptr` = kizárólagos tulajdonos
- `shared_ptr` = megosztott (scene objektumok, technikák, command target-ek)
- raw `*` = nem-tulajdonló megfigyelő (pl. `MyApp` értéktag-re mutató pointer)
- `CommandQueue::Execute()` az `Update()` **első** hívása
- GL objektumokat igénylő tagok: `unique_ptr`-ként, `Init()`-ben konstruálva

## SceneManager / ISceneObject

- `ISceneObject` → `IGUIVisitable` + `GetName()` — ez a SceneManager belépési pontja
- `m_sceneObjects` egyszerre a scene lista és a GUI-visitable lista — nincs külön `m_guiVisitables`
- `ModelBase` megvalósítja `ISceneObject`-et → `Model` és `RayMarchedModel` automatikusan
- `dynamic_pointer_cast<RayMarchedModel>` csak ahol rm-specifikus vezérlők kellenek

## Vertex típusok — KRITIKUS

```
Vertex:           pos(0) | normal(1) | texcoord(2)
VertexMergedNorm: pos(0) | normal(1) | mergedNormal(2) | texcoord(3)
```

- `Model` → `Vertex` (model shader location 2 = texcoord)
- `RayMarchedModel` → `VertexMergedNorm` + `MergeNormals`
- **Ha Model-hez VertexMergedNorm-ot használsz: torzult UV / textúra-hiba**

## OBJ betöltés

- `LoadFromOBJ<Vertex>(path, "./")` — Model-hez
- `LoadFromOBJ<VertexMergedNorm>(path, "./", ModelLoader::MergeNormals)` — RayMarchedModel-hez
- Textúra betöltés: `m_textureManager.GetOrLoad(texPath, false)` — `flip=false` (flip=true tükröz)
- `texFolder = path.parent_path()` — a .mtl-beli textúra útvonalak relatívak az .obj könyvtárához
- `ModelLoader::ResolveTexturePath(texName, texFolder)` → abszolút út

## ShaderManager reload

Minden programot `ShaderManager` tart, consumer-ek csak `GLuint`-ot kapnak. A `ReloadAll()` az ID-t helyben tartja meg — nincs dangling handle.

## TextureManager cache

Kulcs: path string (`GetOrLoad`) vagy 6 arc path `"|"`-val összefűzve (`GetOrLoadCubemap`). Ugyanaz a fájl különböző `flip` értékkel **különböző** Texture objektumot ad vissza (szándékos).

## UML jelölések a docs/-ban

- `(+,+)` = private mező + public getter + public setter
- `(+,!)` = private/protected mező + csak public getter (nincs setter)
- `(+,#)` = public getter + protected setter
- Ez nem standard UML — projekt-specifikus konvenció

## Architectural notation → C++ visibility

- `(+,+)` tagok: `protected` (leszármazottak elérik) + publikus getter/setter
- `(+,!)` tagok: `private`/`protected` + csak publikus getter
- Interface-ek mindig `interface` kulcsszóval

## AxesRenderer

Nincs VAO/VBO — a tengelyek geometriája a vertex shaderbe van égetve (gl_VertexID alapján). Nem kell Mesh, nem kell Build().
