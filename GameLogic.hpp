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

// B3: single source of truth for "start a fresh round" - destroys and
// reloads every texture, then resets every piece of round state,
// including health_counter, fire.visible, enemy.direction and
// enemy.direction_timer, which the original restart blocks missed.
void Game_Reset(GameContext& ctx, GameWorld& world);

// Runs one frame's worth of state update and collision detection
// (stages 2 and 3). Rendering (stage 4) is Renderer.hpp's job.
void Game_RunFrame(GameContext& ctx, GameWorld& world, const Events& event, Uint32 deltatime);
