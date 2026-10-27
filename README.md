# Square Baby

A small 2D survival game built with SDL2. You play a square that
constantly shrinks over time. Collect health items to grow, avoid fire
items and the wandering enemy, and reach full size to win.

## Rules

- **Move**: Arrow keys
- **Restart**: Space, after winning or losing
- Grow to **500px** to win. Shrink below **3px**, or touch the enemy
  while under 500px, to lose.
- Health items make you grow; fire items (which appear after every 3
  health items eaten) make you shrink.

## Building

### Visual Studio (Windows)

Open `sdl.sln`. The project targets `PlatformToolset v145`; retarget
to your installed toolset if Visual Studio reports it as missing.
Debug/Release and Win32/x64 configurations are all set up; the three
SDL DLLs are copied alongside the executable from the project folder.

### CMake (Windows / Linux / macOS)

Requires SDL2, SDL2_image and SDL2_ttf development packages
discoverable via pkg-config (e.g. `apt install libsdl2-dev
libsdl2-image-dev libsdl2-ttf-dev` on Debian/Ubuntu, or `brew install
sdl2 sdl2_image sdl2_ttf` on macOS).

```sh
cmake -B build
cmake --build build
```

The built executable expects an `assets/` folder next to it, containing
`assets/images/` (the PNGs) and `assets/fonts/DejaVuSansMono.ttf`; the
CMake build copies this folder there automatically.

## License

MIT - see [LICENSE](LICENSE).

`DejaVuSansMono.ttf` is distributed under the
[DejaVu Fonts license](https://dejavu-fonts.github.io/License.html),
replacing the original (proprietary) Consolas font.
