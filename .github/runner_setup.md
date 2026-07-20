# Self-Hosted Runner Setup (GL Tests)

## Követelmények

- Windows 10/11, x64
- MSVC 2022 (Visual Studio Build Tools elegendő)
- CMake 3.20+
- vcpkg (telepítve, `VCPKG_ROOT` environment variable beállítva)
- OpenGL 4.5 kompatibilis GPU + driver
- Minden vcpkg package telepítve (ld. lent)

## vcpkg csomagok

```powershell
vcpkg install glew:x64-windows
vcpkg install glm:x64-windows
vcpkg install gtest:x64-windows
vcpkg install tinyobjloader:x64-windows
vcpkg install "imgui[docking-experimental]:x64-windows"
vcpkg install sdl3:x64-windows
vcpkg install sdl3-image:x64-windows
```

> **Fontos:** Az `sdl3` és `sdl3-image` package nevek frissek (2024). Ellenőrizd:
> `vcpkg search sdl3`

## GitHub Actions runner telepítése

1. GitHub repo → Settings → Actions → Runners → **New self-hosted runner**
2. Kövesd a GitHub által generált install parancsokat (PowerShell)
3. A runner configolása közben add meg a következő label-eket:
   ```
   self-hosted,windows,gl
   ```
4. Indítsd el service-ként (hogy gépreindulás után is fusson):
   ```powershell
   .\svc.ps1 install
   .\svc.ps1 start
   ```

## Szükséges environment variable-k

Állítsd be system-wide (System Properties → Environment Variables):

```
VCPKG_ROOT = C:\vcpkg          (vagy a tényleges vcpkg telepítési helye)
```

A runner service-ként futtatva a system environment variable-kat örökli — nem kell `.env` fájl.

## Tesztelés

A GL tesztek a következő paranccsal futtathatók manuálisan (nem CI):

```powershell
cmake -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DBUILD_TESTS=ON -A x64
cmake --build build --config Release
ctest --test-dir build -L GL -C Release --output-on-failure
```

## Display konfiguráció (ha nincs monitor)

A `gl_test_main.cpp` rejtett (1×1 pixel) SDL ablakot hoz létre, de Windows-on az OpenGL context létrehozásához szükséges egy aktív display driver. Ha a runner-gép fejnélküli szerver:

- **Virtuális display adapter**: egy olcsó HDMI "dummy plug" megoldja
- Vagy: Remote Desktop kapcsolatban tartása (RDP aktiválja a display drivert)
- Vagy: Windows `Console0` session alatt futtatva (service + RDP login)
