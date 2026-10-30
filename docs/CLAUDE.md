# CLAUDE.md: Square Baby (SDL2, C++)

Reference file for working on `TheRealMoeid/SDL`. Read this before touching code. It describes what the project is, how it works, every problem found by reading the source, and the order fixes have been (and are being) made in.

## 0. How to use this file

- The owner built this as a final semester presentation project about two years ago and wrote no documentation at the time. It has since been cleaned up for a portfolio across Steps 1-5 below, all of which are now complete.
- Goal: a stable, readable, correctly behaving game with a proper README. Not a rewrite, not an engine, not a new library.
- This revision was checked directly against the actual current source of every file in the repo (all `.hpp`/`.cpp` files, `sdl.vcxproj`, `sdl.sln`, `CMakeLists.txt`, `.gitignore`, `CppProperties.json`, `README.md`, `LICENSE`). The previous revision of this file had drifted out of sync with the code on several points (see "Corrections to this file" in section 8) - this revision corrects that.
- **The live, session-by-session execution plan is `NEXT_STEPS.md`**, not this file's old section 12. This file still records architecture and bug status; `NEXT_STEPS.md` is what to actually work from.
- Fix one bug per commit. Keep gameplay feel the same unless the owner approves a design change (see section 11, open questions).

## 1. What the project is

A small 2D survival game called **Square Baby** (window title). The player is a square sprite that continuously shrinks. Collecting health items makes it grow, fire items make it shrink, and a wandering enemy ends the run on contact. Grow to **500 px** to win. Shrink below 3 px or touch the enemy (while under 500 px) to lose. Space restarts after either ending.

- Language: C++17
- Libraries: SDL2, SDL2_image (PNG textures), SDL2_ttf (score text)
- Platform: Visual Studio (MSVC) project, plus a working CMake build for Linux/macOS/Windows
- Size: gameplay is now split across nine source files (`Player`, `Enemy`, `Health`, `Fire`, `Collision`, `GameLogic`, `Renderer`, `SDLHelpers`, `game.cpp`) instead of one monolithic `main()` - see section 3 and 5. `game.cpp` is now just `main()`, under 50 lines.
- Repo state: README, MIT `LICENSE`, `.gitignore`, `CMakeLists.txt`, and `NEXT_STEPS.md` all present. Still no automated tests or CI.

## 2. What's not yet verified from source alone

- The contents of the actual image files (`assets/images/*.png`) - filenames only, not what they show or whether rights to use them are confirmed. Flagged as an open item before treating the repo as portfolio-final.
- Whether the Step 5 decision to delete the unused images (`Bache.png`, `BacheGherti_Fast.png`, `Hestler_Fast.png`, `Bozorg.png`, `Pause.png`, `speedBooster.png`) was actually executed - worth a quick directory check, since image files aren't visible in a source-only review.
- Whether the bundled vendored `SDL2*.lib`/`.dll` files (still referenced by the non-vcpkg build path) are 32- or 64-bit and which SDL versions they are - relevant to the pending vcpkg migration (see section 10).

## 3. Repository layout (current)

```
SDL/
  game.cpp                 thin main(): init, load, loop, shutdown - no gameplay logic
  GameLogic.hpp/.cpp        GameState enum, GameWorld struct, Game_LoadTextures,
                             Game_Reset, Game_RunFrame (update + collision stage)
  Player.hpp/.cpp           PlayerState, sprite selection, shrink, movement, clamping
  Enemy.hpp/.cpp            EnemyState, open-direction-picking movement (B18)
  Health.hpp/.cpp           HealthState, spawn + shrink
  Fire.hpp/.cpp             FireState, spawn + shrink
  Collision.hpp/.cpp        Overlaps() and the three CollidedWith*() checks
  Renderer.hpp/.cpp         Render_Draw() - all SDL_RenderCopy calls, gated by GameState
  SDLHelpers.hpp/.cpp       GameContext, Events, SDL__* wrapper functions
  GameConstants.hpp         tunable constants (speeds, growth/penalty, asset dirs)
  sdl.sln / sdl.vcxproj     Visual Studio solution and project (+ .filters)
  CMakeLists.txt            cross-platform build via pkg-config
  CppProperties.json        IntelliSense config (x64-Debug only)
  assets/
    images/                 BacheGherti.png Hestler.png Health.png Fire.png Enemy.png
                             background.png GameOver.png Win.png (consolidated here
                             in Step 5 - the game no longer reads root-level copies)
    fonts/                  DejaVuSansMono.ttf (replaced the proprietary Consolas font)
  SDL2.lib SDL2.dll SDL2_image.lib SDL2_image.dll
  SDL2_ttf.lib SDL2_ttf.dll SDL2main.lib    vendored binaries, still in repo root -
                                             vcpkg migration is incomplete, see section 10
  sdl/                      real SDL2 headers, included by SDLHelpers.hpp as <SDL2/SDL.h>
  README.md, LICENSE, .gitignore, NEXT_STEPS.md
```

The old root-level `SDL.h` naming trap (a project-authored header sharing a name with the real SDL header) is gone - it was split and renamed to `SDLHelpers.hpp`/`.cpp` as part of Step 4.

## 4. Build and run

- Open `sdl.sln` in Visual Studio. Configurations: Debug/Release for Win32 and x64.
- `PlatformToolset` is `v145` in all four configurations - confirm it's installed, or retarget.
- `SDL_MAIN_HANDLED` is defined in `SDLHelpers.hpp`, so plain `int main()` is used and `SDL2main.lib` is not needed.
- **Build is inconsistent across configurations right now:** `Debug|x64` has `AdditionalDependencies` pointing at a hardcoded `C:\vcpkg\installed\x64-windows` path plus a post-build step that copies the vcpkg DLLs next to the exe. `Release|x64` has neither - a clean-checkout Release build is not currently expected to work out of the box. This is tracked as a dedicated session in `NEXT_STEPS.md` (vcpkg manifest + fixing Release).
- The renderer uses `SDL_RENDERER_PRESENTVSYNC` (set in `SDLHelpers.cpp`'s `SDL__CreateWindow`), so frame pacing follows the display's actual refresh rate. The old manual `SDL_Delay`-based limiter is gone.
- Asset paths are no longer flat/relative-to-cwd only: `GameConstants.hpp` defines `ASSET_IMAGES_DIR "assets/images/"` and `ASSET_FONTS_DIR "assets/fonts/"`, used to build every load path. The working directory still needs to be the project root (or wherever `assets/` sits) when running the exe directly.
- The CMake build (`cmake -B build && cmake --build build`) copies `assets/` next to the built executable automatically and depends on SDL2/SDL2_image/SDL2_ttf being discoverable via pkg-config.

## 5. Architecture

### 5.1 `SDLHelpers.hpp` / `SDLHelpers.cpp`

Replaces the old `SDL.h`. `GameContext` (window, renderer, `Events`, `lastRun`) is now passed by reference into every `SDL__*` function instead of living as module-level globals - this was Step 4's Commit B. `Events` is unchanged (`exit, left, right, up, down, space, pause`, level-triggered).

| Function | Used by the game? | Notes |
|---|---|---|
| `SDL__CreateWindow(ctx, title, w, h)` | yes | Separate checked `SDL_Init`/`IMG_Init`/`TTF_Init`, accelerated + vsync'd renderer, clears to black. Returns 0 on success, 1 on failure. |
| `SDL__Events(ctx)` | yes | Arrows, Space, P, window close. |
| `SDL__DrawText(ctx, text, x, y, size, "#RRGGBB")` | yes | Still opens `DejaVuSansMono.ttf` on every call - unfixed overhead, see section 8 (B22). |
| `SDL__Destroy(ctx)` | yes | Destroys renderer/window, `TTF_Quit`, `SDL_Quit`. Does **not** call `IMG_Quit` - see B23. |
| `SDL__SetColor(ctx, "#RRGGBB")` | indirectly | Parses hex, also sets the renderer draw color (side effect). Only validates string length, not the `#` prefix or hex digits - see B24. |
| `SDL__DrawPoint / DrawLine / DrawRect / FillRect / DrawCircle / FillCircle / Clear / DrawImage / Present` | no | Still dead code for this game - `game.cpp` calls `SDL_RenderPresent` directly and never calls any of these. |

### 5.2 `Player.hpp` / `.cpp`

`PlayerState { size, x, y, small_tex, big_tex, current_tex }`. `current_tex` is an alias into `small_tex`/`big_tex`, never an owner (this is what B1 fixed). `Player_UpdateSprite` picks the sprite for the *current* frame before drawing (B11 fix - no more one-frame lag). `Player_Shrink` is the same log-based decay as before. Player_HandleMovement builds a normalized direction vector before scaling by PLAYER_SPEED and deltatime (B19 fix) - diagonal and axis-aligned movement are now the same speed.

### 5.3 `Enemy.hpp` / `.cpp`

`EnemyState { x, y, direction, direction_timer, texture }`. `Enemy_Update` is now meaningfully smarter than the original frame-count-based wander:

- `DirectionIsOpen()` checks wall bounds and item collision for a given direction in one call.
- `EnemyBlockedAt()`/`EnemyTryMove()` treat the health item as solid with a soft `ENEMY_HEALTH_AVOID_MARGIN` buffer, falling back to the item's real footprint only (no margin) if the margin alone would block every direction - so the enemy can never get permanently stuck waiting for a health item to disappear.
- `PickOpenDirection()` only re-evaluates when the direction timer elapses or the current direction is actually blocked, instead of rerolling every frame - this is **B18**, merged as PR #25 on 2026-10-29. It directly resolves the old B9 ("stops at walls, stays stuck") - B9 is now marked fixed below.
- Enemy movement is gated to `GameState::Playing` in `GameLogic.cpp`'s `Game_RunFrame`, so it correctly freezes during Game Over/Victory - this is the other half of the old B9.

### 5.4 `Health.hpp`/`.cpp` and `Fire.hpp`/`.cpp`

`HealthState { x, y, size, texture }`, `FireState { x, y, size, visible, texture }` - two separate structs, not shared. 
Both now use the shared FindSpawnPosition() helper in Collision.hpp/.cpp (100-attempt cap, B12 fix). Fire_SpawnIfNeeded only picks a new candidate when it doesn’t already hold one (size > 0 guard), instead of every frame while hidden (B20 fix). (the inline code comment in `Fire.cpp` loosely references "B12" for this, 
but B12 is specifically the unbounded-loop issue; 
the two are related but not the same fix).

### 5.5 `Collision.hpp`/`.cpp`

`Overlaps(a, b)` - the shared rect-overlap test (B6). `CollidedWithHealth/Fire/Enemy` build the player's rect against each item's current rect and reuse `Overlaps()`. The enemy rect is consistently built as a real 50x50 everywhere now, not the player's size (the old enemy-size bug under B6 is fully resolved).

### 5.6 `GameLogic.hpp`/`.cpp`

`GameState { Playing, GameOver, Victory }` (B10). `GameWorld` bundles `PlayerState`, `EnemyState`, `HealthState`, `FireState`, `state`, `score_time`, `health_counter`, `window_size`, and the three non-player/item textures (`background_tex`, `gameover_tex`, `win_tex`).

- `Game_LoadTextures` - loads all nine textures. Called once at startup **and** from `Game_Reset` on every restart.
- `Game_Reset` currently destroys and reloads every texture on every restart, then resets round state - the texture reload is unnecessary (textures don't need to be invalidated between rounds) and is a real, still-open issue: **B21**.
- `Game_RunFrame` - runs update + collision each frame: player shrink (if `Playing`), sprite pick, enemy update (if `Playing`), health spawn/shrink, fire spawn/shrink (not gated by `Playing` - unchanged from the original), player movement (if `Playing`), the three collision checks, score accumulation, the size<3 and size>=500 win/lose checks, and the restart-on-space check. This is the B11 fix - update, collide, and the restart decision all happen before `Render_Draw` is called, so the first frame after a reset renders correctly instead of one frame stale.

### 5.7 `Renderer.hpp`/`.cpp`

`Render_Draw(ctx, world)` - the only place that calls `SDL_RenderCopy`. Entirely state-gated: player/enemy/health/fire/score draw only in `Playing`, the Game Over and Win overlays draw only in their respective states. No game logic lives here.

### 5.8 `game.cpp`

Just `main()`: create window, load textures, set initial player position, then a loop of events -> compute `deltatime` -> `Game_RunFrame` -> `Render_Draw` -> `SDL_RenderPresent`. Under 50 lines, no gameplay logic.

## 6. Game rules and constants

| Thing | Value |
|---|---|
| Window | 700x700 |
| Player start | size 50, centered |
| Player speed | `PLAYER_SPEED = 180.0` px/sec, time based, **not normalized diagonally** (see B19) |
| Player shrink | `size -= 8.3 * ln(size) * 0.3 * dt/1000` per frame (unchanged, time based) |
| Sprite swap | `size > 100` uses `Hestler.png` (big), else `BacheGherti.png` (small) |
| **Win** | `size >= window_size - 200` = **500** (changed from the original 600 during Step 5) |
| **Lose** | `size < 3`, or touching the enemy while `size < window_size - 200` (500) |
| Immunity guards | health growth, fire penalty, and enemy-collision-ends-game are all gated on `player.size < window_size - 200`, consistent with the win threshold |
| Enemy | 50x50, starts at (100, 100), `ENEMY_SPEED = 60.0` px/sec, picks a new direction only when blocked or `ENEMY_DIRECTION_INTERVAL = 70/60` sec elapses (B18) |
| Health item | 50 px at spawn, shrinks 10 px/s, respawns via `Health_SpawnIfNeeded` (still unbounded retry loop, B12) |
| Health growth | flat `HEALTH_GROWTH = 40.0` px per item (B2) |
| Fire item | appears after every 3 health items eaten, 50 px, shrinks 10 px/s |
| Fire penalty | flat `FIRE_PENALTY = 40.0` px per item (B2) |
| `fairness_*` | **removed entirely** - the dead `sqrt`-distance computation flagged in the previous revision of this file no longer exists in `Health.cpp`/`Fire.cpp` |
| Score | `score_time`, a `double` accumulated via `deltatime` while `Playing` (B13), displayed as `(int)score_time` |
| Controls | Arrow keys move, Space restarts, closing the window quits. `P` is read but unused (no pause feature yet). |

## 7. Frame loop, as it actually runs today

1. `SDL__Events(ctx)`, break on exit.
2. Compute `deltatime` from `SDL_GetTicks()`.
3. `Game_RunFrame(ctx, world, event, deltatime)`:
   - Shrink player (if `Playing`).
   - `Player_UpdateSprite` (B11 - before drawing, not after).
   - `Enemy_Update` (if `Playing`).
   - `Health_SpawnIfNeeded`, then `Health_Shrink` (if `Playing`).
   - `Fire_SpawnIfNeeded` (not gated - still re-rolls every frame while hidden, B20), then `Fire_Shrink` (also not gated, matches original behavior).
   - `Player_HandleMovement` (if `Playing`), which clamps internally.
   - Collision checks: health (grow + maybe reveal fire), fire (shrink + hide), enemy (-> `GameOver`).
   - Score accumulation (if `Playing`).
   - Size<3 -> `GameOver`; size>=500 -> `Victory`.
   - If ended and Space pressed: `Game_Reset` (currently also reloads all textures, B21).
4. `Render_Draw(ctx, world)` - all drawing, fully state-gated.
5. `SDL_RenderPresent(ctx.renderer)` - vsync paces this, no manual delay.

Rendering is now cleanly separated from update/collision (B11 fully resolved) - this was previously the single biggest structural issue in the original monolithic `game.cpp`.

## 8. Known bugs

Severity: **High** crash or undefined behavior or broken core loop, **Medium** wrong behavior, **Low** cosmetic or latent, **Open** not yet fixed.

### Fixed (unchanged from the previous revision - still true)

B1 (double texture destroy), B2 (log-formula div-by-zero/absurd growth), B3 (incomplete restart), B4 (unchecked init/load failures), B5 (wrong SDL_image init), B6 (spawn overlap checks), B7 (frame-rate-dependent speed/timing), B8 (player could leave the window), B10 (two independent booleans instead of a `GameState` enum), B13 (frame-count-based score).

### Fixed since the previous revision of this file

**B9. Enemy behavior problems - FIXED.** Previously open ("stops at walls, stays stuck until next direction roll; keeps moving during Game Over/Win"). Both halves are now resolved: `DirectionIsOpen`/`PickOpenDirection` (B18) mean it only commits to directions that are actually open, and `Game_RunFrame` gates `Enemy_Update` on `GameState::Playing`.

**B11. Rendering and logic interleaved - FIXED.** Previously open. `GameLogic.cpp`'s `Game_RunFrame` (update + collide) and `Renderer.cpp`'s `Render_Draw` (render) are now separate calls from `game.cpp`, in that order, every frame.

**B15. Uninitialized variables - FIXED / moot.** `HealthState`/`FireState` now use default member initializers (`size = -1`, etc.), and `fairness_health`/`fairness_fire` no longer exist at all (removed, not just dead) - both halves of the original concern are gone.

**B18 (new). Enemy picks from genuinely open directions instead of blind reroll.** `DirectionIsOpen`/`PickOpenDirection`/`EnemyTryMove`/`EnemyBlockedAt` in `Enemy.cpp`. Merged as PR #25, 2026-10-29. This is what fixed B9.

Also fixed, not previously tracked as a numbered bug: the `win.png` (code) vs `Win.png` (file) case mismatch flagged in the previous revision's section 10 is resolved - `GameLogic.cpp` now loads `ASSET_IMAGES_DIR "Win.png"`, matching the actual filename.

### Still open

**B12. Spawn loop can in theory never end - FIXED.** Health_SpawnIfNeeded
and Fire_SpawnIfNeeded now use the shared FindSpawnPosition() helper
(Collision.hpp/.cpp), capped at 100 random attempts. On failure the item
just stays unspawned (size <= 0) and retries next frame instead of
looping.

**B14. Narrowing conversions.** Largely addressed in practice - every `SDL_Rect` construction across `Collision.cpp`, `Health.cpp`, `Fire.cpp`, `Renderer.cpp` now uses explicit `(int)` casts rather than relying on implicit narrowing. Not removed as a category (the underlying `double`-position / `int`-rect mismatch is inherent to the design), but no longer an unguarded warning source.

**B19. Diagonal movement not normalized - FIXED.** Player_HandleMovement
now builds a direction vector and normalizes it before scaling by
PLAYER_SPEED, so diagonal speed matches axis speed (180 px/s in both
cases).

**B20. Fire re-rolls its candidate position every frame while hidden -
FIXED.** Fire_SpawnIfNeeded's guard changed from `size > 0 && visible` to
`size > 0` - a candidate is picked once and held until actually consumed,
instead of being overwritten every frame while invisible.

**B21 (new). `Game_Reset` destroys and reloads every texture on every restart,** instead of only resetting round state. Textures don't need to be invalidated between rounds. Scheduled in `NEXT_STEPS.md` Session 2.

**B22 (new). `SDL__DrawText` opens and closes the font on every call** (every frame, for the score). Not a leak on the success path, but real per-frame overhead; the failure path after `TTF_RenderUTF8_Solid` does skip `TTF_CloseFont` (a real leak on that specific failure). Scheduled in `NEXT_STEPS.md` Session 2.

**B23 (new). `IMG_Quit()` is never called** in `SDL__Destroy` - `SDL_image` initializes but doesn't cleanly tear down. Scheduled in `NEXT_STEPS.md` Session 2.

**B24 (new). `SDL__SetColor` only checks `strlen(color) == 7`,** not the leading `#` or that the remaining characters are valid hex digits. Scheduled in `NEXT_STEPS.md` Session 2.

### Corrections to this file (previous revision was stale on these points)

- Previously claimed Step 4 (file split) and Step 5 (repo polish) were still pending. Both are done - see section 3.
- Previously claimed B9 and B11 were open. Both are fixed - see above.
- Previously claimed win threshold was `size >= 600`. It's `size >= window_size - 200` = 500.
- Previously claimed `fairness_health`/`fairness_fire` were present as dead code. They've been removed entirely.
- Previously claimed the enemy's collision rect sometimes used the player's size. All current code consistently uses a real 50x50 enemy rect.
- Previously claimed `win.png`/`Win.png` was an unresolved case mismatch. It's resolved.

## 9. Helper library issues (`SDLHelpers.hpp`/`.cpp`)

Most of the original list here is now either fixed or superseded by section 8's bug tracking:

- **Globals in a header - FIXED.** `GameContext` now carries window/renderer/events/lastRun by reference through every `SDL__*` call (Step 4, Commit B). No more module-level globals, no more shadowing between a global `Events` and a local one.
- **Naming trap (`SDL.h` vs `sdl/SDL.h`) - FIXED.** Renamed to `SDLHelpers.hpp`/`.cpp` (Step 4, Commit A).
- **`SDL__DrawText` font reopen + failure-path leak** - still open, see B22.
- **`SDL__DrawImage`** - still dead code, never called; still leaks its texture if it were ever used. Low priority given it's unreachable.
- **`SDL__SetColor` validation** - still open, see B24.
- **`SDL__DrawLine` div-by-zero on zero length, wrong-axis thickness offset** - still present, still dead code (never called).
- **`SDL__DrawRect` negative width for large `width` param** - still present, still dead code.
- **`SDL__DrawCircle`/`FillCircle`** - still present, still dead code, still slow for large radii if ever used.
- **`SDL__Present`** - still dead code (`game.cpp` calls `SDL_RenderPresent` directly and no longer delays at all).
- **`SDL__CreateWindow`** - cleanup on failure paths and `IMG`/`TTF` init are handled (B4, B5, still correct).

Since none of the drawing helpers besides `DrawText` are actually called, the lowest-effort cleanup remains: delete them or move them to a clearly-marked unused file, rather than fixing bugs in code nobody calls. Tracked in `NEXT_STEPS.md` Session 2.

## 10. Build, repo and portfolio issues

- **vcpkg migration is incomplete, not absent.** `Debug|x64` references `C:\vcpkg\installed\x64-windows` (machine-specific, hardcoded) but there's no `vcpkg.json` manifest, and `Release|x64` isn't wired up the same way at all. Vendored `.lib`/`.dll` files are still in the repo root. A previous attempt at finishing this migration had to be reverted after an automated tool left the repo in a half-migrated, broken state - redo this carefully, on its own branch, confirming both Debug and Release build before removing any vendored binaries. Tracked in `NEXT_STEPS.md` Session 3.
- **`.gitignore`** already covers `Debug/`, `Release/`, `x64/`, `.vs/` - this is resolved, no longer an open item.
- **Assets** are consolidated into `assets/images/` and `assets/fonts/` (Step 5) - this is resolved.
- **Case mismatch (`win.png`/`Win.png`)** - resolved, see section 8.
- **Font licensing** - resolved, `DejaVuSansMono.ttf` replaced the proprietary Consolas font.
- **Sprites** - still not individually reviewed for portfolio suitability or confirmed usage rights. Still open, not code work.
- **README, license, `.gitignore`, description** - all present now. Still missing: real gameplay screenshots or a GIF. Tracked in `NEXT_STEPS.md` Session 4.
- **Toolset `v145`** - still requires a recent Visual Studio; still undocumented whether it's confirmed installed on the dev machine.
- **No automated tests, no CI** - still true, tracked in `NEXT_STEPS.md` Session 4 (tests) and as a post-session follow-up (CI).

## 11. Open questions for the owner

1. ~~What was `fairness` meant to do?~~ **Answered (B2):** dropped, flat `40.0` growth/shrink. The variables themselves are now also fully removed from the code (previously they were dead-but-present; that's no longer the case).
2. Should the enemy be able to kill a big player? (Currently irrelevant - 500 means win, same as before, just at a lower threshold.)
3. **Still open:** is a pause feature wanted? `GameState` has no `Paused` value; `P` is tracked in `Events` but never read in `GameLogic.cpp`. The Step 5 decision to delete the unused images (`speedBooster`, `Pause`, `*_Fast`, `Bozorg`, `Bache`) answers "finish or delete" for those specific assets, but doesn't by itself answer whether pause-as-a-feature is still wanted independent of the `Pause.png` asset.
4. ~~Keep Visual Studio only, or also support CMake?~~ **Answered:** both - CMake was added in Step 5 and is a working second build path.
5. ~~Which license for the code, and which font to ship?~~ **Answered:** MIT, DejaVu Sans Mono.
6. ~~Is `PLAYER_SPEED` at `180.0` the right call now that it's refresh-rate independent?~~ **Answered:** confirmed with the owner to keep `180.0` as-is.

## 12. Fix plan status

**Steps 1 through 5 (crash/leak fixes, correctness, timing, structure, repo polish) are all DONE.** See section 8 for the full bug-by-bug status and section 3 for the resulting file layout.

**For what comes next, use `NEXT_STEPS.md`** - it lays out four focused work sessions (correctness fixes + doc sync; SDL resource lifecycle; vcpkg/Release build; tests + portfolio polish), each with objectives, a task list, affected files, what to test, and completion criteria, plus a risk section and a "what after these sessions" list. This file (`CLAUDE.md`) should keep tracking architecture and bug status as that plan is executed; `NEXT_STEPS.md` is the thing to actually follow step by step.

## 13. Working rules for future sessions

- Keep SDL2 and C++. Do not switch libraries or add an engine.
- Smallest change that fixes the problem, one logical fix per commit, message says which bug ID from this file it addresses.
- When a bug is fixed, update section 8 here (mark it fixed with the commit) so this file stays true. **This file was previously allowed to drift out of sync with the actual code for several steps' worth of work - update it in the same session as the code change, not after.**
- Compile and run after every change. If it can't be run, say so instead of claiming it works.
- Do not delete assets or dead code without the owner's answer to the open questions above.
- Keep the owner's style where reasonable unless that's part of what's being refactored.
- Branch naming: narrow, scoped-to-the-actual-commit names (e.g. `step3-enemy-timing`), rather than one branch per whole Step.
