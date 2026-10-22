# CLAUDE.md: Square Baby (SDL2, C++)

Reference file for working on `TheRealMoeid/SDL`. Read this before touching code. It describes what the project is, how it works, every problem I found by reading the source, and the order I plan to fix things in.

## 0. How to use this file

- The owner built this as a final semester presentation project about two years ago and wrote no documentation. It is now being cleaned up to go into a portfolio.
- Goal: a stable, readable, correctly behaving game with a proper README. Not a rewrite, not an engine, not a new library.
- Everything in sections 5 to 9 was checked against the actual source (`game.cpp`, `SDL.h`, `sdl.vcxproj`). Anything I could not see is listed in section 2.
- The game has been compiled and run by the owner after each Step 1-3 fix (see sections 8 and 12); claims marked **(runtime check)** that remain are still unconfirmed.
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
- The renderer is now created with `SDL_RENDERER_PRESENTVSYNC` (Step 3, see section 8 B7), so frame pacing follows the display's actual refresh rate instead of a manual delay.

## 5. Architecture

### 5.1 `SDL.h` (helper layer)

A header that contains function bodies and global variables. Only one `.cpp` includes it, so it links today, but a second `.cpp` including it would cause duplicate symbol errors.

Globals defined in the header: `window`, `renderer`, `lastRun` (a `clock_t`), and a global `Events event` instance.

`struct Events` has booleans: `exit, left, right, up, down, space, pause`. They are level triggered (set on key down, cleared on key up). `SDL__Events()` pumps SDL events and returns a copy. `game.cpp` declares its own local `Events event;`, which shadows the global one. It works, just confusing.

| Function | Used by the game? | Notes |
|---|---|---|
| `SDL__CreateWindow(title, w, h)` | yes | Init SDL (video+timer), IMG, and TTF separately (B5), create window and accelerated, vsync'd renderer (Step 3), clear to black. Returns 0 or 1. Return value is now checked by `main` (B4). |
| `SDL__Events()` | yes | Arrows, Space, P, window close. |
| `SDL__DrawText(text, x, y, size, "#RRGGBB")` | yes | Opens `consola.ttf` on every call. |
| `SDL__Destroy()` | yes | Destroys renderer/window, `TTF_Quit`, `SDL_Quit`. |
| `SDL__SetColor("#RRGGBB")` | indirectly | Parses hex, **also sets the renderer draw color** (side effect), returns `SDL_Color`. |
| `SDL__DrawPoint / DrawLine / DrawRect / FillRect / DrawCircle / FillCircle / Clear / DrawImage / Present` | no | Dead code for this game. |

### 5.2 `game.cpp` (everything else)

One `main()` containing setup, a `while(true)` loop, and cleanup. No functions besides `LoadTextureChecked`, `Overlaps`, and `ResetGame` (added in Steps 1-2), no structs besides `GameState` (added in Step 2), no classes. State is about 30 local variables. Items and states are encoded with sentinel values (`sizeHealth = -1` means "respawn me", `sizeFire = -1` means "no fire").

Setup: window 700x700, load 9 textures through `LoadTextureChecked` (logs failures, B4), set locals.

Textures: `A_Texture` = `BacheGherti.png` (small player), `B_Texture` = `Hestler.png` (big player), `C_Texture` = pointer to whichever of A/B is currently active (alias, not an owner, B1), then `Health`, `Fire`, `Enemy`, `Background`, `GameOver`, `Win`. Note `Win_Texture` is loaded from `"win.png"` but the file is `Win.png` (still unresolved, see section 10).

## 6. Game rules and constants

| Thing | Value |
|---|---|
| Window | 700x700 |
| Player start | size 50, centered at (325, 325) |
| Player speed | `PLAYER_SPEED = 180.0` px/sec, time based (Step 3; matches the old 3px/frame feel at 60fps, deliberately not scaled by refresh rate) |
| Player shrink | `size -= 8.3 * ln(size) * 0.3 * dt/1000` per frame, about 9.7 px/s at size 50, 11.5 px/s at 100 (time based, unchanged) |
| Sprite swap | `size > 100` uses `Hestler.png`, else `BacheGherti.png` |
| Win | `size >= 600` (window minus 100) |
| Lose | `size < 3`, or touching the enemy while `size < 600` |
| Enemy | fixed 50x50, starts at (100, 100), moves `ENEMY_SPEED = 60.0` px/sec (Step 3; matches old 1px/frame @60fps), picks a new random direction (up/down/right/left) every `ENEMY_DIRECTION_INTERVAL = 70/60` seconds (Step 3; matches old 70-frame timer @60fps) |
| Health item | 50 px at spawn, shrinks 10 px/s (lives about 5 s), respawns somewhere random when it expires or is eaten |
| Health growth | `size += HEALTH_GROWTH` (flat `40.0` px per item, fixed B2; was log-distance based) |
| Fire item | appears after every 3 health items eaten (counted only while no fire is visible), 50 px, shrinks 10 px/s |
| Fire penalty | `size -= FIRE_PENALTY` (flat `40.0` px per item, fixed B2; was log-distance based) |
| `fairness_*` | still computed at spawn (`sqrt` distance) but no longer read anywhere — dead code left over from the pre-B2 formula, not yet removed |
| Score | `score_time`, a `double` accumulated as elapsed seconds via `deltatime` while `Playing` (fixed B13; was a frame counter divided by 60). Displayed as `(int)score_time`, red at (0,0), font size 30. Only drawn while playing. |
| Controls | Arrow keys move, Space restarts on Game Over or Win, closing the window quits. `P` is read but unused. No Escape handling. |

## 7. Frame loop, in the order the code runs it

1. `SDL__Events()`, break on exit.
2. `SDL_RenderClear`, draw background.
3. `deltatime = now - pasttime` (ms since the previous frame start).
4. Draw player at the old position (only if playing).
5. Choose sprite for **next** frame by size (one frame of lag).
6. Shrink the player.
7. Draw enemy, then update enemy direction using `ENEMY_DIRECTION_INTERVAL` and move at `ENEMY_SPEED` px/sec, both via `deltatime` (fixed B7/B9 timing, Step 3; was frame-count based). This is not guarded by `state` (enemy still moves during Game Over/Victory, see B9).
8. If `sizeHealth <= 0`: pick a random spot using `Overlaps()` against the player, enemy, and fire (fixed B6; was corner-in-rect checks with a wrong enemy size), set `sizeHealth = 50`, compute `fairness_health` (dead, see section 6). Draw health item. Shrink it.
9. If `sizeFire <= 0 || !fire_visable`: pick a random spot using `Overlaps()` against the enemy, player, and health (fixed B6; the old health check was backwards and effectively always false), set `sizeFire = 50`, compute `fairness_fire` (dead, see section 6). **This still runs every frame while the fire is hidden** (not changed by B6). Draw fire if visible, shrink it, hide it when it reaches 0.
10. Movement. Each pressed arrow redraws the player at the old position, then moves at `PLAYER_SPEED` px/sec via `deltatime` (fixed B7, Step 3; was a fixed 3px/frame step); position is clamped to `[0, window_size - size]` afterward (fixed B8; was a pre-move guard that could still overshoot by up to 2 px).
11. Build corner coordinates for player, health, fire, enemy.
12. Health collision: grow by `HEALTH_GROWTH`, clamp position (B8), mark item eaten, increment `health_counter`, maybe show fire. Fire collision: shrink by `FIRE_PENALTY`, hide fire. Enemy collision: `state = GameState::GameOver`.
13. Draw score text (`score_time` accumulated this frame via `deltatime`, builds a `std::string`, opens the font file again).
14. `size < 3` sets `state = GameState::GameOver`. If GameOver, draw the Game Over image. If also Space: call the shared `ResetGame()` (fixed B3; was a partial inline reset).
15. `size >= 600` sets `state = GameState::Victory`. Same pattern for the Win image and restart, also calling `ResetGame()` (fixed B3; the restart block is no longer a separate copy-paste of the Game Over one).
16. `SDL_RenderPresent`. No manual delay (Step 3; the old `SDL_Delay(fps - deltatime)` limiter and its oscillating-pacing problem are gone — the renderer's `SDL_RENDERER_PRESENTVSYNC` flag now blocks `SDL_RenderPresent` until the next display refresh instead).

Rendering, input, logic and collision are interleaved. The player is drawn first, so items and enemy are drawn over it.

## 8. Known bugs (verified against the code)

Severity: **High** crash or undefined behavior or broken core loop, **Medium** wrong behavior, **Low** cosmetic or latent. "Review #" refers to `SDL_BUGS_AND_IMPROVEMENTS.md`.

### High

**B1. Double destroy of a texture on restart — FIXED**
Removed `SDL_DestroyTexture(C_Texture)` from both the Game Over and Win
restart blocks in `game.cpp`. `C_Texture` is an alias of `A_Texture`/
`B_Texture`, never an owner, so destroying A and B is sufficient.


**B2. Division by zero and absurd growth from `log(distance)` — FIXED**
Replaced both `size += sizeHealth * 3 / log(fairness_health)` and
`size -= sizeFire * 2 / log(fairness_fire)` with flat constants,
`size += HEALTH_GROWTH` / `size -= FIRE_PENALTY` (`HEALTH_GROWTH =
FIRE_PENALTY = 40.0`, confirmed with the owner). No more `log()` in
either path, so no distance-zero/distance-one div-by-zero or inf/-inf,
and no more "closer gives a wildly bigger reward" effect. This also
answers open question 1 in section 11. `fairness_health`/`fairness_fire`
are still computed at spawn but now dead code (not removed yet, see
section 11 cleanup note).

**B3. Restart does not reset the whole state — FIXED**
Added a single `ResetGame()` (takes every texture and game-state
variable by reference) called from both the Game Over and Win restart
sites, replacing the two near-identical ~25 line inline blocks. It now
resets everything the old blocks missed: `health_counter`,
`fire_visable`, `counter_enemy_direction`, `enemy_direction`, and
`state` (both endings at once, see B10). Fixes the concrete case: dying
while a fire was visible no longer leaves a fire on screen at the start
of the next round. `Random_Health_i/j`/`Random_Fire_i/j` are not
explicitly reset inside `ResetGame()` — setting `sizeHealth`/`sizeFire`
to `-1` makes the existing per-frame spawn logic (B6) pick fresh,
validated positions on the next frame, same as before. `score_time` is
reset to `0.0` here too (Step 3 rename, see B13).

**B4. Unchecked failures that can crash — FIXED**
`SDL__CreateWindow`'s return value is now checked in `main` (note: it
returns 0 on success, 1 on failure — opposite of a naive truthy check,
confirmed against the actual source). All nine `IMG_LoadTexture` calls
(startup and both restart blocks) now go through a `LoadTextureChecked`
helper that logs failures via `SDL_Log`. Note: a failed load still
renders nothing silently downstream (SDL no-ops on a null texture) —
this fix makes failures visible in the log, it does not make the game
refuse to run with missing assets. That remains a possible future
improvement, not done here.

### Medium

**B5. Wrong SDL_image initialization — FIXED**
`SDL__CreateWindow` in `SDL.h` now calls `SDL_Init`, `IMG_Init`, and
`TTF_Init` as three separate checked calls instead of OR'ing
`IMG_INIT_PNG | IMG_INIT_JPG` into `SDL_Init`'s flags. Also added
`IMG_Quit`/`TTF_Quit` to the existing window and renderer failure
cleanup paths, which previously only called `SDL_Quit`.

**B6. Spawn checks do not check overlap — FIXED**
Added a reusable `bool Overlaps(const SDL_Rect&, const SDL_Rect&)` and
rewrote both spawn loops to build a real candidate rect and test full
overlap with it, instead of testing only the new item's top-left
corner. Also fixed along the way: the enemy rect in both loops now
uses its actual 50x50 size instead of `Enemy_i + size` (the player's
size); the fire loop's health check now tests the health item's real
current rect (only when `sizeHealth > 0`) instead of the old backwards,
always-`sizeFire`-sized (0 or -1) check that was effectively always
false. Not changed: the fire loop still re-rolls every frame while the
fire is hidden (that's a separate, not-yet-addressed inefficiency, not
part of B6's overlap-correctness scope).

**B7. Gameplay speed depends on frame rate — FIXED**
Player movement, enemy movement, the enemy direction timer, and the
frame limiter are now all time based instead of per-frame (Step 3):

- `Character_i`/`Character_j` changed from `int` to `double`; movement
  uses `PLAYER_SPEED = 180.0` px/sec scaled by `deltatime` instead of a
  fixed 3px step. `PLAYER_SPEED` was deliberately calibrated to match
  the old 60fps feel (3px/frame x 60), not the owner's 144Hz monitor's
  old (bugged, inflated) feel — confirmed with the owner that the
  resulting "slower" player movement at 144Hz is correct, since the old
  speed scaling with refresh rate was the bug itself.
- `Enemy_i`/`Enemy_j` changed from `int` to `double`; movement uses
  `ENEMY_SPEED = 60.0` px/sec via `deltatime` instead of a fixed 1px
  step. `counter_enemy_direction` changed from a frame-count `int` to a
  seconds-elapsed `double`, compared against `ENEMY_DIRECTION_INTERVAL
  = 70.0 / 60.0` seconds instead of `% 70 == 0`.
- The old manual frame limiter (`fps = 1000 / refresh_rate` with a
  `fps = 16` fallback, then `SDL_Delay(fps - deltatime)`) is removed
  entirely from `game.cpp`, along with the now-unused
  `SDL_DisplayMode current` and `SDL_GetCurrentDisplayMode` call. That
  limiter subtracted the *previous* frame's total time (which already
  included the previous delay) from the target frame time, which made
  pacing oscillate rather than settle. `SDL_CreateRenderer` in `SDL.h`
  now passes `SDL_RENDERER_PRESENTVSYNC`, so the display handles
  pacing instead.
- Needed two narrowing-conversion fixes not obvious up front: the
  `SDL_Rect{ ... }` brace-init literals inside the `Overlaps()`
  while-conditions of both spawn loops needed explicit `(int)` casts
  on `Character_i`/`Character_j`/`Enemy_i`/`Enemy_j` — brace-init
  forbids narrowing outright (a hard compiler error), unlike the many
  other `double`-to-`int` spots elsewhere in the file that only warn.

**B8. Player can leave the window — FIXED**
Removed the pre-move guards (`Character_j >= 0`, `Character_i <
window_size - size`, etc.) and instead clamp `Character_i`/`Character_j`
to `[0, window_size - size]` after all movement is applied, every
`Playing` frame. Also added a clamp right after `size += HEALTH_GROWTH`
in the health-collision block, since growth near an edge could
otherwise leave the player partly outside the window until the next
frame's movement clamp ran. Shrinking (fire penalty) needs no clamp —
it only loosens the valid range.

**B9. Enemy behavior problems** (review improvement B, confirmed, not yet fixed)
Speed and the direction timer are now time based (Step 3, see B7), so
the frame-rate-dependence part of this is resolved. Still open: it
stops completely when it hits a wall (the movement guard fails) and
stays stuck until the next direction roll; it keeps moving during Game
Over and Win (the enemy-movement block still isn't guarded by `state`,
see section 7 step 7). Its direction and counter are reset on restart
(`ResetGame()`, B3). Planned for Step 4 or a dedicated pass — not part
of the Step 3 timing work.

**B10. Game state is two independent booleans — FIXED**
Added `enum class GameState { Playing, GameOver, Victory }` and
replaced every `gameover`/`winner` bool (local declarations, all
`!gameover && !winner` guards, both assignment sites, both `if
(gameover)`/`if (winner)` reads, both restart conditions, and
`ResetGame()`'s signature) with a single `state` variable. The
contradictory-state case is now structurally impossible rather than
just unobserved. Note: implemented without a `Paused` value — pause
input (`P` key) still exists in `SDL.h` but `game.cpp` still never
reads it, so there's no pause feature yet for a `Paused` state to
represent (see section 3, section 11 open question 3).

**B11. Rendering and logic are interleaved, with redundant draws** (review #12, extended)
The player is drawn before it is moved or resized. Each pressed arrow draws the player again at the old position (up to 5 draws per frame), which doubles alpha edges and is pointless. Sprite choice lags one frame. Do input, update, collide, render, present as separate stages. Not yet addressed; planned for Step 4.

**B12. Spawn loop can in theory never end** (review #11, more reachable now that B6 is fixed)
Previously could not practically happen, because only the top-left point was tested. Now that B6 uses real overlap tests, a large player (up to 599 px in a 700 px window) leaves less and less free space, so this becomes more reachable than before, though still not hit in practice at current sizes. Add an attempt limit and a fallback. Not yet done.

**B13. Score is not time based — FIXED**
Renamed `score_counter` (an `int` incremented once per frame, displayed
as `score_counter / 60`, which assumed exactly 60fps) to `score_time`
(a `double`, seconds elapsed while `Playing`, accumulated via
`deltatime` each frame). Displayed as `(int)score_time`. No longer
assumes any particular frame rate, consistent with the Step 3 timing
work in B7. It still opens the font file every frame (see helper
issues, section 9) — that part is unchanged. There is also an unused
shadowed `const char* score = 0;` near the top of `main`, left in
place.

**B14. Narrowing conversions.** `SDL_Rect rect = { Character_i, Character_j, size, size };` initializes `int` fields from `double` values. Expect compiler warnings, and a hard error under stricter settings. (The Step 3 `Overlaps()` brace-init spots, see B7, are the hard-error case of this; the rest are still warnings only.)

**B15. Uninitialized variables.** `Random_Health_i/j` and `fairness_health/fire` start uninitialized. They are written before first use in practice (health spawns in frame 1), but this is fragile. `fairness_health/fire` are additionally dead code since B2 — still worth initializing properly if kept, or removing (see section 11).

### Corrections to the earlier review

- **Review #8 ("player is immune to the enemy when large") is not a real bug.** Immunity only starts at `size >= 600`, and that is exactly when `state` becomes `GameState::Victory` in the same frame (was: `winner` becomes `true`, before B10). There is nothing to protect against. The state machine itself (B10) is now fixed.
- **Review #2** is accurate but overstated as "high" (see B5).
- **Review #6** is now fully addressed (see B7) — player speed, enemy speed, and the direction timer are all time based, and the frame limiter itself was also fixed as part of the same pass.
- **Review #10 (image leak)** is real, but `SDL__DrawImage` is never called by the game, so it does not affect gameplay.
- The review misses B3's concrete symptom, the every frame fire respawn, the enemy size error, the ignored init result, the Release|x64 subsystem problem, and the wrong thick line math below.

## 9. Helper library issues (`SDL.h`)

- **Globals and bodies in a header.** `window`, `renderer`, `lastRun`, `event` and all functions are defined in the header. Move declarations to a header and definitions to a `.cpp`, or wrap everything in a small class that owns window and renderer. Rename the file (for example `SDLHelpers.hpp`) so it stops sharing a name with SDL's own `SDL.h`.
- **`SDL__DrawText`**: opens and closes `consola.ttf` on every call (every frame for the score). If `TTF_RenderUTF8_Solid` fails it returns without `TTF_CloseFont` (leak). `SDL_CreateTextureFromSurface` is not checked. Load the font once and cache it.
- **`SDL__DrawImage`**: never destroys the texture it creates (leak per call), does not check texture creation, queries a size it never uses.
- **`SDL__SetColor`**: checks only `strlen == 7`, not the leading `#` or that the digits are hex. Also changes the renderer draw color as a side effect, so after drawing text the draw color is red (harmless only because the background image covers the clear color).
- **`SDL__DrawLine`**: divides by the line length, which is 0 for a zero length line (NaN cast to int is undefined). The thickness offset `(i*dx, -i*dy)` is applied **along** the line instead of perpendicular to it (it should use `(-dy, dx)`), so for a horizontal line the extra lines overlap and thickness does nothing.
- **`SDL__DrawRect`**: `w - 2*i` can go negative for a large `width`.
- **`SDL__DrawCircle` / `FillCircle`**: per pixel loops over a bounding square, slow for large radii.
- **`SDL__Present`**: uses `clock()` (CPU time) for pacing, and the game does not call it anyway (it's dead code — `game.cpp` calls `SDL_RenderPresent` directly and no longer delays at all, see B7/Step 3).
- **`SDL__CreateWindow`**: seeds `srand` (so does `main`), unconditionally does the init work again if called twice. Cleanup on failure paths and `IMG`/`TTF` init are now handled (B4, B5).

Since only four helper functions are actually used, the simplest cleanup is to delete the dead drawing helpers or move them to a separate file, rather than fix code nobody calls.

## 10. Build, repo and portfolio issues

- **Binaries in the repo root** (`.lib`, `.dll`) and committed build output in `x64/Debug`. Replace with a documented dependency setup (vcpkg or the official SDL2 development packages), record SDL2, SDL2_image and SDL2_ttf versions, and make sure the architecture matches the target.
- **`.gitignore`** should cover `Debug/`, `Release/`, `x64/`, `.vs/`. Inspect `x64/Debug` first before ignoring it, in case something needed is only there.
- **Assets are duplicated and scattered.** The game reads the root copies, there is also an `assets/` folder. Choose one folder, add a path prefix in the code, and set the Visual Studio debugging working directory so the game runs from the IDE and from a double click.
- **Case mismatch** `win.png` (code) vs `Win.png` (file). Fine on Windows, breaks elsewhere. Still unresolved.
- **`consola.ttf` is Microsoft's Consolas font.** Redistributing it in a public repo is a licensing risk. Replace it with an open licensed font (for example DejaVu Sans Mono or Liberation Mono) and credit it.
- **Sprites.** I have not seen the images. Before publishing, look at every image and its filename (`Hestler.png`, `BacheGherti.png`, `Bozorg.png`, and so on), and rename or replace anything that is not suitable for a public portfolio. Also confirm you have the right to use each image.
- **No README, no license, no description, no topics, no screenshots or GIF.** All of these matter for a portfolio piece.
- **Toolset `v145`** needs a recent Visual Studio. Document it, or retarget to `v143`.
- Optional later: add a `CMakeLists.txt` so it also builds on Linux/macOS.

## 11. Open questions for the owner

Ask before changing these, they are design decisions, not bugs:
1. ~~What was `fairness` (distance based growth) meant to do?~~ **Answered (B2):** dropped distance from the formula entirely, flat `40.0` px growth/shrink per pickup, confirmed with the owner. Follow-up, still open: delete the now-dead `fairness_health`/`fairness_fire` variables and their spawn-time `sqrt()` computation, or leave them in place for a possible future distance-based feature?
2. Should the enemy be able to kill a big player? (Currently irrelevant because 600 means win.)
3. What were the unused images (`speedBooster`, `Pause`, `*_Fast`, `Bozorg`, `Bache`) for? Finish them, or delete them? (Relatedly: is a pause feature wanted? `GameState` has no `Paused` value yet, see B10.)
4. Keep Visual Studio only, or also support CMake?
5. Which license for the code, and which font to ship?
6. ~~Is `PLAYER_SPEED` at `180.0` (matching old 60fps feel) the right call speed, now that it's refresh-rate independent?~~ **Answered:** confirmed with the owner to keep `180.0` as-is rather than scaling it up to match the old (bugged) 144Hz feel.

## 12. Fix plan (in this order)

**Step 1, stop the crashes and leaks (B1, B4, B5, helper text/image leaks). — DONE**
Single texture ownership, load once. Check every init and load call, print a clear message, exit cleanly. Proper `IMG_Init`.

**Step 2, game correctness (B2, B3, B6, B8, B10). — DONE**
Added `Overlaps()`. Added `ResetGame()` and a `GameState` enum. Replaced the log formula with a flat-constant one. Clamped positions and sizes. Fixed spawn logic (real overlap tests).

**Step 3, timing (B7, B9 partial, B13). — DONE**
Switched to `SDL_RENDERER_PRESENTVSYNC` and removed the manual frame limiter. Converted player movement, enemy movement, and the enemy direction timer to `deltatime`-based (seconds), replacing the old per-frame constants. Converted score from a frame-count assumption to real elapsed time (`score_time`). B9's remaining issues (enemy stuck on walls, enemy moving during an ending) are not timing problems and are left for a later pass.

**Step 4, structure.**
Split into `Game`, `Player`, `Enemy`, `Collision`, `Renderer` files, no mutable globals, `SDLHelpers.hpp/.cpp` or a small renderer class. Also a reasonable place to finish B9 (wall-stuck enemy, state-guarded enemy movement) and B11 (separate input/update/collide/render/present stages), since both involve restructuring the same loop.

**Step 5, repo polish.**
README with screenshot and controls, license, `.gitignore`, assets folder, dependency notes, font replacement, remove committed binaries and build output.

### Tests worth writing (logic only, no SDL window)

- `Overlaps()`: overlapping, touching edges, separate, contained.
- Growth: `HEALTH_GROWTH`/`FIRE_PENALTY` applied flat, size stays inside min and max (no longer distance based, see B2).
- Clamp: player at all four edges and after growth.
- Reset after losing and after winning: every variable equals its initial value (including `fire_visable`, `health_counter`, `score_time`, enemy direction).
- Spawn: result never overlaps player, enemy or the other item, and terminates when no valid spot exists.
- Movement and timers give the same real-time result regardless of frame rate (Step 3: now true by construction via `deltatime`, worth a regression test against 30/60/144fps worth of simulated steps).
- Missing asset: clear error, no crash.

## 13. Working rules for future sessions

- Keep SDL2 and C++. Do not switch libraries or add an engine.
- Smallest change that fixes the problem, one logical fix per commit, message says which bug ID from this file it addresses.
- When a bug is fixed, update section 8 here (mark it fixed with the commit) so this file stays true.
- Compile and run after every change. If I cannot run it, say so instead of claiming it works.
- Do not delete assets or dead code without the owner's answer to the open questions above.
- Keep the owner's style where reasonable (names like `Character_i`, section comment headers) unless that is part of what is being refactored.
- Branch naming used so far: narrow, scoped-to-the-actual-commit names (for example `step3-enemy-timing` for the enemy speed/timer sub-piece of Step 3), rather than one branch per whole Step.
