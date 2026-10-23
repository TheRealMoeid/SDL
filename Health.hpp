#pragma once
#include "SDLHelpers.hpp"

struct PlayerState;
struct EnemyState;
struct FireState;

struct HealthState {
    int x = 0;
    int y = 0;
    double size = -1;
    SDL_Texture* texture = nullptr;
};

void Health_SpawnIfNeeded(HealthState& health, const PlayerState& player, const EnemyState& enemy, const FireState& fire, int window_size);
void Health_Shrink(HealthState& health, Uint32 deltatime);
