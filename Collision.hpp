#pragma once
#include "SDLHelpers.hpp"

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
