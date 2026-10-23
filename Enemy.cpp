#include "Enemy.hpp"
#include "GameConstants.hpp"
#include <cstdlib>

void Enemy_Update(EnemyState& enemy, Uint32 deltatime, int window_size) {
    enemy.direction_timer += deltatime / 1000.0;
    if (enemy.direction_timer >= ENEMY_DIRECTION_INTERVAL) {
        enemy.direction = (rand() % 4) + 1;
        enemy.direction_timer = 0.0;
    }

    double step = ENEMY_SPEED * deltatime / 1000.0;
    bool moved = false;

    if (enemy.direction == 1 && (0 < enemy.y - step) && (enemy.y < (window_size - 50))) {
        enemy.y -= step;
        moved = true;
    }
    if (enemy.direction == 2 && (0 < enemy.y) && (enemy.y + step < (window_size - 50))) {
        enemy.y += step;
        moved = true;
    }
    if (enemy.direction == 3 && (0 < enemy.x) && (enemy.x + step < (window_size - 50))) {
        enemy.x += step;
        moved = true;
    }
    if (enemy.direction == 4 && (0 < enemy.x - step) && (enemy.x < (window_size - 50))) {
        enemy.x -= step;
        moved = true;
    }

    // B9: re-roll immediately instead of waiting out the timer when blocked.
    if (!moved) {
        enemy.direction = (rand() % 4) + 1;
        enemy.direction_timer = 0.0;
    }
}
