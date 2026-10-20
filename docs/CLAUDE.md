# CLAUDE.md: Square Baby (SDL2, C++)

Reference file for working on `TheRealMoeid/SDL`. Read this before touching code. It describes what the project is, how it works, every problem I found by reading the source, and the order I plan to fix things in.

## 0. How to use this file

- The owner built this as a final semester presentation project about two years ago and wrote no documentation. It is now being cleaned up to go into a portfolio.
- Goal: a stable, readable, correctly behaving game with a proper README. Not a rewrite, not an engine, not a new library.
- Everything in sections 5 to 9 was checked against the actual source (`game.cpp`, `SDL.h`, `sdl.vcxproj`). Anything I could not see is listed in section 2.
- The game has never been compiled or run by me. Claims marked **(runtime check)** need to be confirmed by actually running it.
- Fix one bug per commit. Keep gameplay feel the same unless the owner approves a design change (see section 11, open questions).

## 1. What the project is

A small 2D survival game called **Square Baby** (window title). The player is a square sprite that continuously shrinks. Collecting health items makes it grow, fire items make it shrink, and a wandering enemy ends the run on contact. Grow to 600 px and you win. Shrink below 3 px or touch the enemy and you lose. Space restarts after either ending.

- Language: C++ (written as one big `main()`, C style)
- Libraries: SDL2, SDL2_image (PNG textures), SDL2_ttf (score text)
- Platform: Windows, Visual Studio (MSVC), no cross platform build
- Size: `game.cpp` is 396 lines, `SDL.h` is 337 lines. Only these two files are compiled.
- Repo state: 2 commits, no README, no license, no description, no tests.

## 2. What I could and could not see

Seen in full: `game.cpp`, `SDL.h`, `sdl.vcxproj`, the top level file listing.

Not seen (GitHub blocked the fetch or I did not open them), so verify locally:
- Contents of `assets/`, `sdl/` (almost certainly the SDL2 header files, because `SDL.h` does `#include "sdl/SDL.h"`), `x64/Debug/`
- `.gitignore` (an earlier review says it only contains `.vs/`), `sdl.sln`, `sdl.vcxproj.user`, `sdl.vcxproj.filters`
- The images themselves. I know their filenames only, not what they show.
- Whether the bundled `SDL2*.lib/.dll` are 32 or 64 bit, and which SDL versions they are.

There is also a prior review in `SDL_BUGS_AND_IMPROVEMENTS.md` (project file). I re-checked it against the code. Section 8 lists where it was right, overstated, or missing something.

## 3. Repository layout (as listed on GitHub)

```
SDL/
  game.cpp                 all gameplay, one main()
  SDL.h                    helper "library", declarations AND definitions, plus globals
  sdl.sln / sdl.vcxproj    Visual Studio solution and project (+ .filters, .user)
  sdl/                     SDL headers (SDL.h, SDL_ttf.h, SDL_image.h ...), not verified
  assets/                  contents not verified
  x64/Debug/               committed build output, not verified
  SDL2.lib SDL2.dll SDL2_image.lib SDL2_image.dll
  SDL2_ttf.lib SDL2_ttf.dll SDL2main.lib      committed binaries in the repo root
  consola.ttf              font loaded by the game (Microsoft Consolas, see license note)
  Images in the repo root (the game loads these, not the ones in assets/):
    BacheGherti.png  Hestler.png  Health.png  Fire.png  Enemy.png
    background.png   GameOver.png  Win.png
  Images that exist but are never referenced by the code:
    Bache.png  BacheGherti_Fast.png  Hestler_Fast.png  Bozorg.png
    Pause.png  speedBooster.png
```

The unused images suggest planned features that were never finished: a speed boost item, fast sprite variants, a pause screen. `SDL.h` also tracks a `pause` key (`P`) that `game.cpp` never reads.

Naming trap: the project has its own `SDL.h` in the root, and it includes the real SDL as `"sdl/SDL.h"`. The real one lives in the `sdl/` folder, the custom one is the root file. Easy to confuse.

## 4. Build and run

- Open `sdl.sln` in Visual Studio. Configurations: Debug/Release for Win32 and x64.
- `PlatformToolset` is `v145` in all four configurations. That is newer than Visual Studio 2022's `v143`, so on an older install the project will not even load a toolset. Retarget to the installed one if needed.
- Libraries are linked through `#pragma comment(lib, ...)` in `SDL.h`, not through project settings. The `.lib` files are found because they sit in the project folder.
- `SDL_MAIN_HANDLED` is defined in `SDL.h`, so plain `int main()` is used and `SDL2main.lib` is not needed.
- The three DLLs must sit next to the exe, and the exe must be started with the working directory set to the folder holding the images and `consola.ttf` (all asset paths are relative, no folder prefix).
- Subsystems: Debug configs and Release|Win32 use Console, **Release|x64 uses Windows**. With a plain `main` and `SDL_MAIN_HANDLED`, Release|x64 probably fails to link (entry point `WinMain` not found). **(runtime check)**
- Debug|x64 uses MultiByte character set, the others use Unicode. Harmless here, but inconsistent.

## 5. Architecture

### 5.1 `SDL.h` (helper layer)

A header that contains function bodies and global variables. Only one `.cpp` includes it, so it links today, but a second `.cpp` including it would cause duplicate symbol errors.

Globals defined in the header: `window`, `renderer`, `lastRun` (a `clock_t`), and a global `Events event` instance.

`struct Events` has booleans: `exit, left, right, up, down, space, pause`. They are level triggered (set on key down, cleared on key up). `SDL__Events()` pumps SDL events and returns a copy. `game.cpp` declares its own local `Events event;`, which shadows the global one. It works, just confusing.

| Function | Used by the game? | Notes |
|---|---|---|
| `SDL__CreateWindow(title, w, h)` | yes | Init SDL (video+timer) and TTF, create window and accelerated renderer (no vsync), clear to black. Returns 0 or 1. **Return value ignored by `main`.** |
| `SDL__Events()` | yes | Arrows, Space, P, window close. |
| `SDL__DrawText(text, x, y, size, "#RRGGBB")` | yes | Opens `consola.ttf` on every call. |
| `SDL__Destroy()` | yes | Destroys renderer/window, `TTF_Quit`, `SDL_Quit`. |
| `SDL__SetColor("#RRGGBB")` | indirectly | Parses hex, **also sets the renderer draw color** (side effect), returns `SDL_Color`. |
| `SDL__DrawPoint / DrawLine / DrawRect / FillRect / DrawCircle / FillCircle / Clear / DrawImage / Present` | no | Dead code for this game. |

### 5.2 `game.cpp` (everything else)

One `main()` containing setup, a `while(true)` loop, and cleanup. No functions, no structs, no classes. State is about 30 local variables. Items and states are encoded with sentinel values (`sizeHealth = -1` means "respawn me", `sizeFire = -1` means "no fire").

Setup: window 700x700, load 9 textures with `IMG_LoadTexture` (no null checks), set locals, compute the frame time as `1000 / refresh_rate` of display 0 (variable named `fps` but it is milliseconds per frame).

Textures: `A_Texture` = `BacheGherti.png` (small player), `B_Texture` = `Hestler.png` (big player), `C_Texture` = pointer to whichever of A/B is currently active (alias, not an owner), then `Health`, `Fire`, `Enemy`, `Background`, `GameOver`, `Win`. Note `Win_Texture` is loaded from `"win.png"` but the file is `Win.png`.

## 6. Game rules and constants

| Thing | Value |
|---|---|
| Window | 700x700 |
| Player start | size 50, centered at (325, 325) |
| Player speed | 3 px per frame per held arrow (frame based) |
| Player shrink | `size -= 8.3 * ln(size) * 0.3 * dt/1000` per frame, about 9.7 px/s at size 50, 11.5 px/s at 100 (time based) |
| Sprite swap | `size > 100` uses `Hestler.png`, else `BacheGherti.png` |
| Win | `size >= 600` (window minus 100) |
| Lose | `size < 3`, or touching the enemy while `size < 600` |
| Enemy | fixed 50x50, starts at (100, 100), moves 1 px per frame, picks a new random direction (up/down/right/left) every 70 frames |
| Health item | 50 px at spawn, shrinks 10 px/s (lives about 5 s), respawns somewhere random when it expires or is eaten |
| Health growth | `size += sizeHealth * 3 / ln(fairness_health)` |
| Fire item | appears after every 3 health items eaten (counted only while no fire is visible), 50 px, shrinks 10 px/s |
| Fire penalty | `size -= sizeFire * 2 / ln(fairness_fire)` |
| `fairness_*` | integer distance between player and item, measured **at spawn time** (stale by the time you pick it up) |
| Score | frame counter divided by 60, shown red at (0,0), font size 30. Only drawn while playing. |
| Controls | Arrow keys move, Space restarts on Game Over or Win, closing the window quits. `P` is read but unused. No Escape handling. |

## 7. Frame loop, in the order the code runs it

1. `SDL__Events()`, break on exit.
2. `SDL_RenderClear`, draw background.
3. `deltatime = now - pasttime` (ms since the previous frame start).
4. Draw player at the old position (only if playing).
5. Choose sprite for **next** frame by size (one frame of lag).
6. Shrink the player.
7. Draw enemy, then update enemy direction every 70 frames and move 1 px. This is not guarded by `gameover/winner`.
8. If `sizeHealth <= 0`: pick a random spot by rejection sampling, set `sizeHealth = 50`, compute `fairness_health`. Draw health item. Shrink it.
9. If `sizeFire <= 0 || !fire_visable`: pick a random spot, set `sizeFire = 50`, compute `fairness_fire`. **This runs every frame while the fire is hidden.** Draw fire if visible, shrink it, hide it when it reaches 0.
10. Movement. Each pressed arrow redraws the player at the old position, then moves by 3 px, with a boundary test done before the move.
11. Build corner coordinates for player, health, fire, enemy.
12. Health collision: grow, mark item eaten, increment `health_counter`, maybe show fire. Fire collision: shrink, hide fire. Enemy collision: `gameover = true`.
13. Draw score text (builds a `std::string`, opens the font file again).
14. `size < 3` sets `gameover`. If `gameover`, draw the Game Over image. If also Space: destroy all textures, reload them, reset a subset of variables.
15. `size >= 600` sets `winner`. Same pattern for the Win image and restart. The restart block is a copy paste of the Game Over one.
16. `SDL_RenderPresent`, then `SDL_Delay(fps - deltatime)` if `deltatime < fps`.

Rendering, input, logic and collision are interleaved. The player is drawn first, so items and enemy are drawn over it.

## 8. Known bugs (verified against the code)

Severity: **High** crash or undefined behavior or broken core loop, **Medium** wrong behavior, **Low** cosmetic or latent. "Review #" refers to `SDL_BUGS_AND_IMPROVEMENTS.md`.

### High

**B1. Double destroy of a texture on restart — FIXED**
Removed `SDL_DestroyTexture(C_Texture)` from both the Game Over and Win
restart blocks in `game.cpp`. `C_Texture` is an alias of `A_Texture`/
`B_Texture`, never an owner, so destroying A and B is sufficient.


**B2. Division by zero and absurd growth from `log(distance)`** (review #4, confirmed, with more detail)
`fairness_*` is an `int` distance. `ln(1) = 0` gives `x / 0.0 = inf`, so picking up an item at distance exactly 1 sets `size` to `+inf` (instant win) or `-inf` (instant loss). Distance 0 gives `ln(0) = -inf`, a zero change. Distances 2 to 3 give a huge multiplier (`1/ln(2)` is about 1.44, so `50*3/0.69` is about 217 px). Also the distance is stale (measured at spawn, not at pickup) and bigger distance gives a smaller reward, which is probably not what the owner meant. Needs a design decision (section 11) and a bounded formula, for example a fixed growth per health item.

**B3. Restart does not reset the whole state** (review #5, confirmed with a concrete case)
Not reset: `health_counter`, `fire_visable`, `counter_enemy_direction`, `enemy_direction`, `Random_*` positions, `winner`/`gameover` of the other kind. Concrete failure: if the fire was visible when you died, `fire_visable` stays `true`, and the restart sets `sizeFire = -1`, so the first frame respawns it and a fire is on screen right at the start. Fix: one `ResetGame()` that sets every game variable from a single list of initial values, used by both endings.

**B4. Unchecked failures that can crash — FIXED**
`SDL__CreateWindow`'s return value is now checked in `main` (note: it
returns 0 on success, 1 on failure — opposite of a naive truthy check,
confirmed against the actual source). `SDL_GetCurrentDisplayMode` /
`refresh_rate` is guarded with a `fps = 16` fallback instead of risking
integer division by zero. All nine `IMG_LoadTexture` calls (startup and
both restart blocks) now go through a `LoadTextureChecked` helper that
logs failures via `SDL_Log`. Note: a failed load still renders nothing
silently downstream (SDL no-ops on a null texture) — this fix makes
failures visible in the log, it does not make the game refuse to run
with missing assets. That remains a possible future improvement, not
done here.

### Medium

**B5. Wrong SDL_image initialization — FIXED**
`SDL__CreateWindow` in `SDL.h` now calls `SDL_Init`, `IMG_Init`, and
`TTF_Init` as three separate checked calls instead of OR'ing
`IMG_INIT_PNG | IMG_INIT_JPG` into `SDL_Init`'s flags. Also added
`IMG_Quit`/`TTF_Quit` to the existing window and renderer failure
cleanup paths, which previously only called `SDL_Quit`.

**B6. Spawn checks do not check overlap** (review #3, confirmed, with extra defects)
- Only the item's top left point is tested against the other rectangle, not full rectangle overlap.
- The enemy rectangle in both loops uses `Enemy_i + size` (the **player's** size) instead of 50.
- In the fire loop, the health test is reversed: it checks whether the **health item's** corner is inside the **fire** rectangle, using `sizeFire`, which is 0 or -1 at that time, so the test is effectively always false.
- Because the fire loop runs every frame while the fire is hidden (step 9 above), it wastes work and re-rolls `fairness_fire` constantly.
Fix: one reusable `bool Overlaps(const SDL_Rect&, const SDL_Rect&)`, used everywhere including collisions, and spawn the fire only when it is actually due.

**B7. Gameplay speed depends on frame rate** (review #6, narrowed)
Shrink, item lifetimes and the Space/score logic partly use `deltatime`, but player speed (3 px/frame), enemy speed (1 px/frame), the 70 frame direction timer and the score (`frames / 60`) are all per frame. On a 144 Hz display the frame time is 6 ms (about 166 fps), so the player moves about 2.7 times faster than on 60 Hz. The frame limiter is also wrong: `fps - deltatime` subtracts the **previous** frame's total time (which already included the previous delay), not the current frame's work time, so the pacing oscillates. Fix: use `deltatime` for all movement, and use vsync (`SDL_RENDERER_PRESENTVSYNC`) or a correct limiter.

**B8. Player can leave the window** (review #7, confirmed)
Boundary tests run before a 3 px step, so the player can end up to 2 px outside on any edge (up/left allow `>= 0` then subtract 3, so `y` can become -3). Growth is never clamped either, so eating a health item at an edge puts part of the bigger square outside the window. Fix: clamp the position after movement and after any size change.

**B9. Enemy behavior problems** (review improvement B, confirmed)
It stops completely when it hits a wall (the movement guard fails) and stays stuck until the next 70 frame direction roll. It keeps moving during Game Over and Win. Its direction and counter are not reset on restart. Its speed is frame based (B7).

**B10. Game state is two independent booleans** (review improvement C)
`gameover` and `winner` can in principle both be true (nothing prevents the collision/eating blocks from running during an ending, because they are not guarded). Use `enum class GameState { Playing, Paused, GameOver, Victory }`.

**B11. Rendering and logic are interleaved, with redundant draws** (review #12, extended)
The player is drawn before it is moved or resized. Each pressed arrow draws the player again at the old position (up to 5 draws per frame), which doubles alpha edges and is pointless. Sprite choice lags one frame. Do input, update, collide, render, present as separate stages.

### Low or latent

**B12. Spawn loop can in theory never end** (review #11)
Today it cannot practically happen, because only the top left point is tested. After B6 is fixed with real overlap tests and a large player (up to 599 px in a 700 px window), valid spots get rare. Add an attempt limit and a fallback.

**B13. Score is not time based**, and it opens the font file every frame (see helper issues). `score_counter / 60` assumes exactly 60 fps. There is also an unused shadowed `const char* score = 0;` near the top of `main`.

**B14. Narrowing conversions.** `SDL_Rect rect = { Character_i, Character_j, size, size };` initializes `int` fields from `double` values. Expect compiler warnings, and a hard error under stricter settings.

**B15. Uninitialized variables.** `Random_Health_i/j` and `fairness_health/fire` start uninitialized. They are written before first use in practice (health spawns in frame 1), but this is fragile.

### Corrections to the earlier review

- **Review #8 ("player is immune to the enemy when large") is not a real bug.** Immunity only starts at `size >= 600`, and that is exactly when `winner` becomes true in the same frame. There is nothing to protect against. The real issue is the missing state machine (B10).
- **Review #2** is accurate but overstated as "high" (see B5).
- **Review #6** is partially true only (see B7: some things are already time based).
- **Review #10 (image leak)** is real, but `SDL__DrawImage` is never called by the game, so it does not affect gameplay.
- The review misses B3's concrete symptom, the every frame fire respawn, the enemy size error, the null display refresh rate, the ignored init result, the Release|x64 subsystem problem, and the wrong thick line math below.

## 9. Helper library issues (`SDL.h`)

- **Globals and bodies in a header.** `window`, `renderer`, `lastRun`, `event` and all functions are defined in the header. Move declarations to a header and definitions to a `.cpp`, or wrap everything in a small class that owns window and renderer. Rename the file (for example `SDLHelpers.hpp`) so it stops sharing a name with SDL's own `SDL.h`.
- **`SDL__DrawText`**: opens and closes `consola.ttf` on every call (every frame for the score). If `TTF_RenderUTF8_Solid` fails it returns without `TTF_CloseFont` (leak). `SDL_CreateTextureFromSurface` is not checked. Load the font once and cache it.
- **`SDL__DrawImage`**: never destroys the texture it creates (leak per call), does not check texture creation, queries a size it never uses.
- **`SDL__SetColor`**: checks only `strlen == 7`, not the leading `#` or that the digits are hex. Also changes the renderer draw color as a side effect, so after drawing text the draw color is red (harmless only because the background image covers the clear color).
- **`SDL__DrawLine`**: divides by the line length, which is 0 for a zero length line (NaN cast to int is undefined). The thickness offset `(i*dx, -i*dy)` is applied **along** the line instead of perpendicular to it (it should use `(-dy, dx)`), so for a horizontal line the extra lines overlap and thickness does nothing.
- **`SDL__DrawRect`**: `w - 2*i` can go negative for a large `width`.
- **`SDL__DrawCircle` / `FillCircle`**: per pixel loops over a bounding square, slow for large radii.
- **`SDL__Present`**: uses `clock()` (CPU time) for pacing, and the game does not call it anyway.
- **`SDL__CreateWindow`**: seeds `srand` (so does `main`), unconditionally does the init work again if called twice, incomplete cleanup on failure paths.

Since only four helper functions are actually used, the simplest cleanup is to delete the dead drawing helpers or move them to a separate file, rather than fix code nobody calls.

## 10. Build, repo and portfolio issues

- **Binaries in the repo root** (`.lib`, `.dll`) and committed build output in `x64/Debug`. Replace with a documented dependency setup (vcpkg or the official SDL2 development packages), record SDL2, SDL2_image and SDL2_ttf versions, and make sure the architecture matches the target.
- **`.gitignore`** should cover `Debug/`, `Release/`, `x64/`, `.vs/`. Inspect `x64/Debug` first before ignoring it, in case something needed is only there.
- **Assets are duplicated and scattered.** The game reads the root copies, there is also an `assets/` folder. Choose one folder, add a path prefix in the code, and set the Visual Studio debugging working directory so the game runs from the IDE and from a double click.
- **Case mismatch** `win.png` (code) vs `Win.png` (file). Fine on Windows, breaks elsewhere.
- **`consola.ttf` is Microsoft's Consolas font.** Redistributing it in a public repo is a licensing risk. Replace it with an open licensed font (for example DejaVu Sans Mono or Liberation Mono) and credit it.
- **Sprites.** I have not seen the images. Before publishing, look at every image and its filename (`Hestler.png`, `BacheGherti.png`, `Bozorg.png`, and so on), and rename or replace anything that is not suitable for a public portfolio. Also confirm you have the right to use each image.
- **No README, no license, no description, no topics, no screenshots or GIF.** All of these matter for a portfolio piece.
- **Toolset `v145`** needs a recent Visual Studio. Document it, or retarget to `v143`.
- Optional later: add a `CMakeLists.txt` so it also builds on Linux/macOS.

## 11. Open questions for the owner

Ask before changing these, they are design decisions, not bugs:
1. What was `fairness` (distance based growth) meant to do? Should closer items give more, less, or the same?
2. Should the enemy be able to kill a big player? (Currently irrelevant because 600 means win.)
3. What were the unused images (`speedBooster`, `Pause`, `*_Fast`, `Bozorg`, `Bache`) for? Finish them, or delete them?
4. Keep Visual Studio only, or also support CMake?
5. Which license for the code, and which font to ship?

## 12. Fix plan (in this order)

**Step 1, stop the crashes and leaks (B1, B4, B5, helper text/image leaks).**
Single texture ownership, load once. Check every init and load call, print a clear message, exit cleanly. Guard `refresh_rate == 0`. Proper `IMG_Init`.

**Step 2, game correctness (B2, B3, B6, B8, B10).**
Add `Overlaps()`. Add `ResetGame()` and a `GameState` enum. Replace the log formula with a bounded one. Clamp positions and sizes. Fix spawn logic (only spawn the fire when due, real overlap tests, attempt limit).

**Step 3, timing (B7, B9, B13).**
Delta time for every movement and timer, vsync or a correct limiter, enemy timer in seconds with wall bouncing, score from elapsed time.

**Step 4, structure.**
Split into `Game`, `Player`, `Enemy`, `Collision`, `Renderer` files, no mutable globals, `SDLHelpers.hpp/.cpp` or a small renderer class. Do this only after steps 1 to 3 are stable.

**Step 5, repo polish.**
README with screenshot and controls, license, `.gitignore`, assets folder, dependency notes, font replacement, remove committed binaries and build output.

### Tests worth writing (logic only, no SDL window)

- `Overlaps()`: overlapping, touching edges, separate, contained.
- Growth: distance 0, 1, 2, typical values, never NaN/inf, size stays inside min and max.
- Clamp: player at all four edges and after growth.
- Reset after losing and after winning: every variable equals its initial value (including `fire_visable`, `health_counter`, enemy direction).
- Spawn: result never overlaps player, enemy or the other item, and terminates when no valid spot exists.
- Movement and timers give the same result at 30, 60 and 144 fps worth of steps.
- Missing asset: clear error, no crash.

## 13. Working rules for future sessions

- Keep SDL2 and C++. Do not switch libraries or add an engine.
- Smallest change that fixes the problem, one logical fix per commit, message says which bug ID from this file it addresses.
- When a bug is fixed, update section 8 here (mark it fixed with the commit) so this file stays true.
- Compile and run after every change. If I cannot run it, say so instead of claiming it works.
- Do not delete assets or dead code without the owner's answer to the open questions above.
- Keep the owner's style where reasonable (names like `Character_i`, section comment headers) unless that is part of what is being refactored.
