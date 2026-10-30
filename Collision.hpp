#pragma once
#include "SDLHelpers.hpp"
#include <functional>

struct PlayerState;
struct HealthState;
struct FireState;
struct EnemyState;

// B6: real rectangle-overlap test, reused for spawn placement and for the
// player's collisions with health, fire and the enemy.
bool Overlaps(const SDL_Rect& a, const SDL_Rect& b);

bool CollidedWithHealth(const PlayerState& player, const HealthState& health);
bool CollidedWithFire(const PlayerState& player, const FireState& fire);
bool CollidedWithEnemy(const PlayerState& player, const EnemyState& enemy);

// B12: bounded random placement, shared by Health_SpawnIfNeeded and
// Fire_SpawnIfNeeded. Tries up to max_attempts random positions inside
// the window (item_size square) and accepts the first one isBlocked()
// rejects. Returns false (out_x/out_y untouched) if nothing valid turns
// up within the attempt cap, instead of looping forever.
bool FindSpawnPosition(int window_size, int item_size, int max_attempts,
    const std::function<bool(const SDL_Rect&)>& isBlocked,
    int& out_x, int& out_y);