#include "Collision.hpp"
#include "Player.hpp"
#include "Health.hpp"
#include "Fire.hpp"
#include "Enemy.hpp"
#include <cstdlib>

bool Overlaps(const SDL_Rect& a, const SDL_Rect& b) {
    if (a.x + a.w <= b.x || b.x + b.w <= a.x) return false;
    if (a.y + a.h <= b.y || b.y + b.h <= a.y) return false;
    return true;
}

bool CollidedWithHealth(const PlayerState& player, const HealthState& health) {
    SDL_Rect p{ (int)player.x, (int)player.y, (int)player.size, (int)player.size };
    SDL_Rect h{ health.x, health.y, (int)health.size, (int)health.size };
    return Overlaps(p, h);
}

bool CollidedWithFire(const PlayerState& player, const FireState& fire) {
    SDL_Rect p{ (int)player.x, (int)player.y, (int)player.size, (int)player.size };
    SDL_Rect f{ fire.x, fire.y, (int)fire.size, (int)fire.size };
    return Overlaps(p, f);
}

bool CollidedWithEnemy(const PlayerState& player, const EnemyState& enemy) {
    SDL_Rect p{ (int)player.x, (int)player.y, (int)player.size, (int)player.size };
    SDL_Rect e{ (int)enemy.x, (int)enemy.y, 50, 50 };
    return Overlaps(p, e);
}

bool FindSpawnPosition(int window_size, int item_size, int max_attempts,
    const std::function<bool(const SDL_Rect&)>& isBlocked,
    int& out_x, int& out_y) {
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        int x = rand() % (window_size - item_size);
        int y = rand() % (window_size - item_size);
        SDL_Rect candidate{ x, y, item_size, item_size };
        if (!isBlocked(candidate)) {
            out_x = x;
            out_y = y;
            return true;
        }
    }
    return false;
}