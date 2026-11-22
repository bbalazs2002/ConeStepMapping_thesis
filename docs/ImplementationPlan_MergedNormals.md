# Egyesített normálisok kezelése — apex-alapú projektív transzformáció

## Kontextus

A dolgozat jelenleg megoldatlan problémaként írja le az egyesített (per-vertex,
`MergeNormals`-szel átlagolt) normálisok esetét: a befoglaló prizma felső lapja
csak egyetlen referenciacsúcsnál (a 4. ponton, amit az affin M/T mátrixok
számításához használunk) ad pontos textúra-leképezést; szélsőséges esetben
(összetartó normálisok) a prizma felső lapja önmagába fordulhat.

Három megoldási irányt vizsgáltunk meg:

1. **Saját projektív-transzformációs levezetés** (apex-pont + perspektív
   osztás mindenhol) — működik, de eredetileg úgy tűnt, csak a
   "szimmetrikus" (minden csúcs azonos arányban nyúlik az apex-től) esetben
   pontos minden referenciapontban.
2. **Hoetzlein (2025), *Projective Displacement Mapping for Ray Traced
   Editable Surfaces* (arXiv:2502.02011)** — egy publikált, de **részleges**
   megoldás ("parallel offset prism", Eq. 4–6): csökkenti, de nem szünteti
   meg a torzítást; a felhasználó ezt elvetette, mert nem ad látványos
   javulást a legtöbb esetben.
3. **A felhasználó javaslata**: a normálisok metszéspontja legyen a 4.
   referenciapont a transzformációhoz, a felső lap csúcsait pedig az
   eredeti csúcsokból a metszéspont felől húzott vektorokkal írjuk fel.

A 3. irányt tovább vezetve **egy, az 1. és 2. opciónál jóval pontosabb
megoldáshoz jutottam**, amely a háromszögek belsejében (nem csak egy
referenciaponton) megszünteti a torzítást — ezt dolgozza fel ez a terv.
(A háromszög-határokra korlátozódó, finomabb él-inkonzisztencia elvben
teljesen kiküszöbölhető egy további lépéssel — ld. "Fejlesztési
lehetőség" — de ez **nem** része a jelenlegi megvalósítási körnek.)

## A matematikai levezetés

### Az alapötlet: apex-középpontú projektív koordináták

Legyen $P_{apex}$ az a pont, amelyben a három csúcs ($a, b, c$) normálisegyenese
(közelítőleg) metszi egymást. Vezessük be az apex-középpontú bázist:
$A = a - P_{apex}$, $B = b - P_{apex}$, $C = c - P_{apex}$, és
$L = [A\,|\,B\,|\,C]$. Egy tetszőleges $X$ pont "apex-koordinátái"
$(\alpha,\beta,\gamma)^T = L^{-1}(X - P_{apex})$ — ez pusztán egy eltolás +
lineáris leképezés (nem projektív), tehát $(\alpha,\beta,\gamma,1)^T = N\cdot(X,1)^T$
egy affin $N$ mátrixszal felírható.

Ebben a bázisban: $a=(1,0,0)$, $b=(0,1,0)$, $c=(0,0,1)$, és mivel a felső
lap csúcsai ugyanazon az egyenesen vannak, mint a hozzájuk tartozó
alapcsúcs (az apexen át): $d=(\mu,0,0)$, $e=(0,\nu,0)$, $f=(0,0,\rho)$,
ahol $\mu,\nu,\rho$ annak az aránynak felel meg, hogy az adott felső
csúcs milyen távol van az apextől az alapcsúcshoz képest.

### Az eredeti (szimmetrikus) levezetés és korlátja

Ha $\mu=\nu=\rho$ (minden csúcs **azonos arányban** nyúlik ki az apex felől),
zárt alakban felírható egy $4\times4$-es **projektív** mátrix (nem affin —
valódi perspektív osztást igényel), amely $a,b,c,d,e,f$ **mind a hat
pontban pontosan** megfelel a jelenlegi egységprizma-tér céltartomány
pontjainak ($a\to(0,0,0)$, ..., $d\to(0,1,0)$, $e\to(1,1,0)$, $f\to(0,1,1)$
— ugyanaz, mint ma). Ez tökéletes lenne, **de** $\mu=\nu=\rho$ csak akkor
teljesül automatikusan, ha a felső csúcsokat **az apextől egyenlő arányban**
generáljuk — ami viszont **nem egyezik meg** a jelenlegi, repedésmentes
extrudálással ($d_i = v_i + h_{max}\cdot n_i$, csúcsonként **rögzített
hosszúságú** eltolás a saját normális mentén).

### A felfedezés: nem kell $\mu=\nu=\rho$-t megkövetelni

Ha a felső csúcsok célpontjainak magasságát **nem rögzítjük** 1-re
mindhárom csúcsnál, hanem szabadon hagyjuk ($y_d, y_e, y_f$ külön-külön
változók), a levezetés azt mutatja, hogy a mátrix konzisztenciájához
**pontosan egyetlen** megkötésre van szükség:

$$\frac{y_d \cdot \mu}{1-\mu} = \frac{y_e \cdot \nu}{1-\nu} = \frac{y_f \cdot \rho}{1-\rho} \;(=: C)$$

Ez **3 ismeretlenre 1 egyenlet** — tehát **mindig megoldható**,
tetszőleges (akár mind különböző) $\mu,\nu,\rho$ esetén! Rögzítve pl.
$y_d=1$-et (ugyanaz a konvenció, mint ma), a másik kettő abból
következik: $y_e = \frac{C(1-\nu)}{\nu}$, $y_f = \frac{C(1-\rho)}{\rho}$,
ahol $C = \frac{\mu}{1-\mu}$.

**Ez azt jelenti: a crack-mentes (csúcsonként rögzített $h_{max}$ hosszúságú)
extrudálás MELLETT is létezik egzakt projektív mátrix, amely mind a hat
referenciapontot pontosan leképezi** — nem kell választani a pontosság és a
repedésmentesség között!

### Ára: az egységprizma felső lapja megdől

A fenti megoldás egyetlen következménye: a céltartomány (egységprizma tér)
felső lapja **már nem feltétlenül vízszintes** ($y=1$ sík), hanem egy
$(0,y_d,0)$, $(1,y_e,0)$, $(0,y_f,1)$ pontokon átmenő **döntött sík**
(ami primitívenként más, hiszen $y_d,y_e,y_f$ primitívenként számolódik).
Ez azt jelenti, hogy az `intersectUnitPrism` függvény (jelenleg hardkódolt
$y \leq 1$ feltétellel) **egyetlen féltér-tesztjét** általánosítani kell:
a fix $y=1$ sík helyett egy primitívenként átadott, tetszőleges síkegyenletet
kell használnia. A másik négy féltér-teszt (x≥0, y≥0, z≥0, x+z≤1)
**változatlan marad** — ez egy kis, jól körülhatárolt, zárt alakú
módosítás, **nem** bilineáris folt / world-space marching csere (ahogy a
Hoetzlein-cikk teljes megoldása igényelné).

### Degenerált eset: majdnem párhuzamos normálisok

Ha a három normális majdnem párhuzamos (sík/enyhén görbülő felületrész),
az apex nagyon távol kerül, és $\mu,\nu,\rho \to 1$ — ekkor a fenti
képletekben $C = \mu/(1-\mu) \to \infty$, numerikusan instabil. Ez
pontosan az az eset, ahol a **jelenlegi affin konstrukció már amúgy is
pontos** (mert ott tényleg nincs szükség projekcióra). Megoldás: ha
$|1-\mu|$ (vagy ezzel ekvivalensen az apex távolsága) egy küszöb alatt van,
**essünk vissza a jelenlegi affin útvonalra változtatás nélkül** — ez egy
természetes, jól indokolt elágazási feltétel.

## Kritikus felülvizsgálat: milyen torzítást okoz ez a végső felületen?

**A kérdés jogos és egy valódi, komoly problémára mutatott rá.** Két
különálló hatást kell szétválasztani.

### 1) A döntött felső sík önmagában nem okoz torzítást a *kereséshez* képest

Levezetéssel megmutatható, hogy egy apex-koordinátájú $X$ pont
$X = (1-s)\cdot P_{apex} + s\cdot P_{bary}(u,v)$ alakban írható fel, ahol
$P_{bary}(u,v) = \lambda_a a + \lambda_b b + \lambda_c c$ a **sima, lapos**
barycentrikus pont az alaplapon, $s=\alpha+\beta+\gamma$ pedig egy
"sugár menti" paraméter. Ez pontosan a kúp/csonka gúla standard
centrálvetületi paraméterezése — geometriailag értelmes, nem önkényes.

### 2) DE: a felszín-rekonstrukció közvetlen $M^{-1}$-es számítása repedést okozna

A probléma: $X - P_{bary}(u,v) = (s-1)\cdot[\lambda_a A + \lambda_b B + \lambda_c C]$,
ahol $A=a-P_{apex}$ stb. Mivel $A$ párhuzamos $n_a$-val, de **nem egyenlő**
vele (egy $\kappa_a$ skálázási tényezőtől eltekintve, ami
**primitívenként** más), az eltolás iránya
$\lambda_a\kappa_a n_a + \lambda_b\kappa_b n_b + \lambda_c\kappa_c n_c$ —
**ez csak akkor egyezik meg** a kívánt, Hoetzlein-féle (Eq. 2) "lapos"
interpolált normálissal $N'(u,v)=\lambda_a n_a+\lambda_b n_b+\lambda_c n_c$,
ha $\kappa_a=\kappa_b=\kappa_c$ (vagyis a szimmetrikus eset, amit korábban
már kizártunk).

**Ennek a gyakorlati következménye éldarabok mentén:** egy közös élen
($a,b$) ülő két szomszédos háromszögnek **különböző apexe van** (mert a
harmadik csúcsuk eltér), tehát $P_{apex} \ne P_{apex}'$. Ha a tényleges
felszínpontot *közvetlenül* az apex-alapú mátrix inverzével
($M^{-1}(u,v,h)$) számolnánk, a két háromszög **a közös él belsejében**
(nem csak a csúcsokon!) **különböző 3D pontot** adna ugyanarra a
$(u,v,h)$-ra — ez egy valódi, látható **varrás/repedés** lenne a
megjelenített felületen, rosszabb jellegű hiba, mint az eredeti
"torzulás egy referenciaponton kívül" probléma.

**Fontos: a `MergeNormals` jelenlegi motivációja is pont ez** — a sík
("naiv", vertex-only) kiterjesztés azért repedésmentes, mert **kizárólag
az adott él két végpontjának adatától függ**, nem a harmadik csúcstól
(apextől). Bármilyen konstrukció, ami egy **primitívenkénti** mennyiséget
(apex, vagy akár a Hoetzlein-cikk $N_g$ lapnormálisa is!) kever az
*interior* alakba, ezt a tulajdonságot elveszti.

*(Ellenőrizve: a Hoetzlein-cikk saját "parallel offset prism" korrekciós
tényezője, $n_f(u,v)$, szintén tartalmazza a primitívenkénti $N_g$-t —
tehát **ugyanez a repedés-kockázat náluk is fennállna**, HA a végső
felszínpontot a prizma-geometriából közvetlenül olvasnák ki.)*

### A feloldás: a keresés és a végső rekonstrukció szétválasztása (fejlesztési lehetőség, nem a jelenlegi kör része)

A Hoetzlein-cikk **Algorithm 1 "final hit" blokkja** pontosan ezt teszi —
ez a kulcs, amit *egy későbbi finomítás részeként* át lehetne venni,
**akkor is, ha a saját mátrix-alapú keresési magunkat megtartjuk**. A
jelenlegi (1–3. lépésből álló) implementációs kör ezt **nem tartalmazza**
— ott ez az él-inkonzisztencia vállalt, dokumentált korlátként megmarad:

- A **keresés/marching** (hol lép be-ki a sugár a prizmába, hol egyezik
  meg a sugár magassága a textúra magasságával) az apex-alapú projektív
  $M,T$ mátrixokkal történik — ez adja a jó, (majdnem) torzításmentes
  becslést a találat $(u,v,h)$ texture-koordinátájára.
- **A találat után** a ténylegesen megjelenített/árnyalt világtér-pontot
  **NEM** $M^{-1}(u,v,h)$-val számoljuk, hanem a megtalált $(u,v)$-ból
  **2D-s barycentrikus inverzióval** (`triangleBarycentric`-stílusú,
  ugyanaz a technika, mint Hoetzlein C. függeléke, csak a sík
  UV-háromszögre, nem 3D-ben) visszaállítjuk $\lambda=(\lambda_a,\lambda_b,\lambda_c)$-t,
  majd a **lapos, csak a nyers csúcsadatoktól függő** képlettel
  (Hoetzlein Eq. 7):
  $$p_{hit} = \lambda_a a+\lambda_b b+\lambda_c c \;+\; h\cdot h_{max}\cdot(\lambda_a n_a+\lambda_b n_b+\lambda_c n_c)$$
  állítjuk elő a végleges pontot (és ugyanígy a normálist árnyaláshoz).
  Ez **garantáltan élkonzisztens** (csak a 2-3 érintett csúcs saját
  adatától függ, pont úgy, ahogy a `MergeNormals` is ezt garantálja ma).

**Eredmény:** a keresés pontosabb lesz (az apex-korrekció miatt), a
**megjelenített geometria pedig változatlanul repedésmentes és a
magasságtérkép által meghatározott részletek torzítatlanul megmaradnak**
— mert a ténylegesen kirajzolt pont kiszámítása független attól, hogy a
keresés milyen segédgeometriát/mátrixot használt.

## Tisztázó megjegyzések (visszatérő kérdések alapján)

1. **Nem feltételezzük, hogy a normálisok pontosan metszik egymást.** Az
   apex egy legkisebb-négyzetes "legközelebbi pont 3 egyeneshez" illesztés
   eredménye — működik kitérő (nem pontosan metsző) egyenesekre is. Ha a
   normálisok majdnem párhuzamosak, az illesztés instabillá válik, ekkor a
   jelenlegi affin konstrukcióra esünk vissza.
2. **Az `intersectUnitPrism`-et módosítani kell** — az $y\le1$ felső-lap
   teszt primitívenkénti síkra cserélődik, a másik 4 teszt változatlan.
3. **A világtér-beli prizma-geometria (`d,e,f`, azaz a ténylegesen kirajzolt
   csúcsok) NEM változik** — továbbra is `d_i = v_i + h_max·n_i`, kizárólag
   a saját (megosztott, repedésmentes) vertex-normálistól függően. **Szándékosan
   NEM vezetünk be olyan szorzót (pl. Hoetzlein $1/(N_i\cdot N_g)$-féle
   korrekcióját), ami a lapnormálistól ($N_g$, primitívenkénti mennyiség)
   függne** — az ilyen szorzó a felső lapot világtérben párhuzamossá tenné
   az alaplappal, de mivel $N_g$ háromszögenként eltér, egy megosztott
   csúcs felső pontja két szomszédos háromszögnél **eltérő** lenne, ami
   újra rést nyitna (pont azt törné el, amit a `MergeNormals` megold). A
   "ferdeség" ezért kizárólag a keresési mátrix ($M,T$) belső
   felépítésében jelenik meg, nem a kirajzolt geometriában és nem a végső,
   megjelenített felszínen (ami a fenti, lapos rekonstrukcióval készül).

## A végső rekonstrukció pontos menete (lépésről lépésre)

**Fontos pontosítás a valódi kód alapján:** a jelenlegi
`Frag_ConeStepMapping.frag`/`Frag_LinearSearch.frag` **nem állít elő
világtér-beli találati pontot egyáltalán**. A keresés
(`findIntersection_coneStepMapping`/`findIntersection_linearSearch`,
`RayMarch_common.glsl`) kizárólag egy 2D `uv`-t ad vissza
(`IntersectReturn.uv`), amit közvetlenül a `conemap_getHeight(uv)` /
`getNormalFromHeightmap(uv)` lekérdezésekhez használnak; az árnyaláshoz
(`viewDir` stb.) a **rasterizált prizma-felület saját, GL által
interpolált pozícióját** (`gs_out_pos`) használja a kód, nem egy
visszafejtett, pontos találati pontot. Tehát a "repedés-kockázat" pontos
helye **nem egy (ma nem is létező) világtér-rekonstrukciós lépés**, hanem
maga a `uv` érték: kérdés, hogy ez az apex-alapú kereséssel számolt `uv`
egyezik-e két szomszédos háromszögnél ugyanarra a fizikai pontra.

**A lépések, konkrétan, a meglévő kód kiegészítéseként:**

1. **`IntersectReturn` kiegészítése egy magasság-mezővel** (`RayMarch_common.glsl`):
   jelenleg csak `vec2 uv`-t ad vissza a találatról, a magasságot
   (`ui.z`/`u.z` a függvényeken belül) nem. Hozzá kell adni egy `float h`
   mezőt, és mindkét `findIntersection_*` függvény visszatérési értékét
   kiegészíteni vele (a hurkon belül már rendelkezésre áll, csak eddig
   nem volt visszaadva).
2. **A geometry shader továbbadja a nyers csúcsadatokat is** a fragment
   shadernek: `a,b,c` világtér-pozíció, `n_a,n_b,n_c` egyesített normális,
   `uv_a,uv_b,uv_c` textúra-koordináta (ezek már most is elérhetők a
   geometry shaderben, csak eddig nem kerültek ki — új `flat`
   kimeneti változók).
3. **A keresési hurok változatlanul fut** (`findIntersection_*` hívás,
   ugyanúgy, mint ma) — csak az `M`,`T` most már az apex-korrigált
   (projektív) mátrixok, és a visszatérési érték most `data.uv` MELLETT
   `data.h`-t is tartalmaz.
4. **Egyetlen $M^{-1}$ alkalmazás, a hurok UTÁN** (nem lépésenként!):
   ```glsl
   vec4 hitHom = inverse(M) * vec4(data.uv, data.h, 1.0);
   vec3 hitPos = hitHom.xyz / hitHom.w;   // világtér-pont, apex-korrigált
   ```
5. **Visszavetítés lapos barycentrikus súlyokra** — ugyanaz a technika,
   mint Hoetzlein C. függelékének `triangleBarycentric`-je (Ericson,
   *Real-Time Collision Detection*), csak a saját `a,b,c`-nkre alkalmazva:
   ```glsl
   vec3 bary = triangleBarycentric(hitPos, a, b, c); // (λ_a, λ_b, λ_c)
   ```
   Ez egy 3D pont legközelebbi síkbeli vetületének barycentrikus súlyait
   adja — zárt alak, nincs keresés/iteráció.
6. **A tényleges textúra-lookuphoz használt UV felülírása a lapos,
   kizárólag a 2-3 csúcs saját UV-jétől függő értékkel:**
   ```glsl
   vec2 uv_final = bary.x*uv_a + bary.y*uv_b + bary.z*uv_c;
   ```
   Ez **nem** függ az apextől vagy bármilyen primitívenkénti mennyiségtől
   — két szomszédos háromszög, ugyanarra a fizikai pontra, **garantáltan
   ugyanazt** az `uv_final`-t adja (hiszen csak a megosztott csúcsok saját
   UV-jétől függ).
7. **Innentől minden változatlan**: `conemap_getHeight(uv_final)`,
   `getNormalFromHeightmap(uv_final)` — csak `uv` helyett `uv_final`-lal.

**Mi ennek a valódi tétje?** A keresés (4-6. lépés nélkül is) már önmagában
hatalmas javulást hoz a *mai* egypontos-pontos affin mátrixhoz képest — a
4-6. lépés egy **további, opcionális finomítás**, ami kifejezetten a
**háromszög-határok közvetlen közelében húzódó, keskeny sávra** vonatkozó
maradék-inkonzisztenciát zárja ki teljesen (lásd a Hoetzlein-cikk "4.2
Watertightness" fejezete — ez a jelenség ÁLTALÁNOS minden
prizma-alapú, primitívenkénti sugárkövetési technikánál, nem az apex-javítás
sajátja). **Javaslat:** a 4-6. lépést érdemes a 2. fázis részeként
megvalósítani (nem sokkal nagyobb munka, és a dolgozatban "teljesen
megoldott" állítható, nem csak "a legtöbb helyen megoldott, a
háromszög-határok kivételével").

## Implementáció — jelenlegi kör (1–3. lépés)

**Ez a kör egymagában valósítja meg a fő javítást** (a torzítás
megszüntetése a referenciaponton kívül, a "megcsavarodás" hibamód
kiküszöbölése). A 4–5. (végső rekonstrukció szétválasztása) **nem
része ennek a körnek** — ld. lejjebb, "Fejlesztési lehetőség".

1. **Geometry shaderben (`Geom_RM_abcd.geom`), primitívenként:**
   a. A három (pozíció, egyesített normális) párból legközelebbi-pont
      illesztéssel ("closest point to 3 lines", zárt alakú 3×3-as lineáris
      rendszer) kiszámítani $P_{apex}$-et.
   b. Ha a normálisok majdnem párhuzamosak (numerikus küszöb) → jelenlegi
      affin M/T számítás változatlanul.
   c. Egyébként: kiszámítani $\mu,\nu,\rho$-t a **jelenlegi, crack-mentes**
      $d_i = v_i + h_{max}\cdot n_i$ pontokból (ez **nem változik**), majd
      $y_d=1, y_e, y_f$-et a konzisztencia-képletből, majd felépíteni az
      új projektív M/T mátrixot (zárt alak, a fenti $R \cdot N$ szerkezet
      szerint).
   d. Kiszámolni és kimenetként továbbadni (geometry shader → fragment
      shader, pl. egy `flat`/interpolálatlan kimeneti változóval) a döntött
      felső sík egyenletét ($y_e, y_f$, vagy ezekből levezetett sík-normál +
      eltolás).
2. **Minden M/T-vel végzett ponttranszformáció helyén** (a felderítés szerint:
   `Geom_RM_abcd.geom`, `Frag_ConeStepMapping.frag`, `Frag_LinearSearch.frag`,
   és az összes `src/Shaders/Debug/Vert_Debug*.vert`, ahol `invM`-et
   használnak) **perspektív osztás hozzáadása** (`.xyz / .w`). Ez az affin
   fallback esetén ártalmatlan no-op (ott $w=1$ mindig).
3. **`intersectUnitPrism` (`RayMarch_common.glsl`) general­izálása:** az
   $y \leq 1$ féltér-teszt helyett egy paraméterül kapott, primitívenkénti
   sík használata (a másik négy teszt változatlan).

**Zéró CPU-oldali és vertex-attribútum változás** — az apex-keresés és a
mátrix-konstrukció teljes egészében a geometry shaderben történik, a már
meglévő `vs_out_merged[]` és `gl_in[].gl_Position` adatokból.

**Tudott, vállalt korlát ennél a körnél:** a "Kritikus felülvizsgálat"
szakaszban leírt, háromszög-határok közvetlen közelére korlátozódó
él-inkonzisztencia (a keresési `uv` primitívenként eltérő apex miatt) **itt
nem kerül kijavításra** — ez a 4–5. lépés (alább) feladata lenne. A
dolgozatban ezt explicit, őszintén dokumentálni kell korlátként, nem
eltitkolni.

## Fejlesztési lehetőség (nem a jelenlegi kör része): a végső rekonstrukció szétválasztása

A "Kritikus felülvizsgálat: milyen torzítást okoz ez a végső felületen?" és
"A végső rekonstrukció pontos menete" szakaszokban leírt 4–7. lépés (új
`IntersectReturn.h` mező, nyers csúcsadatok továbbadása, egyszeri
$M^{-1}$ a hurok után, lapos barycentrikus visszavetítés, `uv_final`)
**jövőbeli finomításként marad nyitva**, nem a mostani implementációs kör
része. Ha/amikor megvalósul, ez zárja ki teljesen (nemcsak a legtöbb
helyen) a maradék él-inkonzisztenciát — ld. a részletes levezetést és
lépéssort feljebb.

## Kapcsolódó, már eldöntött rész: OBJ → RayMarchedModel import

Fontos felfedezés (változatlanul érvényes): a `CreateModelFromOBJ`
(`src/Sources/MyApp/CreateModels.cpp:45-99`) jelenleg kizárólag sima,
**rasterizált** `Model`-t épít `Vertex`-ekből; a `MergeNormals` csak a
tesztekben fut. Az élő alkalmazásban jelenleg **nem lehet** tetszőleges
OBJ-modellt ray-marched technikával, egyesített normálisokkal betölteni —
csak a hardkódolt 4-csúcsú sík létezik `RayMarchedModel`-ként, aminél a
probléma fel sem merülhet.

**Ez a jelen terv előfeltétele** (a felhasználó megerősítette: "az első
lépés mindenképp az, hogy tetszőleges modellre lehessen alkalmazni a
jelenlegi ray-marching technikákat"), és **egy fázissal megelőzi** a fenti
matematikai javítást:

- **1. fázis:** `MyApp::CreateRayMarchedModelFromOBJ(path)` — új függvény,
  a `CreateDefaultRayMarchedModel` (`CreateModels.cpp:13-43`, a
  `RayMarchedModel` felépítésének mintája) és a `CreateModelFromOBJ`
  (`:45-99`, az OBJ-betöltés és anyag-feldolgozás mintája) ötvözete:
  `ModelLoader::LoadFromOBJ<VertexMergedNorm>(path, "./", ModelLoader::MergeNormals)`,
  majd `RayMarchedModel` felépítése (technique, selected program, light dir,
  mesh-enkénti anyag-feldolgozás, a jelenleg kiválasztott heightmap). GUI:
  új "Add RayMarched .obj" gomb a meglévő `m_objPathBuf`-ot és a már létező
  `CreateRMObjectCommand`-ot felhasználva (ugyanaz a minta, mint az "Add
  RayMarched" gombnál).
  **Scope (a felhasználóval egyeztetve): egy anyagot, egy mesh-t tartalmazó
  modellek (pl. kocka, gömb) — a `MergeNormals` jelenlegi anyagonkénti
  csoportosítása (`ModelLoader.h:163-205`) ennél a körnél nem korlátozó
  tényező.**
  *Elvárt köztes eredmény (a 2. fázis előtt): a megjelenítés összetett
  geometrián láthatóan eltorzulhat/degenerálódhat — ez a tudott, várt
  alapállapot, ami igazolja, hogy az importálás helyesen reprodukálja a
  dokumentált problémát.*
- **2. fázis:** a fenti apex-alapú projektív javítás, ugyanezeken a most
  már betölthető, valódi teszt-modelleken (kocka, gömb) közvetlenül
  demonstrálható és ellenőrizhető — mind a torzítás, mind a
  "megcsavarodás" hibamód megszűnésére nézve.

## Dolgozat-szöveg (javaslat, `temp/`-be)

- Hivatkozás felvétele a `forrasok.bib`-be (Hoetzlein 2025, arXiv:2502.02011)
  — mint kapcsolódó munka / inspiráció, még ha a végső megoldás más is lett.
- `Befoglalo-geometria.tex` "Megjegyzés: normálisok egyesítése" alfejezet
  átírása: a probléma leírása után bemutatni az apex-alapú projektív
  transzformációt — a levezetéssel, a döntött felső sík koncepciójával, és
  a majdnem-párhuzamos esetre vonatkozó fallback-kel. Explicit, őszintén
  dokumentálni kell a megmaradó, háromszög-határokra korlátozódó
  él-inkonzisztenciát mint vállalt korlátot (nem állítható, hogy a probléma
  teljesen megszűnt) — valamint megemlíteni a kijavítás módját
  (keresés/végső-rekonstrukció szétválasztása) mint jövőbeli, nem
  megvalósított finomítást.
- `Tertranszformaciok.tex` / `Unit-prism-intersection.tex` kiegészítése az
  általánosított (per-primitív döntött felső síkú) egységprizma-metszéssel.

## Ellenőrzés

- **1. fázis után, 2. fázis előtt:** kocka vagy gömb OBJ betöltése az új
  gombbal, `normalMult` felhúzása — elvárt: látható torzulás/degenerálódás
  (igazolja a helyes bekötést).
- **2. fázis után:** ugyanazon a modellen — elvárt: a torzulás a
  háromszögek *belsejében* megszűnik, a felső lap sosem "csavarodik meg".
  Debug rendszerrel (lépés-vizualizáció, be-/kilépési pontok) összevetve
  ellenőrizhető, hogy a belépési/kilépési pontok és a lépések a döntött
  felső síkkal konzisztensek.
- **Élkonzisztencia (tudott, vállalt korlát ennél a körnél, nem hiba):**
  egy kocka két szomszédos (más-más apexű) háromszögének közös élét
  közelről, nagy `normalMult` mellett megnézve **várhatóan** látható lesz
  egy keskeny sávban némi maradék eltérés — ez a dokumentált, elfogadott
  korlát (ld. "Fejlesztési lehetőség" szakasz), nem a jelenlegi kör
  hibája. A cél itt annak megfigyelése/dokumentálása, hogy ez a maradék
  jelenség valóban **csak** a háromszög-határ közvetlen közelére
  korlátozódik, a háromszög nagy részén (a fő, eredeti bug helyén) a
  javulás egyértelmű és nagymértékű.
- Regresszió: egyetlen (nem egyesített) normálisú primitíveken a numerikus
  fallback-ágnak kell lefutnia (majdnem-párhuzamos normálisok), a
  viselkedés nem változhat a jelenlegihez képest.
- Generált kód a workflow szerint előbb `temp/PhaseXX/`-be kerül
  felülvizsgálatra, nem közvetlenül a `src/`-be.
