#include "Collision.hpp"
#include "Player.hpp"
#include "Health.hpp"
#include "Fire.hpp"
#include "Enemy.hpp"

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
