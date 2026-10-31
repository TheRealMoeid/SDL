#pragma once
#include "SDLHelpers.hpp"

struct PlayerState;
struct EnemyState;
struct HealthState;

struct FireState {
    int x = 0;
    int y = 0;
    double size = -1;
    bool visible = false;
    SDL_Texture* texture = nullptr;
};
//places and shows the fire at the moment it is revealed, checked
// against the current player/enemy/health positions. Returns false if no
// valid spot was found (fire stays hidden).
bool Fire_Reveal(FireState& fire, const PlayerState& player, const EnemyState& enemy, const HealthState& health, int window_size);
void Fire_Shrink(FireState& fire, Uint32 deltatime);
