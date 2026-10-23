#include "Fire.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "Health.hpp"
#include "Collision.hpp"
#include <cstdlib>
#include <cmath>

void Fire_SpawnIfNeeded(FireState& fire, const PlayerState& player, const EnemyState& enemy, const HealthState& health, int window_size) {
    // Mirrors the original "sizeFire <= 0 || !fire_visable" spawn condition,
    // still re-rolled every frame while hidden (B12, not yet addressed).
    if (fire.size > 0 && fire.visible) return;

    SDL_Rect candidate;
    do {
        fire.x = rand() % (window_size - 50);
        fire.y = rand() % (window_size - 50);
        candidate = { fire.x, fire.y, 50, 50 };
    } while (Overlaps(candidate, SDL_Rect{ (int)enemy.x, (int)enemy.y, 50, 50 }) ||
        Overlaps(candidate, SDL_Rect{ (int)player.x, (int)player.y, (int)player.size, (int)player.size }) ||
        (health.size > 0 && Overlaps(candidate, SDL_Rect{ health.x, health.y, (int)health.size, (int)health.size })));

    fire.size = 50;
    fire.fairness = (int)sqrt(((fire.x - player.x) * (fire.x - player.x)) +
        ((fire.y - player.y) * (fire.y - player.y)));
}

void Fire_Shrink(FireState& fire, Uint32 deltatime) {
    // Not gated by GameState::Playing in the original - preserved exactly,
    // this is outside B9/B11's scope. Worth flagging to the owner: fire
    // keeps shrinking (and can hide itself) during Game Over/Victory while
    // health does not, which is an existing asymmetry, not a new one.
    if (!fire.visible) return;
    fire.size -= 10 * deltatime / 1000.0;
    if (fire.size <= 0) {
        fire.visible = false;
        fire.size = -1;
    }
}
