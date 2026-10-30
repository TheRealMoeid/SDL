#include "Health.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "Fire.hpp"
#include "Collision.hpp"
#include "GameConstants.hpp"
#include <cstdlib>

// B12: capped instead of unbounded - if nothing valid is found this
// frame, health.size just stays <= 0 and this is retried next frame.
static constexpr int SPAWN_MAX_ATTEMPTS = 100;

void Health_SpawnIfNeeded(HealthState& health, const PlayerState& player, const EnemyState& enemy, const FireState& fire, int window_size) {
    if (health.size > 0) return;

    SDL_Rect playerRect{ (int)player.x, (int)player.y, (int)player.size, (int)player.size };
    SDL_Rect enemyRect{ (int)enemy.x, (int)enemy.y, 50, 50 };

    auto isBlocked = [&](const SDL_Rect& candidate) {
        return Overlaps(candidate, playerRect) ||
            Overlaps(candidate, enemyRect) ||
            (fire.visible && Overlaps(candidate, SDL_Rect{ fire.x, fire.y, (int)fire.size, (int)fire.size }));
        };

    int x, y;
    if (FindSpawnPosition(window_size, ITEM_SPAWN_SIZE, SPAWN_MAX_ATTEMPTS, isBlocked, x, y)) {
        health.x = x;
        health.y = y;
        health.size = ITEM_SPAWN_SIZE;
    }
}

void Health_Shrink(HealthState& health, Uint32 deltatime) {
    health.size -= 10 * deltatime / 1000.0;
}