#pragma once
#include "SDLHelpers.hpp"

struct HealthState;
struct FireState;

struct EnemyState {
    double x = 100;
    double y = 100;
    int direction = 0;
    double direction_timer = 0.0; // seconds since last direction change
    SDL_Texture* texture = nullptr;
};

void Enemy_Update(EnemyState& enemy, const HealthState& health, const FireState& fire, Uint32 deltatime, int window_size);