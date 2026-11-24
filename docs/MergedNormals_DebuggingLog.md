# Egyesített normálisok — apex-alapú javítás: hibakeresési napló

Ez a dokumentum az apex-alapú projektív korrekció (ld. `ImplementationPlan_MergedNormals.md`)
tényleges shader-szintű implementálása során feltárt hibákat, azok diagnózisát és javítását
rögzíti, időrendben. Célja, hogy később — a dolgozat szövegezésekor vagy egy esetleges
regresszió esetén — vissza lehessen keresni, *miért* néz ki úgy a kód, ahogy, és *mit* nem
fedett le ez a kör.

A munka két staging-mappában zajlott: `temp/Phase34` (a teljes iterációs történet, minden
próbálkozással együtt) és `temp/Phase35` (tiszta pillanatkép a végső, működő állapotról, hogy
ne kelljen a Phase34 zajos történetében keresgélni). A `src/`-be másolás a szokásos munkafolyamat
szerint külön, manuális lépés — ennek a napló írásakori státusza **nem ellenőrzött**.

---

## 1. Matematikai alap (előzmény, nem ebben a körben)

A teljes levezetés a `docs/ImplementationPlan_MergedNormals.md`-ben található. Röviden:

- $P_{apex}$: legkisebb-négyzetes "legközelebbi pont 3 egyenes­hez" illesztés a három
  csúcs normálisegyenesére.
- Apex-középpontú koordináták: $(\alpha,\beta,\gamma) = L^{-1}(X-P_{apex})$, affin lépés.
- $\mu,\nu,\rho$: a felső csúcsok ($d,e,f$) radiális helyzete az apexhez képest, az
  alapcsúcsokhoz viszonyítva — **projekciós** definícióval (nem egzakt kollinearitással,
  mert a legkisebb-négyzetes apex általában nem esik pontosan mindhárom normálisegyenesre).
- Felfedezés (még a jelen napló előtt): a felső lapnak **nem kell megdőlnie** — egy rögzített
  $y=1$ tetősík mellett is létezik egzakt projektív $R$ mátrix tetszőleges $\mu,\nu,\rho$-ra.
  Ez egyszerűsítette az implementációt (`intersectUnitPrism` változatlan maradhatott).
- Ebből következik: $M$ és $T$ **valódi projektív** mátrixok lettek (nem affinok, mint a régi,
  egy-sarokpontos konstrukciónál) — minden pont-transzformációnál explicit perspektív osztás
  (`perspectiveDiv`) kell.

**Ez a projektivitás az összes ebben a naplóban leírt hiba közös gyökere** — a legtöbb hiba
abból fakadt, hogy egy-egy kódrészlet még az affin-korszakból származó feltételezéssel élt
(pl. hogy interpoláció és mátrix-alkalmazás felcserélhető, vagy hogy "bent van-e a kamera"
egyszerűen eldönthető), ami affin leképezésnél igaz, projektívnél nem.

---

## 2. Megtalált és javított hibák (időrendben)

### 2.1 Ill-kondicionált apex-illesztés (guard bővítés)

**Tünet:** feltételezett, de konkrétan nem megfigyelt kockázat — $\mu\to1$ vagy
$\nu,\rho\to0$ esetén $k,w_\beta,w_\gamma$ elszállhat.

**Javítás:** a meglévő (majdnem-párhuzamos normálisok esetére szolgáló) affin fallback
kiegészült egy második feltétellel: $k,w_\beta,w_\gamma \in (10^{-3},10^3)$ — ezen kívül
affin konstrukcióra esünk vissza.

**Megjegyzés:** ez a guard **létezik**, de egy később felfedezett, sokkal súlyosabb hiba
(ld. 2.7, "kamera bent van" teszt, és 2.8, `gs_out_tex` vs `M(p)`) miatt **nem derült ki
megbízhatóan, hogy ez a guard önmagában elég szűk-e**. Lásd az 5. szakaszt.

### 2.2 Téves hipotézis: "prizma megcsavarodása"

Egy gömb-tesztnél (Föld-magasságtérkép) a pólusok/varrat környékén torzítás és a tüskés
magasságtérkép mellett szórt artifaktok jelentek meg. Első hipotézis: a befoglaló prizma
felső lapja "önmagába csavarodik" szélsőségesen eltérő normálisoknál.

**Cáfolat (a felhasználó mutatott rá):** egy *konvex* gömbnél a normálisegyenesek mindig a
felszín *mögött* (a középpont felé) metszik egymást, sosem előtte — ez a hibamód
strukturálisan nem fordulhat elő. A hipotézist elvetettük.

**Második cáfolat:** a hiba a **mindig-affin** (apex-korrekció nélküli) konstrukcióval is
megjelent — tehát nem is lehetett az apex-mátrix numerikus problémája.

A tényleges okok (varrat/pólus-topológia, `gs_out_tex` inkonzisztencia) csak később, egyedi
primitívek izolálásával derültek ki (ld. 2.6, 2.8).

### 2.3 Debug-eszköz: egyetlen primitív izolálása

**Probléma:** a debug-szonda (külön debug-kamerából induló egyetlen sugár) nem feltétlenül
esik egybe azzal a pixellel, ahol a vizuális hiba látszik — nehéz volt beazonosítani, *melyik*
háromszög a hibás.

**Megoldás:** a geometry shader `main()`-jének elején egy korai `return`, ha `showDebug` aktív
és egy konkrét `Primitive ID` van kiválasztva, de `gl_PrimitiveIDIn` nem egyezik — ekkor a
teljes modellből **csak a kiválasztott primitív geometriája** jelenik meg. Ez tette lehetővé,
hogy a felhasználó vizuálisan "végigsöpörjön" a pólus környéki háromszögeken.

Nem igényelt új SSBO-slotot vagy CPU-oldali változást (a meglévő `Primitive ID` mezőt
használja fel).

### 2.4 Debug-adatok bővítése: apex-fit diagnosztika

**Cél:** a $\mu,\nu,\rho,k,w_\beta,w_\gamma,P_{apex},d,e,f$ és a "melyik ágat használta"
flag közvetlen megfigyelhetővé tétele egy kiválasztott primitívre.

**Megoldás:** 5 új, tömörített `vec4` slot a `debugNumerical` SSBO-ban (`[23..27]`,
`vec3+skalár` csomagolással), emiatt a lépésenkénti adatok bázis-offsetje `23`→`28`-ra tolódott.
Érintett fájlok: `RayMarch_common.glsl` (`DBG_STEP_BASE` konstans), `Vert_DebugCones.vert`,
`Vert_DebugHit.vert`, valamint CPU-oldalon `Sources/MyApp/Debug.cpp` (buffer méret/offset +
új dump-szekció) és `Sources/MyApp/GUI.cpp` (ugyanaz az offset-javítás + új "Apex-fit
diagnostics" panel).

**⚠️ Ez a réteg mára inkonzisztens a `docs/SSBO_Debug_Layout.md`-vel** — ld. 5. szakasz.

### 2.5 Epszilon a `ti < 0.0` feltételnél

**Tünet (felhasználói kérdés):** `ti = dz/denom` lebegőpontos osztás; majdnem-konvergenciánál
(`dz≈0`) egy icipici kerekítés átbillentheti `ti`-t enyhén negatívba, holott a valós érték
0 vagy enyhén pozitív lenne — a kód ezt tévesen "rossz irány, nincs metszéspont" jelzésként
kezelte.

**Javítás:** `if (ti < -1e-6) return no-hit;` majd `ti = max(ti, 0.0);` — csak az érdemben
negatív eseteket kezeli hibaként, a zajszint alattiakat nullára fogja, hagyva hogy a normál
konvergencia-teszt döntsön.

### 2.6 "Túllövés a prizma határán" — **négy egymást követő próbálkozás**

Ez volt a legtöbb iterációt igénylő probléma. A kúp-lépés mérete csak a lokális
biztonsági sugártól függ, nem tudja, milyen közel van a sugár a prizma tényleges
kilépéséhez (`maxT`) — vékony/rövid prizmáknál egyetlen lépés simán átugorhatja a teljes
érvényes ablakot.

1. **Clamp `maxT`-re, teszt ott:** `ti = min(ti, maxT-t)`, majd a szokásos
   `ui.z<=tex.r` teszt a (most pontosan a határra eső) ponton.
   → **Eredmény:** egypixeles "hóesés"-szerű zaj jelent meg szórtan a gömb körül.
   Ok: a határpontnak nincs semmilyen kúp-garanciája, szomszédos pixelek (eltérő
   subpixel-sugár) más-más eredményre juthatnak ugyanazon a bizonytalan ponton.

2. **Ugyanaz a clamp, de a teszt kihagyása a clampelt lépésnél** (`reachedExit` flag).
   → **Eredmény:** a zaj **nem szűnt meg** — a probléma nem a tesztben, hanem magában a
   clampelésben/ezen a ponton landolásban volt.

3. **Szabadon hagyott (nem clampelt) lépés; `t>=maxT` esetén azonnali, feltétel nélküli
   no-hit, mielőtt bármit mintáznánk odakint.**
   → **Eredmény:** a zaj és a legtöbb "tüske"-szerű artifakt eltűnt. **De**: debug-szondával
   igazolhatóan **jogos találatokat is elveszítettünk** — ha az érvényes ablak (pl. $t\in[1.0,
   1.05]$) rövidebb, mint a természetes lépésméret, a keresés sosem mintáz semmit az ablakon
   belül, pedig lehetne ott valódi metszéspont.

4. **Felezéses pótlépés** (`ti=(maxT-t)*0.5`, ismételve, `maxSteps`-szel korlátozva).
   A felhasználó **két ponton is korrigálta a kommentben adott indoklást**:
   - *Első korrekció:* "a rövidebb lépés garantáltan biztonságos, mert a kúp garanciájának
     csak egy részét használjuk" — **hamis**. A kúp-generátor egy nullára állított,
     "üres" (textúra-padding) terület szélén **maximálisra nyitja** a kúpot, függetlenül
     attól, hogy ez a mi háromszögünk érvényes UV-tartományán kívülre vinne. A generátornak
     fogalma sincs háromszög-határokról — ennek felismerése kizárólag a ray-marching
     algoritmus feladata.
   - *Második, pontosító korrekció:* a kúp tangens-értéke **csak** azt garantálja, hogy
     *a kúpon belül maradva* a sugár nem metszi a (globális) magasságmezőt — ez igaz
     állítás, de **más kérdésről szól**, mint hogy a pont a mi háromszögünk érvényes
     tartományán belül van-e. A két kérdés független; a kúp-garanciára hivatkozni a
     határ-kezelés indoklásaként félrevezető volt.
   → **Eredmény (ami miatt ezt a próbálkozást is elvetettük):** egy **mindenhol nulla
   magasságú** háromszögnél (ahol a helyes eredmény egy sík, torzítatlan háromszög lenne,
   pontosan illeszkedve a wireframe-re) a ConeStepMapping **torz, rossz alakú polygon**-t
   adott — mert a felezés *soha nem éri el pontosan* a `maxT` határt (mindig szigorúan előtte
   közelít), miközben a valódi metszéspont (sík felületnél) **pontosan egybeesik** a prizma
   alaplapjával, ami épp a `maxT` határ. A LinearSearch (ami nem használ kúp-lépést, csak
   fix lépésközt) ugyanezen a háromszögön helyesen a sík alakot adta (külön, nem ide tartozó
   "lyukacsos" hibával, ld. 3.2).

**Végleges, helyes megoldás:** mivel $M$ és $T$ ugyanazt a homogén nevezőt osztja (közös
$W$-sor az apex-konstrukcióban), **a textúra-térbeli kilépési pont $z$ (magasság)
komponense pontosan megegyezik** az egységprizma-térbeli kilépés $y$ komponensével. Ebből:

- `u2.z ≈ 0` ⟺ a sugár a prizma **alaplapján** keresztül lép ki → ez **feltétel nélkül**
  találat (bármilyen valós magasság $\ge0$, ami pontosan a ray saját magassága itt).
- `u2.z` érdemben $>0$ ⟺ oldal-/tetőlapon lép ki → nincs mit tesztelni odakint, jogos no-hit.

Ez a felismerés **kiváltotta az egész felezéses mechanizmust** egyetlen, olcsó elágazással —
nincs szükség iterációra.

### 2.7 "Kamera bent van a prizmában" teszt — hiányos feltétel

**Felfedezés módja:** egy konkrét primitívnél (360, $k=6.35$) a debug-szonda belépési
pontjának visszavetített (world-space) pozíciója **nyilvánvalóan távol** esett a tényleges
(parányi) háromszögtől — annak ellenére, hogy a kód "bent van"-ként kezelte a kamerát.

**Ok:** a kód csak `unitInt.near < 0.0`-t ellenőrizte. Egy projektív $T$ mellett egy, a
háromszögtől távoli kamera a `T`-vel leképezve olyan pontba kerülhet, hogy a cél (`At`) felé
mutató sugár iránya **elfelé** mutat a prizmától — ekkor **mindkét** metszésparaméter
(`near` és `far`) negatív lesz (az egész prizma a sugár *mögött* van), de az
`intersectUnitPrism` ettől még `found=true`-t ad (`near<far` formálisan teljesül). A hívó kód
ezt tévesen "bent van"-nak értelmezte.

Kézi számolással (a konkrét primitív $T$ mátrixával és a debug-kamera adataival)
megerősítve: `UnitEye≈(1.235, 2.605, 0.864)` (már ez is kívül esik az egységkockán),
`near≈-2.38`, `far≈-2.15` — mindkettő negatív.

**Javítás:** `near<0.0 && far>0.0` — mindhárom előfordulási helyen (`Frag_ConeStepMapping.frag`,
`Frag_LinearSearch.frag`, `Geom_RM_apex.geom` debug blokkja).

### 2.8 `gs_out_tex` vs. `M(p)` — a legstrukturálisabb hiba

**Tünet, ami a 2.7 javítás UTÁN is megmaradt:** "falak" (maximumhoz közeli, hamis
magasság-találatok) jelentek meg a prizma-határoknál, Föld-magasságtérképpel, **mindkét**
technikánál (ConeStepMapping és LinearSearch is).

**Ok:** a belépési pont (`u1`), amikor a kamera a prizmán kívül van, nem $M$-en keresztül
számolódott, hanem a `gs_out_tex` varying-ból — ami a hat kalibrációs pont (`texPos[0..5]`)
**GPU-rasztarizáció általi, lineáris (barycentrikus) interpolációja**. A régi, affin
korszakban ez pontosan megegyezett azzal, mintha $M$-et alkalmaznánk a fragment világtér-
pozíciójára — mert **affin leképezés felcserélhető a barycentrikus interpolációval**. Az
apex-korrekció mellett $M$ **projektív**, és ez a felcserélhetőség **nem áll fenn** —
`gs_out_tex` és a helyes `M(p)` eltér egymástól, annál jobban, minél nagyobb $k$ (minél
"projektívebb" a leképezés). A belépés (hibás, naiv) és a kilépés (helyes, $M$-alapú)
pont emiatt két **inkonzisztens parametrizáció** mentén indította a keresést.

Ez magyarázza, miért csak bizonyos (nagy $k$-jú) primitíveknél jelentkezett súlyosan, miért
egyformán mindkét technikánál (ez a hiba a megosztott belépésszámításban volt, nem a
keresési algoritmusban), és miért nem sikerült sosem debug-szondával reprodukálni — a
debug blokk mindvégig helyesen, $M$/$T$-n keresztül számolt, sosem a naiv `gs_out_tex`
útvonalat használta.

**Javítás:** mindkét fragment shaderben a kamerán-kívüli ág `gs_out_tex` helyett
`perspectiveDiv(M * vec4(p, 1.0))`-t használ (`p = gs_out_pos`, már eddig is elérhető volt).

---

## 3. Amit **nem** oldottunk meg ebben a körben

### 3.1 Háromszög-határ inkonzisztencia (vállalt, dokumentált korlát)

A `docs/ImplementationPlan_MergedNormals.md` ezt explicit, tudatosan vállalt korlátként
említi: szomszédos háromszögeknek **más-más $P_{apex}$-ük van**, ezért a keresési `uv` a
közös él közvetlen közelében elvben kismértékben eltérhet a két oldalon. A javasolt teljes
megoldás ("a keresés és a végső rekonstrukció szétválasztása", barycentrikus visszavetítés
a nyers csúcsadatokra, Hoetzlein Eq. 7 mintájára) **nincs implementálva** ebben a körben —
jövőbeli finomításként van dokumentálva.

### 3.2 LinearSearch "lyukacsossága"

A sík (nulla magasságú) teszt-háromszögnél a LinearSearch helyesen a sík alakot adta, de
**szórt lyukakkal** (hiányzó pixelek). A felhasználó ezt **külön, az apex-témától
független problémának** azonosította, feltételezése szerint numerikus pontatlanság —
**nincs diagnosztizálva, nincs javítva** ebben a körben.

### 3.3 `useApex` guard tényleges szükségessége, újra-tesztelve

A $k,w_\beta,w_\gamma\in(10^{-3},10^3)$ guard (2.1) széles — egy $k=6.35$-ös primitívnél
komoly, vizuálisan észlelhető problémák jelentkeztek, **de ezek a problémák végül a 2.7 és
2.8 hibákra vezethetők vissza**, nem feltétlenül az apex-illesztés instabilitására magára.
**Nem történt újratesztelés** a 2.7/2.8 javítások után annak eldöntésére, hogy ez a guard
önmagában elég szigorú-e, vagy létezik-e olyan háromszög, ahol *csak* a $k$ nagysága (a
2.7/2.8 hibák nélkül is) látható problémát okoz.

### 3.4 "Vékony/elfajult háromszög-alak" hipotézis

Korán felmerült (a pólus-közeli, "pite-szelet" háromszögek saját $a,b,c$ pozíciói majdnem
kollineárisak lehetnek, az $L$ mátrix invertálása emiatt instabillá válhat — függetlenül a
normálisok párhuzamosságától). **Ezt sosem sikerült konkrétan megfigyelt hibához kötni** —
a ténylegesen talált hibák (2.6, 2.7, 2.8) mind más okra vezethetők vissza. Nyitott,
nem megerősített, de nem is cáfolt hipotézis.

### 3.5 Dokumentáció-szinkron

- `docs/ImplementationPlan_MergedNormals.md`: a korai, már felülvizsgált "döntött tetősík"
  levezetést még mindig tartalmazhatja (a jelen kör ezt már a sík tetősíkos verzióval
  implementálta) — **nincs frissítve**.
- `docs/SSBO_Debug_Layout.md`: **elavult** — a 2.4 pontban leírt bővítés (23→28 slot-eltolás,
  új `[23..27]` apex-diagnosztika) nincs átvezetve. Emellett a dokumentum már a mostani kör
  *előtt* is pontatlan volt (17 header slotot ír le, miközben a kód ekkor már 23-at
  használt) — ezt is érdemes lenne rendezni egy külön körben.
- Dolgozat-szöveg (Hoetzlein-hivatkozás a `forrasok.bib`-be, `Befoglalo-geometria.tex` és
  `Tertranszformaciok.tex` kiegészítése) — a terv szerint az implementáció igazolása után
  esedékes; **nincs elkezdve**.

### 3.6 `src/` szinkron állapota

A napló írásakor **nem ellenőrzött**, hogy a `temp/Phase35` legutóbbi két javítása
(kamera-bent-van teszt, `gs_out_tex`→`M(p)`) át lett-e már másolva a `src/`-be — ez a
felhasználó saját, manuális lépése a megszokott munkafolyamat szerint.

---

## 4. Áttekintett fájlok / források

**Shaderek (olvasva és/vagy módosítva):**
`src/Shaders/RayMarching/{Geom_RM_apex.geom, Geom_RM_abcd.geom, RayMarch_common.glsl,
Frag_ConeStepMapping.frag, Frag_LinearSearch.frag}`,
`src/Shaders/Debug/{Vert_DebugCones.vert, Vert_DebugEnterExit.vert, Vert_DebugHit.vert,
Vert_DebugRay.vert, Vert_DebugSteps.vert}` — és ezek `temp/Phase34` / `temp/Phase35` másolatai.

**C++ (olvasva és/vagy módosítva):**
`src/Utils/ModelLoader.h` (a `MergeNormals` és `LoadFromOBJ` implementációja — ellenőrizve,
hogy a pozíció-alapú normál-egyesítés **nem** nyúl az UV-khoz; ez alapján **kizárva** mint a
varrat/pólus-torzítás oka),
`src/Sources/MyApp/{Debug.cpp, GUI.cpp}`,
`src/Sources/RendererVisitor/OpenGLRendererVisitor.cpp` (a `debugVisual[5]/[6]` CPU-oldali
feltöltésének megértéséhez),
`src/Headers/Debug/RayMarchDebugState.h`.

**Dokumentáció:**
`docs/ImplementationPlan_MergedNormals.md` (az irányadó terv),
`docs/SSBO_Debug_Layout.md` (ld. 3.5, elavult),
`thesis/chapters/Algoritmusok/Tertranszformaciok.tex` (a meglévő affin levezetés szövege,
csak olvasva, nem módosítva).

**Külső hivatkozás:**
Hoetzlein (2025), *Projective Displacement Mapping for Ray Traced Editable Surfaces*
(arXiv:2502.02011) — kapcsolódó munka/ihlető forrás az eredeti tervben, a végső megoldás
ettől független, saját levezetés.

**Diagnosztikai módszer:**
a beépített "Export debug log" funkció (`MyApp::ExportDebugLog`) ismételt, kézi
kiértékelése (konkrét primitívek: 478, 241, 259, 360) — a `temp/Phase34` 2.3–2.4 pontban
leírt eszközeivel (primitív-izoláció, bővített numerikus dump) kombinálva.

---

## 5. Lehetséges további hibaforrások (figyelmeztetés jövőbeli munkához)

- **A `DBG_STEP_BASE` (28) szinkronban tartása**: ha a `debugNumerical` fejléc mérete még
  egyszer változik, **mind a négy** helyet frissíteni kell egyszerre:
  `RayMarch_common.glsl` (`DBG_STEP_BASE` makró), `Vert_DebugCones.vert`, `Vert_DebugHit.vert`,
  valamint a CPU-oldali `Debug.cpp`/`GUI.cpp` buffer-méret és olvasási offset. Ezek **nem
  egy helyen** vannak definiálva — könnyű csak néhányat frissíteni.
- **Önkényes epszilon-konstansok**, amik empirikusan lettek választva, nem szigorúan
  levezetve: `1e-4` (`exitsThroughBase` küszöb), `1e-6` (lépésméret-zaj küszöb és a ciklus
  leállási feltétele), `1e-5` (`determinant(S)` degeneráció), `(1e-3,1e3)` (`k,wB,wC` guard
  tartomány). Más mesh-léptéknél / `normalMult` értéknél ezek újrahangolásra szorulhatnak.
- **`maxT` számítása egyetlen tengelyből** (`x`, majd `y`, majd `z`, az első
  `abs(v.*)>1e-4`-et teljesítő): feltételezi, hogy a belépés/kilépés pontosan kollineáris a
  textúra-térben (ez igaz projektív leképezésnél), de a *melyik tengelyt válasszuk*
  heurisztika (nem a legnagyobb komponens, hanem az első küszöb feletti) elvben kevésbé
  stabil, mint egy max-komponens-alapú választás lenne — ez nem lett vizsgálva/javítva.
  Eddig minden kézzel ellenőrzött esetben a három tengely konzisztens eredményt adott.
- **`gs_out_tex` maradék felhasználása**: a 2.8 javítás után érdemes átnézni, hogy a
  `gs_out_tex` varying-ot használja-e még bármi más (pl. árnyékolás, normál-számítás) a
  fragment shaderekben — ha nem, holt kód, érdemes lenne eltávolítani/dokumentálni, hogy
  miért maradt meg (jelenleg nem lett szisztematikusan átvizsgálva).
- **Debug-szonda ≠ tényleges pixel-sugár**: a teljes hibakeresés során visszatérő
  buktató volt, hogy a debug-kamera saját (egyetlen) szondasugara **nem feltétlenül**
  egyezik meg azzal, amit egy adott, vizuálisan hibás pixel saját sugara csinál — "nincs
  hit" a szondán nem bizonyítja, hogy a közelben nincs vizuális hiba, és fordítva. A 2.8
  javítás után ez a kockázat **csökkent** (mindkét útvonal most már $M$/$T$-n keresztül,
  konzisztensen számol), de nem szűnt meg teljesen (a debug-szonda iránya ált. más, mint
  bármelyik konkrét fragment saját sugara).
- **$\mu$ / normalMult-arány érzékenysége**: empirikusan a pólus-közeli primitíveknél
  $\mu\approx1.9$–$2.0$ ($k\approx2$) volt jellemző, míg egy más elhelyezkedésű primitívnél
  $\mu\approx1.19$ ($k\approx6.35$) — minél közelebb $\mu$ az 1-hez, annál érzékenyebb/
  "projektívebb" a leképezés. Ez fizikailag a `normalMult`/sugár arányhoz kötődik, és
  érdemes lehet a dolgozatban külön tárgyalni, mint a módszer egyik ismert érzékenységi
  tényezőjét (nem hiba, hanem a módszer egyik jellemzője).
