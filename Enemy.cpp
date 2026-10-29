#include "Enemy.hpp"
#include "GameConstants.hpp"
#include "Health.hpp"
#include "Fire.hpp"
#include "Collision.hpp"
#include <cstdlib>

// B16/B17: item collision for the enemy. health_margin is the avoidance
// buffer around health (0 means "real item size only" - used as a
// fallback so the soft avoidance margin never permanently traps the
// enemy against a wall or another item).
static bool EnemyBlockedAt(double x, double y, const HealthState& health, const FireState& fire, double health_margin) {
    SDL_Rect candidate{ (int)x, (int)y, ITEM_SPAWN_SIZE, ITEM_SPAWN_SIZE };

    if (health.size > 0) {
        SDL_Rect health_rect{
            (int)(health.x - health_margin),
            (int)(health.y - health_margin),
            (int)(ITEM_SPAWN_SIZE + 2 * health_margin),
            (int)(ITEM_SPAWN_SIZE + 2 * health_margin)
        };
        if (Overlaps(candidate, health_rect)) return true;
    }

    if (fire.visible &&
        Overlaps(candidate, SDL_Rect{ fire.x, fire.y, (int)fire.size, (int)fire.size }))
        return true;

    return false;
}

static bool EnemyTryMove(double x, double y, const HealthState& health, const FireState& fire) {
    if (!EnemyBlockedAt(x, y, health, fire, ENEMY_HEALTH_AVOID_MARGIN)) return true;
    if (!EnemyBlockedAt(x, y, health, fire, 0.0)) return true;
    return false;
}

// B18: can the enemy actually take a step in this direction right now -
// combines the wall bounds check and the item check into one query so
// direction selection can test all four options up front instead of
// discovering a block only after committing to a direction.
static bool DirectionIsOpen(int direction, double x, double y, double step, int window_size, const HealthState& health, const FireState& fire) {
    switch (direction) {
    case 1: return (0 < y - step) && (y < (window_size - 50)) && EnemyTryMove(x, y - step, health, fire);
    case 2: return (0 < y) && (y + step < (window_size - 50)) && EnemyTryMove(x, y + step, health, fire);
    case 3: return (0 < x) && (x + step < (window_size - 50)) && EnemyTryMove(x + step, y, health, fire);
    case 4: return (0 < x - step) && (x < (window_size - 50)) && EnemyTryMove(x - step, y, health, fire);
    default: return false;
    }
}

// B18: picks a direction the enemy can actually move in right now, instead
// of rolling blind and finding out next frame. Returns 0 if every
// direction is currently blocked (true deadlock - same fallback as before).
static int PickOpenDirection(double x, double y, double step, int window_size, const HealthState& health, const FireState& fire) {
    int open[4];
    int count = 0;
    for (int d = 1; d <= 4; ++d) {
        if (DirectionIsOpen(d, x, y, step, window_size, health, fire)) {
            open[count++] = d;
        }
    }
    if (count == 0) return 0;
    return open[rand() % count];
}

void Enemy_Update(EnemyState& enemy, const HealthState& health, const FireState& fire, Uint32 deltatime, int window_size) {
    enemy.direction_timer += deltatime / 1000.0;
    double step = ENEMY_SPEED * deltatime / 1000.0;

    bool timer_elapsed = enemy.direction_timer >= ENEMY_DIRECTION_INTERVAL;
    bool current_blocked = enemy.direction == 0 ||
        !DirectionIsOpen(enemy.direction, enemy.x, enemy.y, step, window_size, health, fire);

    // B18: only re-evaluate direction when the timer is up or the current
    // one is actually blocked - otherwise keep going the same way, so it
    // doesn't twitch between directions every frame.
    if (timer_elapsed || current_blocked) {
        int picked = PickOpenDirection(enemy.x, enemy.y, step, window_size, health, fire);
        if (picked != 0) {
            enemy.direction = picked;
        }
        // picked == 0: true deadlock, nothing open anywhere - keep the old
        // direction and try again next frame (matches prior fallback).
        enemy.direction_timer = 0.0;
    }

    bool moved = false;
    if (enemy.direction != 0 && DirectionIsOpen(enemy.direction, enemy.x, enemy.y, step, window_size, health, fire)) {
        switch (enemy.direction) {
        case 1: enemy.y -= step; break;
        case 2: enemy.y += step; break;
        case 3: enemy.x += step; break;
        case 4: enemy.x -= step; break;
        }
        moved = true;
    }

    if (!moved) {
        enemy.direction = 0;
        enemy.direction_timer = ENEMY_DIRECTION_INTERVAL; // force a re-pick next frame
    }
}