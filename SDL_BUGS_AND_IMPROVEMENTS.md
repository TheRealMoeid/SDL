# SDL_BUGS_AND_IMPROVEMENTS.md: Square Baby (SDL2, C++)

Bug IDs (B1 to B15) match `CLAUDE.md`. This version replaces the earlier review after every claim was re-checked against `game.cpp`, `SDL.h` and `sdl.vcxproj`.

**Scope note:** based on reading the source only. The game was not compiled or run. Items marked *(runtime check)* must be confirmed by running it. I could not open `assets/`, `sdl/`, `x64/Debug`, `.gitignore` or the images.

## 1. What changed from the previous review

| Earlier item | Verdict | Change |
|---|---|---|
| #1 Double destroy of texture | Confirmed | Kept as B1 |
| #2 SDL_image init flags | Confirmed, severity lowered | Now Medium (B5). PNG loading probably still works. |
| #3 Placement checks | Confirmed, extended | B6, adds wrong enemy size, reversed fire test, per frame fire respawn |
| #4 `log(distance)` | Confirmed, extended | B2, adds distance 1 gives infinity (instant win or loss), stale distance |
| #5 Incomplete restart | Confirmed, concrete symptom found | B3, fire can be visible right after restart |
| #6 Frame rate dependence | Partly true | B7, shrink and item timers are already time based |
| #7 Leaves the window | Confirmed | B8, also growth is never clamped |
| #8 Immune to enemy when large | **Removed, not a bug** | Size 600 is exactly when the win triggers, so there is nothing to protect |
| #9 Text error handling | Confirmed | Helper issues, plus the font is reopened every frame |
| #10 Image texture leak | Confirmed, but latent | `SDL__DrawImage` is never called by the game |
| #11 Infinite spawn loop | Latent | B12, practically impossible today, becomes possible after B6 is fixed |
| #12 Logic and rendering mixed | Confirmed, extended | B11, redundant draws |
| not listed | **New** | Ignored init result, refresh rate 0 crash, Release x64 subsystem, wrong thick line math, enemy bugs, unchecked texture loads |

## 2. Project summary

- Repo: `TheRealMoeid/SDL`, game name **Square Baby**
- C++, SDL2, SDL2_image, SDL2_ttf, Windows and Visual Studio (toolset `v145`)
- Only `game.cpp` (396 lines, one `main()`) and `SDL.h` (337 lines, helpers and globals) are compiled
- Gameplay: the player square shrinks over time. Health items make it grow, fire items shrink it, an enemy kills it on touch. Size 600 wins, size under 3 loses, Space restarts.

## 3. Bugs

### High

**B1. Texture destroyed twice on restart** (`game.cpp`, both restart blocks)
`C_Texture` is an alias of `A_Texture` or `B_Texture`, but the restart code destroys A, B and C.
Fix: destroy only A and B, or drop `C_Texture` and pick the active texture when drawing. Best: load textures once and destroy once at exit, not on every restart.

**B2. Division by zero and extreme growth from `log(distance)`** (`EatingHealthBar`, `EatingFireBar`)
`size += sizeHealth * 3 / log(fairness_health)` and `size -= sizeFire * 2 / log(fairness_fire)`.
- Distance 1: `log(1) = 0`, result is infinity. Size becomes infinite, which means an instant win or loss.
- Distance 0: `log(0)` is negative infinity, so the change is zero.
- Distance 2 to 3: very large multiplier (about 217 px of growth).
- The distance is measured when the item spawns, not when it is picked up, and farther items give smaller rewards.
Fix: replace with a bounded formula (for example a fixed growth per item, clamped between min and max size). Needs a design decision from the owner.

**B3. Restart resets only part of the state** (both restart blocks)
Not reset: `health_counter`, `fire_visable`, `counter_enemy_direction`, `enemy_direction`, item positions.
Concrete symptom: if fire was visible when you died, it is still visible after restart (the restart sets `sizeFire = -1`, the next frame respawns it).
Fix: one `ResetGame()` that sets all game variables, used by both endings.

**B4. Unchecked failures that can crash**
- `main` ignores the return value of `SDL__CreateWindow`, so a failed window leaves `renderer` null and the loop still runs.
- `SDL_GetCurrentDisplayMode` is unchecked. If `refresh_rate` is 0, `1000 / refresh_rate` is an integer division by zero. *(runtime check)*
- Nine `IMG_LoadTexture` calls are unchecked. A missing file (wrong working directory, or `win.png` vs `Win.png` on a case sensitive system) silently draws nothing.
- `SDL__CreateWindow` does not clean up every initialized subsystem on all failure paths.
Fix: check every init and load, print the SDL error, exit cleanly.

### Medium

**B5. Wrong SDL_image initialization** (`SDL.h`, `SDL__CreateWindow`)
`IMG_INIT_PNG | IMG_INIT_JPG` are passed to `SDL_Init`, and `IMG_Init()` is never called. Incorrect, though loading probably still works because SDL_image loads codecs on demand.
Fix: initialize SDL and SDL_image separately, check the result.

**B6. Item placement does not test overlap** (random placement loops)
- Only the item's top left point is tested, not rectangle overlap.
- The enemy rectangle uses `size` (the player's size) instead of 50.
- In the fire loop the health test is reversed and uses `sizeFire`, which is 0 or -1 there, so it never rejects anything.
- The fire loop runs every frame while the fire is hidden, re-rolling position and distance each time.
Fix: one `Overlaps(rectA, rectB)` function used for spawning and collisions, and spawn the fire only when it is due.

**B7. Speed depends on frame rate** (movement, enemy, score, limiter)
Player speed (3 px per frame), enemy speed (1 px per frame), the 70 frame direction timer and the score (`frames / 60`) are per frame. Shrink and item lifetimes already use `deltatime`. On a 144 Hz display the frame time is 6 ms, so the player moves about 2.7 times faster than at 60 Hz.
The limiter also subtracts the previous frame's total duration (which already included its delay) instead of the current frame's work time, so pacing oscillates.
Fix: multiply every movement by `deltatime`, use vsync or a correct limiter, time the enemy direction in seconds, compute score from elapsed time.

**B8. Player can leave the window**
Edges are tested before a 3 px step, so the player can end up to 2 px outside (for example `y = -3`). Growth is never clamped, so eating an item at an edge pushes the bigger square outside.
Fix: clamp the position after movement and after every size change.

**B9. Enemy problems**
It stops moving when it hits a wall until the next direction roll, keeps moving during Game Over and Win, does not reset its direction or counter on restart, and its speed is frame based.
Fix: bounce off walls, update only while playing, reset on restart, use time based movement and timers.

**B10. State kept in two booleans** (`gameover`, `winner`)
Contradictory states are possible because eating and collision blocks run during an ending.
Fix: `enum class GameState { Playing, Paused, GameOver, Victory }`.

**B11. Rendering mixed with logic, redundant draws**
The player is drawn before it moves or resizes, then redrawn once per pressed arrow at the old position (up to 5 draws per frame). The sprite choice lags one frame.
Fix: stages per frame: events, update, collisions, render, present.

### Low or latent

**B12. Spawn loop could never end.** Practically impossible now. After B6, a very large player leaves few free spots. Add an attempt limit and a fallback.
**B13. Score text.** Frame based and reopens the font file every frame. Unused shadowed `const char* score = 0;` near the top of `main`.
**B14. Narrowing conversions.** `SDL_Rect` initialized with `double` values gives warnings, errors under stricter settings.
**B15. Uninitialized variables.** `Random_Health_i/j`, `fairness_health`, `fairness_fire`. Set before first use today, but fragile.

## 4. Helper library issues (`SDL.h`)

| Problem | Consequence | Fix |
|---|---|---|
| Globals (`window`, `renderer`, `lastRun`, `event`) and function bodies in a header | Duplicate symbol errors if a second `.cpp` includes it | Declarations in a header, definitions in a `.cpp`, or a class owning window and renderer |
| Custom file named `SDL.h` (it includes the real one as `"sdl/SDL.h"`) | Easy to confuse with the library header | Rename, for example `SDLHelpers.hpp` |
| `SDL__DrawText` opens the font on every call | File I/O every frame | Load once and cache |
| `SDL__DrawText` returns without closing the font when rendering fails, and does not check texture creation | Leak, possible null use | Check each call, release on every path |
| `SDL__DrawImage` never destroys its texture, does not check creation | Leak per call (not used by the game) | Destroy after drawing, check results |
| `SDL__SetColor` checks length only, also sets the renderer draw color | Bad input read as a wrong color, hidden side effect | Validate `#` and hex digits, separate parsing from drawing |
| `SDL__DrawLine` divides by length, offsets thickness **along** the line | NaN on zero length, thickness has no effect on horizontal lines | Guard zero length, use the perpendicular `(-dy, dx)` |
| `SDL__DrawRect` uses `w - 2*i` | Negative size for large widths | Validate dimensions |
| `SDL__DrawCircle`, `FillCircle` loop over a square | Slow for large radii | Midpoint algorithm or skip |
| `SDL__Present` uses `clock()` (CPU time) | Poor frame pacing (not used by the game) | `SDL_GetTicks64` or `SDL_GetPerformanceCounter`, or vsync |
| `SDL__CreateWindow` incomplete cleanup on failure | Leaked init state | Unwind every initialized subsystem |

Only `SDL__CreateWindow`, `SDL__Events`, `SDL__DrawText` and `SDL__Destroy` are used. The other drawing helpers are dead code and can be deleted or moved to a separate file.

## 5. Gameplay improvements

- **Growth and shrink:** shrink in pixels per second, a fixed growth per health item, a minimum and maximum size, never negative or infinite.
- **Enemy:** timer based direction changes, bounce at walls, optional chase when the player is near, configurable speed.
- **Game states:** pause (`P` is already read but unused), a restart prompt, optionally a main menu.
- **Unfinished assets:** `speedBooster.png`, `Pause.png`, `*_Fast.png`, `Bozorg.png`, `Bache.png` are never used. Either build those features or remove the files.
- **Score:** time based, with a best score shown on the end screens.

## 6. Build and repository

- **Toolset `v145`** needs a recent Visual Studio. Document it or retarget to `v143`.
- **Release|x64** uses the Windows subsystem with a plain `main` and `SDL_MAIN_HANDLED`, so it probably fails to link. Use Console, or add `WinMain`, or set the entry point. *(runtime check)*
- **Binaries in the repo root** (`SDL2*.lib/.dll`) and committed `x64/Debug` output. Use vcpkg or documented SDL2 development packages, record versions, match the architecture. Inspect `x64/Debug` before ignoring it.
- **`.gitignore`** should cover `Debug/`, `Release/`, `x64/`, `.vs/`.
- **Assets:** images exist both in the root and in `assets/`. The game reads the root copies through relative paths. Pick one folder, add a path prefix, set the Visual Studio debugging working directory, print clear errors for missing files.
- **Case mismatch:** `win.png` in code, `Win.png` on disk.
- **`consola.ttf`** is Microsoft's Consolas. Replace with an open licensed font and credit it.
- **Sprites:** look at every image and filename (`Hestler.png`, `BacheGherti.png`, `Bozorg.png`) and make sure they are suitable and legal to show in a public portfolio.
- **Missing:** README (controls, screenshot or GIF, build steps), license, repo description and topics.

Suggested layout:

```text
SDL/
  src/        main.cpp Game.cpp Player.cpp Enemy.cpp Collision.cpp Renderer.cpp
  include/    Game.hpp Player.hpp SDLHelpers.hpp
  assets/     images/ fonts/
  tests/
  README.md  LICENSE  .gitignore
```

## 7. Priorities

| Level | Work |
|---|---|
| P0 | B1, B4, B5, text and image leaks: ownership and error checks |
| P1 | B2, B3, B6, B8, B10: growth formula, reset, overlap, clamping, state enum |
| P2 | B7, B9, B13: delta time, limiter or vsync, enemy, score |
| P3 | Split into Game, Player, Enemy, Collision, Renderer, remove mutable globals |
| P4 | README, license, `.gitignore`, assets, dependency notes, font replacement |

## 8. Tests (logic only, no SDL window)

- `Overlaps()` for overlapping, touching, separate, contained rectangles
- Growth for distance 0, 1, 2 and typical values: never NaN or infinity, size stays within limits
- Clamping at all four edges and after growth
- Reset after losing and after winning: every variable returns to its initial value, including `fire_visable`, `health_counter`, enemy direction
- Spawning never overlaps the player, enemy or the other item, and terminates when no spot exists
- Same movement and timing result for 30, 60 and 144 fps worth of steps
- Missing asset gives a clear error and no crash

## 9. Recommendation

Do not rewrite the game and do not add an engine. First make it stable: it builds, restarts cleanly, handles collisions correctly, and plays the same on every machine. Then split the code into components. Before changing gameplay rules, ask the owner the open questions in `CLAUDE.md` section 11 (what the distance based growth should do, what to do with the unused assets, which license and font).

Included is some information about a GitHub repository and its language composition.
repo: TheRealMoeid/SDL
repo ID: 1392982276
The language composition of this repo is [{"name":"C","percent":96.9},{"name":"C++","percent":1.8},{"name":"Other","percent":1.3}]
