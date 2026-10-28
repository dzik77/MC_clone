# MC_clone

Gra wokselowa (voxel) z trybem wieloosobowym, napisana od zera w **C++20** i **OpenGL 4.6**.
Projekt składa się z autorytatywnego serwera, który generuje świat i liczy fizykę, oraz klienta, który renderuje świat i przewiduje ruch gracza.

## Funkcje

**Świat**
- Proceduralne generowanie terenu (FastNoise2): szum *continentalness*, *erosion* i *peaks & valleys* mapowany przez splajny na wysokość terenu
- Bloki: ziemia, trawa, kamień, drewno, liście, woda (poziom wody `y = 62`)
- Drzewa rozmieszczane deterministycznie (hash Squirrel3 z seedem)
- Świat podzielony na kolumny chunków 16×256×16 (16 chunków 16³ w pionie)
- Generowanie terenu na 4 wątkach roboczych serwera

**Sieć** (ENet, UDP, port `1234`)
- Serwer autorytatywny, 60 ticków na sekundę, do 32 graczy
- Predykcja ruchu po stronie klienta i rekoncyliacja ze stanem z serwera
- Strumieniowanie chunków w promieniu widzenia (domyślnie 32 chunki) oraz ich wyładowywanie
- Prosta kompresja chunków (warstwy i chunki jednolite wysyłane jako jeden bajt), patrz `core/include/ChunkCompression.hpp`
- Stawianie i niszczenie bloków synchronizowane między graczami

**Renderowanie**
- Wierzchołki spakowane w jednym `uint32` (pozycja, tekstura, kierunek ściany, UV, ambient occlusion)
- Jeden duży VBO/EBO z własnym alokatorem pamięci VRAM (`VramAllocator`)
- `glMultiDrawElementsIndirect` z persistent-mapped buffer na komendy rysowania
- Tablica tekstur (`GL_TEXTURE_2D_ARRAY`) 16×16
- Osobny przebieg dla przezroczystej wody
- Widok z pierwszej osoby i dwa widoki z trzeciej osoby (z kolizją kamery)

## Struktura projektu

```
core/      Kod wspólny dla klienta i serwera: bloki, pakiety, kompresja chunków, fizyka
server/    Serwer: pętla ticków, generowanie świata, obsługa graczy
client/    Klient: okno, renderowanie, sieć, sterowanie
  Shaders/   Shadery GLSL (świat, gracz)
  textures/  Tekstury bloków
deps/      Submoduł FastNoise2
```

> Katalog `src/` zawiera starszą, jednoprocesową wersję gry i nie jest budowany przez CMake.

## Wymagania

- Kompilator z obsługą C++20 (GCC 11+, Clang 14+ lub MSVC 2022)
- CMake 3.14+ (zalecane 3.26+)
- Karta graficzna i sterownik z obsługą **OpenGL 4.6**
- Git (zależności są pobierane przy konfiguracji)
- Linux: nagłówki X11 i OpenGL, np. na Ubuntu/Debianie:
  ```bash
  sudo apt install build-essential cmake libx11-dev libxrandr-dev libxinerama-dev \
                   libxcursor-dev libxi-dev libgl1-mesa-dev libwayland-dev libxkbcommon-dev
  ```

GLFW, ENet, GLM i Dear ImGui pobiera automatycznie `FetchContent`. GLAD i stb_image są w repozytorium.

## Budowanie

```bash
git clone --recurse-submodules <adres-repozytorium> MC_clone
cd MC_clone

# jeśli sklonowano bez --recurse-submodules:
git submodule update --init --recursive

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Powstają dwa pliki wykonywalne: `build/server_app` i `build/client_app`. Shadery i tekstury są kopiowane obok klienta automatycznie przy każdym buildzie.

## Uruchamianie

Najpierw serwer, potem klient (w osobnych terminalach):

```bash
cd build
./server_app
./client_app
```

Klient łączy się z `127.0.0.1:1234` i ponawia próbę, dopóki serwer nie odpowie. Klient trzeba uruchamiać z katalogu, w którym leży, bo shadery i tekstury są ładowane ze ścieżek względnych.

W konsoli serwera wpisz `exit`, żeby go zatrzymać.

## Sterowanie

| Klawisz | Akcja |
|---|---|
| `W` `A` `S` `D` | Ruch |
| `Spacja` | Skok / w górę (wolna kamera) |
| `Lewy Shift` | W dół (wolna kamera) |
| Lewy przycisk myszy | Niszczenie bloku (oraz przechwycenie kursora) |
| Prawy przycisk myszy | Stawianie bloku |
| `F5` | Przełączanie widoku: 1. osoba → 3. osoba z tyłu → 3. osoba z przodu |
| `F2` | Wolna kamera (włącz/wyłącz) |
| `P` | Włączanie/wyłączanie fizyki |
| `F1` | Zwolnienie kursora myszy |
| `Esc` | Wyjście |

## Opcje deweloperskie

W `CMakeLists.txt` są zakomentowane definicje, które można włączyć:

- `DEBUG` dla `client_app`: callback debugowy OpenGL i nakładka Dear ImGui
- `PERFORMANCE_DEBUG` dla `server_app`: czasy poszczególnych etapów każdego ticku

Parametry świata są w `core/include/WorldConfig.hpp`, seed i poziom wody w `server/include/World.hpp`, a zasięg widzenia i TPS w `server/include/Server.hpp`.

## Biblioteki

- [GLFW](https://github.com/glfw/glfw): okno i wejście
- [GLAD](https://github.com/Dav1dde/glad): ładowanie funkcji OpenGL
- [GLM](https://github.com/g-truc/glm): matematyka
- [ENet](https://github.com/lsalzman/enet): sieć UDP
- [FastNoise2](https://github.com/Auburn/FastNoise2): szum do generowania terenu
- [Dear ImGui](https://github.com/ocornut/imgui): interfejs debugowy
- [stb_image](https://github.com/nothings/stb): wczytywanie tekstur
- [unordered_dense](https://github.com/martinus/unordered_dense): szybkie hash mapy
