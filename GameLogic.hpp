#pragma once
#include "SDLHelpers.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "Health.hpp"
#include "Fire.hpp"

// B10: explicit game state, replacing the two independent gameover/winner
// booleans that nothing kept from both being true at once.
enum class GameState {
    Playing,
    GameOver,
    Victory
};

struct GameWorld {
    PlayerState player;
    EnemyState  enemy;
    HealthState health;
    FireState   fire;
    GameState   state = GameState::Playing;
    double      score_time = 0.0;
    int         health_counter = 0;
    int         window_size = 700;
    SDL_Texture* background_tex = nullptr;
    SDL_Texture* gameover_tex = nullptr;
    SDL_Texture* win_tex = nullptr;
};

// Loads every texture; used both at startup and after a reset.
void Game_LoadTextures(GameContext& ctx, GameWorld& world);

// B21: Game_Reset no longer touches textures - it only resets round
// state now. Textures are loaded once by Game_LoadTextures, called
// only from game.cpp at startup.
void Game_Reset(GameContext& ctx, GameWorld& world);

void Game_RunFrame(GameContext& ctx, GameWorld& world, const Events& event, Uint32 deltatime);
