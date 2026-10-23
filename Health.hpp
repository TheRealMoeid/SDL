#pragma once
#include "SDLHelpers.hpp"

struct PlayerState;
struct EnemyState;
struct FireState;

struct HealthState {
    int x = 0;
    int y = 0;
    double size = -1;
    int fairness = 0; // dead code since B2 (flat growth replaced the distance formula); kept pending the owner's open question
    SDL_Texture* texture = nullptr;
};

void Health_SpawnIfNeeded(HealthState& health, const PlayerState& player, const EnemyState& enemy, const FireState& fire, int window_size);
void Health_Shrink(HealthState& health, Uint32 deltatime);
