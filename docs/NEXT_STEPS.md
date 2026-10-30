# NEXT_STEPS.md: Square Baby — Development Roadmap

Based on a full review of source code (all `.hpp`/`.cpp` files), `CLAUDE.md`,
`README.md`, `SDL_Repository_Code_Review.md`, `sdl.vcxproj`/`sdl.sln`,
`CMakeLists.txt`, `.gitignore`, and `CppProperties.json` as they currently
stand in the repository. No source files were modified to produce this
report.

## 0. Important: `CLAUDE.md` is out of date

Before anything else: `CLAUDE.md` describes an earlier state of the project
and is no longer accurate on several points. The actual code is **ahead**
of what `CLAUDE.md` claims:

- It says Step 4 (file split) and Step 5 (repo polish) are future work.
  They are done — `Player/Enemy/Health/Fire/Collision/GameLogic/Renderer`
  are already split into their own files, `CMakeLists.txt` exists, MIT
  `LICENSE` exists, `consola.ttf` was already replaced with
  `DejaVuSansMono.ttf`.
- It says B9 (enemy gets stuck on walls) is "confirmed, not yet fixed."
  It's fixed — `Enemy.cpp` now has `DirectionIsOpen`/`PickOpenDirection`,
  which picks from genuinely open directions instead of rolling blind.
- It says B11 (rendering/input/logic interleaved) is not yet addressed.
  It's fixed — `GameLogic.cpp`/`Renderer.cpp`/`game.cpp` now run as
  distinct events → update → collide → render → present stages.
- It references win at `size >= 600` (window − 100). The actual code
  (`GameLogic.cpp`, `README.md`) uses `window_size - 200` = **500**.
- It says `fairness_health`/`fairness_fire` are still present as dead
  code. They're gone — `Health.cpp`/`Fire.cpp` no longer compute them at
  all.
- It says the enemy's box still uses the player's size instead of 50×50
  (B6 commentary). Current `Collision.cpp`/`Enemy.cpp` consistently use
  a real 50×50 enemy rect.

Session 1 below includes a documentation-sync task to fix this, because
every future session (and anyone else reading `CLAUDE.md` as a reference)
is currently working from a stale picture of the codebase.

---

## 1. Completed work (verified against current source)

- Steps 1–5 of the original fix plan: texture ownership, checked
  init/load, flat growth/shrink constants, `Overlaps()`-based spawn
  checks, centralized `GameState` enum + `Game_Reset`, position
  clamping, delta-time-based movement/enemy/score, vsync-based pacing,
  file split into per-system modules, MIT license, open-licensed font,
  `assets/images/` + `assets/fonts/` layout, CMake build path.
- Enemy AI: direction selection now checks all four directions for
  actual openness (wall, fire, health item with a soft avoidance
  margin that falls back to hard-footprint-only if the margin alone
  would block every direction) and only re-evaluates on timeout or an
  actually-blocked current direction. Enemy is also correctly gated to
  `GameState::Playing`, so it freezes on Game Over/Victory.
- Render/update separation: `Game_RunFrame` (update+collide) and
  `Render_Draw` (render) are distinct calls from `game.cpp`'s loop.

## 2. Remaining work

### Essential fixes (correctness bugs, still present in the code today)

1. **Diagonal movement isn't normalized** (`Player.cpp`,
   `Player_HandleMovement`). Each axis is scaled by `PLAYER_SPEED`
   independently, so moving diagonally yields ~254 px/s instead of the
   intended 180 px/s. Not in `CLAUDE.md`'s original bug list.
2. **Unbounded spawn loops** (`Health.cpp`, `Fire.cpp`). Both use
   `do { ... } while (Overlaps(...))` with no attempt cap. This is the
   old B12 — more reachable now that B6's real overlap checks are in
   place, since a large player leaves little free space.
3. **Fire re-rolls its position every frame while hidden**
   (`Fire.cpp`, `Fire_SpawnIfNeeded`). Harmless visually (fire isn't
   drawn while hidden) but wasteful and makes the fire's eventual
   spawn position effectively re-randomized on every single frame
   until it's shown — should pick once per hidden period, not every
   frame.
4. **`SDL__DrawText` opens/closes the font every call**
   (`SDLHelpers.cpp`) — this runs every frame for the score text. Not
   a leak (it does close the font on the success path) but real
   per-frame overhead, and the failure path after
   `TTF_RenderUTF8_Solid` skips `TTF_CloseFont` entirely (leak on that
   specific failure).
5. **`IMG_Quit()` is never called** in `SDL__Destroy` — `SDL_image` is
   initialized but not cleanly torn down on exit.
6. **`Game_Reset` destroys and reloads every texture on every
   restart** (`GameLogic.cpp`). Textures don't need to be invalidated
   between rounds; only round *state* does. This wasn't in
   `CLAUDE.md`'s bug list and is a real catch — it also couples
   restart speed to disk/GPU texture upload cost for no reason.

### Recommended improvements (not bugs, but worth doing before calling this portfolio-final)

7. **Release|x64 build isn't wired up like Debug|x64.**
   `sdl.vcxproj`'s `Debug|x64` config has `AdditionalDependencies` for
   the vcpkg SDL libs and a post-build DLL copy step; `Release|x64`
   has neither. A clean checkout currently cannot produce a working
   Release build.
8. **vcpkg migration is half-done.** `IncludePath`/`LibraryPath` point
   at `C:\vcpkg\installed\x64-windows` (machine-specific, Debug|x64
   only), but there's no `vcpkg.json` manifest, and the vendored
   `SDL2*.lib`/`.dll` files are still sitting in the repo root per
   your prior note. (This was attempted once before and had to be
   reverted after an automated tool left it half-finished — see
   Session 3's risk note.)
9. **`SDL__SetColor` only checks string length (`== 7`), not the `#`
   prefix or that the characters are valid hex.**
10. **Dead drawing helpers remain in `SDLHelpers.cpp`/`.hpp`**
    (`DrawPoint`, `DrawLine`, `DrawRect`, `FillRect`, `DrawCircle`,
    `FillCircle`, `DrawImage`, `Clear`). None are called by the game.
    `DrawLine` still has the known div-by-zero-on-zero-length and
    wrong-axis thickness-offset bugs, `DrawImage` still leaks its
    texture — but since nothing calls them, fixing vs. deleting is a
    judgment call, not an urgent fix.
11. **No tests, no CI.** `Overlaps()`, the growth/shrink math, and
    clamping are all pure logic that doesn't need an SDL window to
    test.
12. **No screenshots or gameplay GIF in `README.md`.**
13. **CMake's `ASSET_DIR` variable is declared and never used.**
14. **`CppProperties.json` only defines an `x64-Debug` configuration**
    (used by some editors for IntelliSense) — minor, cosmetic.
15. **Open item from your own Step 5 notes, status unverified from
    source alone:** whether the unused images (`Bache.png`,
    `BacheGherti_Fast.png`, `Hestler_Fast.png`, `Bozorg.png`,
    `Pause.png`, `speedBooster.png`) were actually deleted, since
    image files aren't visible in a source-only review. Worth a quick
    `git status`/directory check at the start of Session 1.

---

## 3. Architectural changes worth considering (not scheduled in these sessions)

These are real, legitimate suggestions from the code review, but they're
bigger-lift and lower-urgency than anything above — doing them now would
be scope creep for a "get it portfolio-ready" push:

- **`include/`+`src/` directory split.** Purely organizational; no
  behavior change. Worth doing once, not worth doing twice — so if you
  do it, do it *after* Sessions 1–2 land (less churn to rebase).
- **Explicit `active`/`visible` flag instead of a sentinel size.**
  `FireState` already has a real `visible` bool; `HealthState` still
  relies purely on `size <= 0` as its "needs respawn" signal. Mixing
  the two conventions across two structurally similar items is a
  minor inconsistency, not a bug — low priority.
- **RAII wrappers for SDL textures/fonts.** Nice-to-have given the
  project's small size and manual-destroy discipline so far; not worth
  introducing until Session 2's resource-lifecycle work is done, since
  it would touch the same code twice.
- **A `Paused` `GameState` value.** Currently a pure design decision
  (open question in `CLAUDE.md` section 11, #3) — the `P` key is
  tracked in `Events` but `game.cpp`/`GameLogic.cpp` never reads it.
  Needs you to decide if pause is a wanted feature before any code
  changes here.

## 4. Risks and dependencies

- **vcpkg migration (Session 3) is the highest-risk session.** It was
  attempted once before and had to be reverted after an automated tool
  left the repo in a broken, half-migrated state (deleted vendored
  libs, manifest mode never enabled, vcpkg not actually on `PATH` on
  this machine). Do this session manually, in its own branch, and
  confirm both Debug and Release build *before* deleting any vendored
  `.lib`/`.dll` files.
- **No CI yet** means every session's regression testing is manual —
  budget real time for a full win/lose/restart playtest after each
  session, not just a compile check.
- **Toolset `v145`** is required by all four `sdl.vcxproj`
  configurations. Confirm it's actually installed before Session 3;
  if not, retargeting is a prerequisite, not a side effect.
- **Sprite/image licensing is still unverified.** `CLAUDE.md` section
  10 flags that the actual image contents (`Hestler.png`,
  `BacheGherti.png`, etc.) haven't been reviewed for portfolio
  suitability or rights to use. This isn't code work, but it blocks
  calling the repo "done" for a public portfolio.
- **Adding a test framework (Session 4) adds a new build dependency.**
  Doing it after Session 3's vcpkg work lands means you can pull a
  test framework through vcpkg too, instead of vendoring a second
  dependency a different way.

---

## 5. Session plan

### Session 1 — Correctness fixes + documentation sync

**Objective:** Fix the real, still-open gameplay bugs that affect how
the game actually plays, and bring `CLAUDE.md` back in line with the
actual codebase so it's trustworthy as a reference again.

**Tasks (in order):**
1. Verify whether the unused images (`Bache.png`, `*_Fast.png`,
   `Bozorg.png`, `Pause.png`, `speedBooster.png`) were deleted per
   your Step 5 decision; delete now if not.
2. Normalize diagonal movement in `Player_HandleMovement`.
3. Add a bounded-attempt loop to `Health_SpawnIfNeeded` — decide and
   implement the fallback (defer spawn to next frame vs. fall back to
   a scanned grid position) when no valid position is found within the
   attempt cap.
4. Apply the same bounded-attempt fix to `Fire_SpawnIfNeeded`.
5. Change `Fire_SpawnIfNeeded` to pick a position once when fire
   newly becomes needed, instead of re-rolling every frame while
   hidden.
6. Rewrite `CLAUDE.md` sections 0, 8, 11, and 12 to match the verified
   current state (Steps 4–5 done, B9/B11 fixed, win threshold = 500,
   `fairness_*` removed, enemy box correct) and add entries for the
   new fixes made in this session.

**Files:**
- Modify: `Player.cpp`, `Health.cpp`, `Fire.cpp`, `CLAUDE.md`
- New: none expected (a small shared "bounded random placement" helper
  could go in `Collision.hpp`/`.cpp` if the attempt-cap logic ends up
  duplicated between `Health.cpp` and `Fire.cpp` — judgment call during
  implementation, not planned up front)

**Testing:**
- Manual playtest: compare axis-only movement speed to diagonal
  movement speed (should now feel the same).
- Manual playtest: multiple health/fire spawn cycles, confirm normal
  spawn behavior is unchanged and fire's position is now stable while
  hidden (not visibly relevant, but confirms the logic change didn't
  break anything).
- Code-level check that both spawn loops now have a hard exit path —
  can't easily force the "board is full" case in normal play, so this
  is primarily a code-review check, not a playtest.
- Full read-through of the rewritten `CLAUDE.md` against the actual
  files to confirm no remaining stale claims.

**Completion criteria:** Game builds and runs with no regressions in a
full win/lose/restart cycle; diagonal speed matches axis speed;
`CLAUDE.md` contains no claims contradicted by the current source.

---

### Session 2 — SDL resource lifecycle

**Objective:** Fix the real resource-management issues that run on the
live gameplay path (not the dead drawing helpers), and stop `Game_Reset`
from reloading textures it doesn't need to reload.

**Tasks (in order):**
1. Load the score font once (on startup, alongside the other textures)
   and cache it — in `GameContext` or `GameWorld`, whichever better
   matches the existing ownership pattern — instead of opening/closing
   it every `SDL__DrawText` call.
2. Close the font on shutdown, in `SDL__Destroy`.
3. Add `IMG_Quit()` to `SDL__Destroy`.
4. Tighten `SDL__SetColor` to check for a leading `#` and valid hex
   digits, not just `strlen == 7`.
5. Split `Game_Reset` into round-state reset (what it should keep
   doing every restart) and texture loading (what should only happen
   once, at startup) — remove the destroy/reload-all-textures call
   from the restart path.
6. Decide and apply one of: delete the dead drawing helpers
   (`DrawPoint`/`DrawLine`/`DrawRect`/`FillRect`/`DrawCircle`/
   `FillCircle`/`DrawImage`/`Clear`), or move them to a clearly-marked
   unused/legacy file — don't leave them fixed-but-still-dead, since
   nothing calls them either way.

**Files:**
- Modify: `SDLHelpers.hpp`, `SDLHelpers.cpp`, `GameLogic.hpp`,
  `GameLogic.cpp`
- Possibly remove declarations/definitions from `SDLHelpers.hpp`/`.cpp`
  if the dead-helper decision in task 6 is "delete" rather than "move"

**Testing:**
- Full playtest including several restart cycles (both via loss and
  via win) — confirm textures still render correctly after a restart
  now that they're not being reloaded.
- Confirm the score text still renders every frame with no visible
  change in behavior or timing.
- Confirm clean shutdown doesn't produce any new SDL/IMG/TTF error in
  the console log.
- If convenient: watch GPU memory/handle count across 15–20 restart
  cycles to sanity-check there's no new leak introduced by the
  font-caching change.

**Completion criteria:** Score renders identically to before; restart
no longer reloads textures; `SDL__Destroy` balances every init call;
`SDL__SetColor` rejects malformed input instead of silently returning
black; dead helpers are either gone or clearly isolated — not left
mixed in with live code.

---

### Session 3 — Build system: vcpkg manifest + Release config

**Objective:** Finish the vcpkg migration properly this time (manifest
mode, all configs, no leftover vendored binaries), and make Release|x64
buildable from a clean checkout the same way Debug|x64 already is.

**Tasks (in order):**
1. Confirm vcpkg is actually installed and bootstrapped on the dev
   machine before touching any project files (this step is exactly
   what was skipped last time and caused the revert).
2. Add a `vcpkg.json` manifest declaring `sdl2`, `sdl2-image`,
   `sdl2-ttf`.
3. Enable manifest mode in `sdl.vcxproj` for **all four**
   configurations, not just `Debug|x64`.
4. Wire `Release|x64` with the same `AdditionalDependencies` +
   post-build DLL copy step `Debug|x64` already has (using the
   release-flavored lib names, e.g. `SDL2.lib` instead of
   `SDL2d.lib`).
5. Decide whether to also wire the `Win32` configs the same way, or
   explicitly document them as unsupported/x64-only.
6. Build and run **both** Debug|x64 and Release|x64 from a clean
   checkout before removing anything.
7. Only after both configs are confirmed working: remove the vendored
   `SDL2*.lib`/`.dll`, `SDL2_image*.lib`/`.dll`, `SDL2_ttf*.lib`/`.dll`
   from the repo root, and add the vcpkg-generated install directory
   to `.gitignore`.
8. Update `README.md`'s Visual Studio build instructions to match the
   manifest-mode setup (likely simpler than the current instructions,
   since manifest mode installs automatically on build).
9. Re-confirm the CMake path still configures and builds cleanly
   (pkg-config-based CMake isn't being touched, but worth a sanity
   check after changes to the VS side).

**Files:**
- New: `vcpkg.json`
- Modify: `sdl.vcxproj`, `.gitignore`, `README.md`
- Remove (only after step 6 passes): vendored `SDL2*`/`SDL2_image*`/
  `SDL2_ttf*` `.lib`/`.dll` files in the repo root

**Testing:**
- Clean checkout → open `sdl.sln` → build Debug|x64 → run → confirm
  gameplay works.
- Clean checkout → build Release|x64 → run → confirm gameplay works
  (this exact path is currently untested and likely broken).
- Confirm DLLs appear next to the built `.exe` automatically for both
  configs, no manual copying.
- `cmake -B build && cmake --build build` still succeeds.

**Completion criteria:** Both Debug|x64 and Release|x64 build and run
from a clean checkout with zero manual steps beyond having vcpkg
installed; no vendored SDL binaries remain in the repo root; README
build instructions match what actually has to be done.

**Do this session on its own branch**, and don't delete the vendored
binaries until both configs are independently confirmed working — this
is the one session with a documented prior failure.

---

### Session 4 — Minimal tests + portfolio polish

**Objective:** Add a small, real test suite for the pure gameplay logic,
and get the repo's portfolio-facing materials (screenshots, final docs)
actually in place.

**Tasks (in order):**
1. Add a `tests/` directory and pick a lightweight test approach —
   given the project's small size, a header-only framework (e.g.
   doctest) pulled via the vcpkg manifest from Session 3 is a lower-
   overhead choice than a full GoogleTest setup, but either is
   reasonable.
2. Write tests for `Overlaps()`: overlapping, edge-touching, contained,
   and fully separate rectangles — the review correctly flags that
   "edge-touching counts as colliding or not" needs to be decided and
   encoded, not left implicit.
3. Write tests for growth/shrink application: `HEALTH_GROWTH`/
   `FIRE_PENALTY` applied correctly, and position/size staying within
   bounds after clamping.
4. If reasonably isolable without a larger refactor: a couple of
   direction-selection tests for `Enemy.cpp`'s `DirectionIsOpen`/
   `PickOpenDirection` logic (these are currently `static`, so this
   may require either exposing them for testing or testing indirectly
   through `Enemy_Update`'s resulting position — assess during
   implementation rather than committing to an approach now).
5. Wire a test target into `CMakeLists.txt` (`enable_testing()` +
   the test executable) so tests are runnable via `ctest`.
6. Capture real gameplay screenshots and/or a short GIF; add them to
   `README.md`.
7. Final consistency pass: re-check `CLAUDE.md` and `README.md` against
   the state left by Sessions 1–3.

**Files:**
- New: `tests/CollisionTests.cpp`, `tests/MovementTests.cpp` (or
  combined into fewer files if that fits the chosen framework better)
- Modify: `CMakeLists.txt`, `README.md`, `CLAUDE.md`

**Testing:**
- The tests themselves must build and pass — this session's "testing"
  largely *is* the deliverable.
- Confirm `README.md` screenshots/GIF actually render on GitHub (not
  just locally).

**Completion criteria:** `ctest` (or equivalent) runs and passes
locally; README shows real gameplay, not placeholder text; both
reference docs accurately describe the finished state of the project.

---

## 6. After these four sessions

Once Sessions 1–4 are done, the project will have: fixed correctness
bugs, clean resource lifecycle, a reproducible cross-platform build, a
minimal regression-test safety net, and presentable documentation. That
is a reasonable, well-defined stopping point for portfolio purposes.
Beyond it, in rough priority order:

1. **GitHub Actions CI** running the CMake build + test suite on every
   push — the natural next step once there's something for CI to run.
2. **The `include/`/`src/` directory split**, if you want it — purely
   organizational, no behavior risk, best done once the churn from
   Sessions 1–3 has settled.
3. **The pause-feature decision** (section 3's open design question) —
   only worth scheduling once you've decided whether it's a feature you
   actually want, not a bug to fix.
4. **Sprite/image licensing review** — not code work, but needed before
   calling the repo portfolio-final, independent of any session above.
5. Time beyond this is likely better spent on your other in-progress
   projects (the fire management system, the embedded systems
   coursework) than on further polish of a small, now-solid 2D game.
