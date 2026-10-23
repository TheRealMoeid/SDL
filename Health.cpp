#include "Health.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "Fire.hpp"
#include "Collision.hpp"
#include <cstdlib>
#include <cmath>

void Health_SpawnIfNeeded(HealthState& health, const PlayerState& player, const EnemyState& enemy, const FireState& fire, int window_size) {
    if (health.size > 0) return;

    SDL_Rect candidate;
    do {
        health.x = rand() % (window_size - 50);
        health.y = rand() % (window_size - 50);
        candidate = { health.x, health.y, 50, 50 };
    } while (Overlaps(candidate, SDL_Rect{ (int)player.x, (int)player.y, (int)player.size, (int)player.size }) ||
        Overlaps(candidate, SDL_Rect{ (int)enemy.x, (int)enemy.y, 50, 50 }) ||
        (fire.visible && Overlaps(candidate, SDL_Rect{ fire.x, fire.y, (int)fire.size, (int)fire.size })));

    health.size = 50;
    health.fairness = (int)sqrt(((health.x - player.x) * (health.x - player.x)) +
        ((health.y - player.y) * (health.y - player.y)));
}

void Health_Shrink(HealthState& health, Uint32 deltatime) {
    health.size -= 10 * deltatime / 1000.0;
}
