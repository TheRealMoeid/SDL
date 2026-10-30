#include "Fire.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "Health.hpp"
#include "Collision.hpp"
#include "GameConstants.hpp"
#include <cstdlib>

static constexpr int SPAWN_MAX_ATTEMPTS = 100;

void Fire_SpawnIfNeeded(FireState& fire, const PlayerState& player, const EnemyState& enemy, const HealthState& health, int window_size) {
    // B20: the old guard was "size > 0 && visible", which re-rolled the
    // candidate position every single frame while fire sat hidden
    // (visible stays false until GameLogic flips it on). Guarding on
    // size alone means a candidate is picked once and held until it's
    // actually consumed (shrunk to 0 after being made visible).
    if (fire.size > 0) return;

    SDL_Rect playerRect{ (int)player.x, (int)player.y, (int)player.size, (int)player.size };
    SDL_Rect enemyRect{ (int)enemy.x, (int)enemy.y, 50, 50 };

    auto isBlocked = [&](const SDL_Rect& candidate) {
        return Overlaps(candidate, enemyRect) ||
            Overlaps(candidate, playerRect) ||
            (health.size > 0 && Overlaps(candidate, SDL_Rect{ health.x, health.y, (int)health.size, (int)health.size }));
        };

    // B12: bounded attempts, same as Health_SpawnIfNeeded.
    int x, y;
    if (FindSpawnPosition(window_size, ITEM_SPAWN_SIZE, SPAWN_MAX_ATTEMPTS, isBlocked, x, y)) {
        fire.x = x;
        fire.y = y;
        fire.size = ITEM_SPAWN_SIZE;
    }
}

void Fire_Shrink(FireState& fire, Uint32 deltatime) {
    // Still not gated by GameState::Playing - unchanged, outside this
    // session's scope (see CLAUDE.md B9/B20 notes on the existing
    // asymmetry with Health_Shrink).
    if (!fire.visible) return;
    fire.size -= 10 * deltatime / 1000.0;
    if (fire.size <= 0) {
        fire.visible = false;
        fire.size = -1;
    }
}