# Complete Code Review: Square Baby (SDL2 / C++)

**Repository:** [TheRealMoeid/SDL](https://github.com/TheRealMoeid/SDL)  
**Review type:** Static source-code and repository review  
**Review date:** October 30, 2026  
**Scope:** Architecture, gameplay logic, SDL resource management, enemy AI, build system, tests, documentation, and portfolio readiness.

> **Important limitation:** This review is based on source and repository inspection. The project was not compiled, launched, or played as part of this review, so runtime behavior and build reproducibility have not been independently verified.

## Executive recommendation

**Do not rewrite the game or add major features yet. First make the existing implementation reliable, testable, and reproducibly buildable.** The repository has a reasonable foundation: the game is divided into focused source files, and the enemy-direction selection has recently improved. The highest-value next steps are to make random item spawning guaranteed to terminate, establish clear SDL resource ownership, make initialization failures explicit, and add tests around pure gameplay logic.

The ratings below are qualitative engineering assessments, not measured performance scores.

| Area | Assessment | Priority |
|---|---|---|
| Architecture | A reasonable foundation; current separation is worth keeping while addressing correctness | Medium |
| Gameplay edge cases | Several edge cases need explicit handling and regression tests | High |
| SDL resource management | Concrete lifecycle and cleanup concerns in helper and reset paths | High |
| Enemy AI | Recent improvement is useful; add predictable fallback behavior and tests | Medium |
| Build system | Windows and CMake workflows need to be made consistent and reproducible | High |
| Automated tests | No test directory or CI workflow was visible in the inspected repository tree | High |
| Documentation | Useful starting point, but some instructions and historical notes need syncing | Medium |
| Portfolio readiness | Potentially presentable after stability, screenshots, clean setup instructions, and CI | Medium |

---

## 1. Highest-priority correctness and reliability issues

### P1 — Make item spawning bounded

**Files:** `Health.cpp`, `Fire.cpp`

The item spawn functions use `do-while`-style random position selection. If the game state leaves no valid location, an unbounded loop can run indefinitely.

**Recommended change:**

- Bound the number of random attempts.
- If random attempts fail, use a finite scan of possible grid positions, if the game uses a grid that can be enumerated.
- Return an explicit success/failure result from the spawn operation.
- Define what should happen when no valid position exists: for example, defer the spawn until a later update or mark the item inactive.
- Avoid changing the existing position or visibility state until a valid placement has been found.

**Tests to add:**

- A valid location is selected when one is available.
- An item is not placed on an occupied or otherwise invalid location.
- A fully occupied play area causes the spawn request to fail promptly rather than loop forever.
- A failed spawn leaves the existing state consistent.

This should be the first implementation task because it removes a potentially unbounded failure mode and can be addressed without redesigning the entire game.

### P1 — Fix SDL helper resource ownership and failure paths

**File:** `SDLHelpers.cpp`

The helper layer has several concerns:

- `SDL__DrawImage` creates a texture but does not destroy it, creating a potential GPU/resource leak when called repeatedly.
- `SDL__DrawText` opens a font each time it is called. If used during normal frame rendering, repeated font opens create unnecessary overhead.
- In the text-rendering path, a failure after `TTF_RenderUTF8_Solid` can return without closing the font.
- Texture creation results should be validated before rendering or dereferencing the returned texture.
- Successful shutdown does not call `IMG_Quit`.
- `SDL__CreateWindow` checks whether `IMG_Init` returned zero, but a nonzero return does not necessarily mean every requested image format was initialized. Check the required flags explicitly.

**Recommended change:**

- Give each SDL resource a clear owner and lifetime.
- Load fonts once and reuse them; close them during shutdown.
- Ensure temporary surfaces and textures are released on both success and failure paths.
- Check every allocation/creation result before using it.
- Centralize SDL subsystem initialization and teardown, and only release subsystems that were successfully initialized.
- Prefer RAII wrappers or smart pointers with appropriate SDL deleters where they fit the current project structure.

If a resource is intended to be cached, make the cache and its lifetime explicit rather than relying on callers to manage it implicitly.

### P1 — Separate round reset from resource initialization

**File:** `GameLogic.cpp`

`Game_Reset` destroys and reloads textures unnecessarily. Resetting a round should normally reset gameplay state, not tear down and reconstruct resources that remain valid for the lifetime of the application.

**Recommended change:**

Separate the operations into clearly named responsibilities, such as:

- application/resource initialization;
- round-state initialization or reset;
- per-frame gameplay updates;
- application/resource cleanup.

A round reset should reset player/enemy positions, health, fire state, timers, score, and game-state fields as appropriate, while preserving persistent resources. The exact list should be checked against the current data model and desired restart behavior.

**Tests to add:**

- Reset after loss returns all round state to the expected initial values.
- Reset after victory behaves consistently.
- Reset does not recreate persistent textures or fonts unnecessarily.

### P1 — Make initialization failure explicit

**Relevant location:** `LoadTextureChecked` and the resource-loading path.

`LoadTextureChecked` logs a failure but allows the application to continue with a null texture. That can lead to later failures far away from the real cause.

**Recommended change:**

- Make resource loading report success or failure explicitly.
- If a required texture or font cannot be loaded, stop initialization cleanly and display a useful error message.
- Ensure partial initialization is safely cleaned up.
- Distinguish required resources from optional resources, if any exist.
- Do not allow gameplay to start when a required resource is missing.

### P2 — Cap large frame deltas or use a fixed timestep

**Files:** `game.cpp`, `Enemy.cpp`

Frame-delta-based updates can produce large movement steps after a pause, window drag, debugger stop, or other long frame. Depending on the collision/update order, an object may move too far in one update or pass through a boundary.

**Recommended change:**

- As a minimal change, clamp abnormally large frame deltas before using them for movement.
- As a larger follow-up, consider a fixed-timestep simulation with rendering decoupled from updates.
- Keep the solution proportional to the game's scope; a full engine-style loop is not necessary unless the current behavior requires it.

**Tests to add:**

- Movement remains bounded after an unusually long frame.
- Boundary constraints still hold after a large delta.
- Behavior is consistent at representative frame rates, including 30, 60, and 144 FPS.

### Additional gameplay improvements

#### Normalize diagonal movement

If horizontal and vertical movement are each scaled by `180` pixels per second, diagonal movement becomes approximately `180 × √2 = 254.6` pixels per second. Normalize the movement vector when both axes are active if equal movement speed in every direction is intended.

#### Do not reroll hidden fire coordinates every frame

`Fire_SpawnIfNeeded` rerolls hidden fire coordinates each frame. Select a location when a spawn is actually required, rather than repeatedly changing the location of an inactive item. This makes behavior easier to reason about and avoids unnecessary work.

#### Replace sentinel state with an explicit active state

Health and fire lifecycle logic uses a value such as `size = -1` to indicate an inactive item. Prefer an explicit `active`/`visible` state, or a well-defined lifecycle representation, so geometry and lifecycle are not conflated. Before changing this, verify all call sites and rendering/collision assumptions.

#### Keep enum-based game states

The existing `GameState` enum is a good basis for representing menu, active play, victory, and loss states. Keep it rather than replacing it with unrelated Boolean flags. Ensure state transitions are centralized and tested.

---

## 2. Enemy AI

The recent change in `Enemy.cpp` to select a direction from available options rather than blindly rerolling directions is an improvement. Pull request #25, “Enemy: pick direction from open options instead of blind reroll (B18),” was merged on October 29, 2026.

The next improvement should be targeted robustness, not a sophisticated AI rewrite.

**Recommended improvements:**

1. **Use a shared candidate-position validity check.** Avoid duplicating boundary/collision conditions across direction-selection branches. The same rules should govern both choosing a move and validating the resulting position.
2. **Retain the current direction when it remains valid.** Randomly changing direction every update can look erratic. Choose a new direction when needed, rather than merely because a new choice is possible.
3. **Define a deadlock policy.** If no direction is available, explicitly decide whether the enemy waits, retries later, or follows another deterministic fallback. Avoid loops that repeatedly sample impossible moves.
4. **Keep enemies inactive outside active gameplay.** Confirm that enemy updates do not continue to move or change state while the game is in a menu, victory, or loss screen unless that behavior is intentional.
5. **Add boundary and blocked-movement regression tests.** Test corners, edges, obstacles/occupied positions, and the case where all candidate directions are blocked.

Do not introduce pathfinding, behavior trees, or a more complex AI system unless the game design actually needs them.

---

## 3. Build-system and dependency management

### Visual Studio project configuration

**Files:** `sdl.vcxproj`, `sdl.vcxproj.filters`

The project contains hardcoded dependency paths such as `C:\vcpkg\installed\x64-windows` for the Debug x64 configuration. The Release x64 configuration does not appear to have equivalent dependency/linking and runtime-DLL deployment treatment. These configurations should be checked individually rather than assuming that a successful Debug build implies a successful Release build.

**Recommended change:**

- Avoid machine-specific absolute paths where practical.
- Confirm that Debug and Release use consistent include paths, libraries, and runtime dependencies.
- Ensure required DLLs are available when launching the built game from a clean checkout.
- Document the expected compiler, architecture, and dependency setup.

### CMake and SDL discovery

**File:** `CMakeLists.txt`

CMake currently uses `PkgConfig` to locate SDL2, SDL2_image, and SDL2_ttf. That can be reasonable on systems with the relevant pkg-config metadata installed, but it makes the Windows setup less straightforward when dependencies are managed through vcpkg.

The `ASSET_DIR` variable is declared but appears unused, and some comments describe asset paths that no longer match the current `assets/` layout.

**Recommended change:**

- Choose a clear, documented source of truth for building the project. CMake is a reasonable candidate for a portable build configuration.
- Consider a vcpkg manifest (`vcpkg.json`) so the SDL dependencies and versions are declared with the project rather than relying on undocumented machine state.
- If using CMake with vcpkg, document the toolchain configuration and package names in the setup instructions.
- Copy or otherwise make the `assets/` directory available beside the built executable in a predictable way.
- Remove stale comments and unused CMake variables, or implement them if they are intended to control behavior.
- Verify a clean Debug and Release build and launch, not just successful configuration.

The inspected repository tree did not contain a root-level `vcpkg.json`, so the manifest-based setup would be a new addition unless the repository has changed since the review.

The official SDL CMake guidance is available at: [SDL CMake documentation](https://github.com/libsdl-org/SDL/blob/main/docs/README-cmake.md).

### Recommended build strategy

Make one documented build path reliable first. A reasonable plan is:

1. Establish a reproducible CMake build.
2. Declare dependencies in a package manifest if using vcpkg.
3. Make asset paths and deployment behavior consistent.
4. Verify Debug and Release builds from a clean checkout.
5. Add CI to build and run tests on each pull request.
6. Add a second operating system only after the primary build is stable.

Do not maintain two independent build instructions that quietly depend on different library versions or directory layouts.

---

## 4. Architecture: improve in small steps

The current source-file separation is useful. The goal is to improve ownership and make gameplay logic testable without turning a small game into an overengineered framework.

### Suggested direction

```text
SquareBaby/
├── CMakeLists.txt
├── vcpkg.json
├── README.md
├── LICENSE
├── assets/
│   ├── fonts/
│   └── images/
├── include/
│   ├── Game.hpp
│   ├── GameConstants.hpp
│   ├── GameLogic.hpp
│   ├── Player.hpp
│   ├── Enemy.hpp
│   ├── Health.hpp
│   ├── Fire.hpp
│   ├── Collision.hpp
│   ├── Renderer.hpp
│   └── SDLHelpers.hpp
├── src/
│   ├── main.cpp
│   ├── Game.cpp
│   ├── GameLogic.cpp
│   ├── Player.cpp
│   ├── Enemy.cpp
│   ├── Health.cpp
│   ├── Fire.cpp
│   ├── Collision.cpp
│   ├── Renderer.cpp
│   └── SDLHelpers.cpp
└── tests/
    ├── CollisionTests.cpp
    ├── MovementTests.cpp
    ├── SpawnTests.cpp
    └── GameStateTests.cpp
```

This is a **possible target structure, not a demand to move everything immediately**. The existing root-level layout is functional. Do not perform a large file reorganization as the first task; first address the correctness and testing gaps.

### Design principles to follow

- Keep `game.cpp` or the main entry point thin. It should coordinate initialization, the main loop, and shutdown rather than contain all gameplay rules.
- Separate frame updates, collision decisions, round reset, rendering, and application teardown where their responsibilities are currently mixed.
- Give SDL initialization, fonts, textures, surfaces, and windows clear ownership and cleanup paths.
- Centralize gameplay constants rather than scattering repeated values across modules.
- Keep small data structures simple. Do not create classes merely to wrap a few fields if doing so does not improve ownership or behavior.
- Use RAII or smart-pointer wrappers with SDL-specific deleters when that reduces the chance of leaks and double-free errors.
- Make pure gameplay decisions callable without opening an SDL window. This is the key to low-friction unit testing.

---

## 5. Automated tests and continuous integration

No `tests/` directory or GitHub Actions workflow was visible in the inspected repository tree. That makes it harder to detect regressions while fixing gameplay and resource-handling issues.

### Start with pure gameplay tests

Consider a lightweight test framework such as GoogleTest, or another small framework that fits the existing build. Prioritize tests that do not require a window, GPU, or audio device.

| Test area | Example cases |
|---|---|
| Collision | Overlapping rectangles, edge-touching behavior, containment, separated rectangles |
| Movement | Horizontal and vertical movement, diagonal normalization, boundary clamping |
| Spawn | Valid placement, overlap rejection, no-free-space termination |
| Game state | Reset after loss, reset after victory, expected initial values |
| Enemy movement | Edge/corner behavior, blocked directions, no valid direction |
| Frame timing | Large delta, stable boundary handling, bounded movement |

Document the intended semantics, especially whether two rectangles that only touch at an edge count as colliding. Tests should encode the game's chosen behavior rather than assume a particular answer.

### Add CI after the build is reproducible

A useful first CI workflow should:

- configure and build the project on a clean Windows runner;
- build the relevant configuration(s), preferably Debug and Release where practical;
- run the automated tests;
- fail on build or test errors;
- report enough context to diagnose a failure.

A Linux build can be a later step once dependency discovery, filesystem assumptions, and asset paths are handled portably. CI should reinforce a known-good build process rather than try to compensate for an unclear one.

---

## 6. Specific SDL helper review

**File:** `SDLHelpers.cpp` (and the matching declarations in `SDLHelpers.hpp`)

| Issue | Why it matters | Recommended action |
|---|---|---|
| `SDL__DrawImage` creates a texture without destroying it | Repeated calls may leak GPU resources | Destroy temporary textures or use a cache with explicit ownership |
| `SDL__DrawText` opens a font every call | Repeated file/resource work can add frame-time overhead | Cache/reuse fonts and close them during shutdown |
| Error path may skip `TTF_CloseFont` | Failure paths can leak resources | Make every return path release owned resources, preferably through RAII |
| Texture creation result is not validated consistently | Null resources may reach rendering calls | Check all creation functions and propagate errors |
| `SDL__DrawLine` divides by line length without handling zero length | A zero-length line can trigger invalid arithmetic | Define and test zero-length behavior before division |
| `SDL__DrawLine` thickness offset is calculated along the line rather than perpendicular | Thick lines may not appear centered or behave as expected | Compute a perpendicular offset and test horizontal, vertical, diagonal, and zero-length cases |
| `SDL__SetColor` checks string length but not format or hex digits | Invalid color input can be accepted or parsed incorrectly | Validate the `#` prefix and hex digits, and report invalid values explicitly |
| `SDL__SetColor` mutates renderer draw color | Hidden global state can affect later drawing operations | Document the side effect or pass color explicitly where appropriate |
| Successful shutdown omits `IMG_Quit` | SDL_image subsystem cleanup is incomplete | Balance initialization and cleanup, including partial-init failure paths |
| `SDL__CreateWindow` only checks `IMG_Init(...) == 0` | A nonzero result may still omit one requested format | Check the specific required initialization flags |
| Some drawing helpers appear unused | Dead or unverified code increases maintenance burden | Decide whether each helper is intended; remove unused functions or test and document them |

The safest pattern is to make resource ownership and error propagation explicit. Avoid broad refactors until the expected behavior of the current helpers has been tested.

---

## 7. Documentation and portfolio readiness

The README is a useful starting point: it describes the game's purpose, rules, enemy behavior, dependencies, and setup. The next pass should make the repository easy for a new visitor to understand and build.

### README improvements

- Add real gameplay screenshots or a short GIF/video captured from the actual game. Do not use generic or unrelated artwork as a substitute for gameplay evidence.
- Add an architecture overview explaining the responsibilities of the main modules.
- Provide a concise, clean-checkout build guide with exact prerequisites and commands.
- State which configurations and operating systems have actually been tested.
- Include known limitations or unfinished features rather than implying they are implemented.
- Document controls accurately. The `P` key is tracked in `SDLHelpers.cpp`, but the game does not appear to implement a pause behavior; either document that it is not implemented or address it in a separate feature task later.
- Check whether all assets and fonts have appropriate licenses/attribution. Document any font replacement or asset changes where required.
- Remove outdated asset-path instructions and align comments with the current folder layout.

### Repository metadata

A short repository description could be:

> A 2D survival game built with C++ and SDL2, featuring enemy movement, health pickups, fire hazards, and a simple game-state loop.

Suggested GitHub topics:

`cpp`, `c-plus-plus`, `sdl2`, `game-development`, `2d-game`, `cmake`, `game-programming`

Only retain topics that accurately describe the final repository. For example, include `cmake` once CMake is a supported and documented build path.

### Branch housekeeping

There are several older branches. Check whether each contains unique work and whether its commits are already merged or otherwise preserved before deleting any branch. Do not delete branches based on age alone.

### Suggested project context documents

If using AI assistants or handing the project to another developer, maintain a small set of current documents:

- `ARCHITECTURE.md` — module responsibilities, resource ownership, main loop, and state transitions.
- `PROJECT_CONTEXT.md` — current development stage, build instructions, and next task.
- `KNOWN_ISSUES.md` — confirmed defects, reproduction steps, and workarounds.
- `docs/CLAUDE.md` — update historical notes that refer to the earlier monolithic implementation or old asset paths, so they do not mislead future contributors.

Keep these documents factual and update them as the code changes. Avoid duplicating the same instructions in multiple documents unless the source of truth is clear.

---

## 8. Recommended implementation roadmap

Do these in small, reviewable steps. The roadmap is ordered by risk reduction and leverage, not by how visually impressive a change might look.

### Phase 1 — Reliability and resource lifecycle

1. Bound item-spawn loops and define the no-valid-position behavior.
2. Fix texture/font/surface cleanup and SDL subsystem shutdown.
3. Make required-resource loading failures stop initialization cleanly.
4. Separate round reset from persistent resource initialization.
5. Normalize diagonal movement if equal directional speed is intended.
6. Avoid rerolling inactive fire positions every frame.

**Exit criteria:** No unbounded spawn retry; resource ownership is documented; failure paths clean up; reset behavior is predictable.

### Phase 2 — Test the important logic

1. Introduce a test target.
2. Add collision, movement, spawn, enemy-boundary, and game-reset tests.
3. Make gameplay logic callable without creating an SDL window where practical.

**Exit criteria:** Core rules have regression tests, including the edge cases that motivated Phase 1.

### Phase 3 — Reproducible builds and CI

1. Select and document the primary build path.
2. Align CMake/Visual Studio dependency handling and asset deployment.
3. Validate clean Debug and Release builds.
4. Add CI for building and running tests.

**Exit criteria:** A new contributor can build from a clean checkout using documented steps, and CI verifies the main path.

### Phase 4 — Incremental architecture cleanup

1. Separate application lifetime from round lifetime.
2. Clarify ownership of SDL resources.
3. Centralize shared movement/collision constraints and gameplay constants.
4. Move headers and implementations into clearer directories only when useful, updating build files in the same task.

**Exit criteria:** Responsibilities and ownership are easy to locate; restructuring has not changed gameplay behavior unintentionally.

### Phase 5 — Portfolio polish

1. Add authentic screenshots or a short gameplay capture.
2. Update the README and project context docs.
3. Verify asset/font licensing and attribution.
4. Review repository topics, description, and branch housekeeping.

**Exit criteria:** The project is easy to understand, build, and evaluate from a clean repository page.

---

## 9. Recommended first task and development workflow

### First task: item-spawning reliability

Start with `Health.cpp` and `Fire.cpp`. The bounded-spawn change is focused, reduces a potentially severe failure mode, and has clear success criteria. Do not combine it with a large architecture rewrite or broad file moves.

Before implementation, inspect the relevant call sites and answer these questions:

- Which positions are considered valid, and which objects can occupy them?
- Is the play area grid-based or continuous?
- What should happen if no spawn location is available?
- Does the caller need to know whether spawning succeeded?
- Can the selection logic be extracted into a pure function and tested without SDL?

Then implement the smallest change that establishes bounded termination and consistent state.

### Preferred contribution workflow

1. Inspect the relevant code, documentation, and current repository state.
2. Define the root cause, affected files, risks, alternatives, and tests before coding.
3. Keep one issue or cohesive task per branch.
4. Provide code changes as a `.patch` file, rather than modifying the user's working tree directly.
5. Include exact commands to apply the patch and run tests, plus the expected result.
6. Have the repository owner apply the patch, run the tests, inspect the diff, and commit on the task branch.
7. Merge into `main` only after the change is verified.

Example local sequence, after a patch and branch name have been agreed:

```bash
git switch main
git pull --ff-only
git switch -c fix/bounded-item-spawning
git apply --check path/to/change.patch
git apply path/to/change.patch
# Run the documented build and tests; inspect the diff before committing.
git diff --check
git diff
git status
```

Only commit after the patch has been applied and the relevant tests have passed. Adjust commands if the local checkout or remote workflow differs.

---

## 10. Verification status and limits

This document is a **source-level review**, not a completed runtime verification report.

- The repository files and available documentation were inspected.
- The project was not compiled as part of this review.
- The game was not launched or played as part of this review.
- The potential failure modes described above should be confirmed with targeted tests while implementing the fixes.
- Build configuration concerns should be verified using a clean checkout and each supported build configuration.
- Repository state can change after the review date; re-check current files and branches before applying changes.

**No code was changed as part of this review.** The purpose of this report is to establish a practical, risk-ordered improvement plan before implementation.
