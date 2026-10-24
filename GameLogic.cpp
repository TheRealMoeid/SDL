#include "GameLogic.hpp"
#include "GameConstants.hpp"
#include "Collision.hpp"
#include <cstdio>
#include "GameConstants.hpp"

static SDL_Texture* LoadTextureChecked(SDL_Renderer* ren, const char* path) {
    SDL_Texture* tex = IMG_LoadTexture(ren, path);
    if (!tex) {
        SDL_Log("Failed to load texture '%s': %s", path, IMG_GetError());
    }
    return tex;
}

void Game_LoadTextures(GameContext& ctx, GameWorld& world) {
    world.player.small_tex = LoadTextureChecked(ctx.renderer, ASSET_IMAGES_DIR "BacheGherti.png");
    world.player.big_tex = LoadTextureChecked(ctx.renderer, ASSET_IMAGES_DIR "Hestler.png");
    world.player.current_tex = world.player.small_tex;
    world.health.texture = LoadTextureChecked(ctx.renderer, ASSET_IMAGES_DIR "Health.png");
    world.fire.texture = LoadTextureChecked(ctx.renderer, ASSET_IMAGES_DIR "Fire.png");
    world.enemy.texture = LoadTextureChecked(ctx.renderer, ASSET_IMAGES_DIR "Enemy.png");
    world.background_tex = LoadTextureChecked(ctx.renderer, ASSET_IMAGES_DIR "background.png");
    world.gameover_tex = LoadTextureChecked(ctx.renderer, ASSET_IMAGES_DIR "GameOver.png");
    world.win_tex = LoadTextureChecked(ctx.renderer, ASSET_IMAGES_DIR "Win.png");
}

static void Game_DestroyTextures(GameWorld& world) {
    SDL_DestroyTexture(world.player.small_tex);
    SDL_DestroyTexture(world.player.big_tex);
    SDL_DestroyTexture(world.health.texture);
    SDL_DestroyTexture(world.fire.texture);
    SDL_DestroyTexture(world.enemy.texture);
    SDL_DestroyTexture(world.background_tex);
    SDL_DestroyTexture(world.gameover_tex);
    SDL_DestroyTexture(world.win_tex);
}

static void Game_ResetState(GameWorld& world) {
    world.state = GameState::Playing;
    world.player.size = 50;
    world.player.x = (world.window_size - (int)world.player.size) / 2;
    world.player.y = (world.window_size - (int)world.player.size) / 2;
    world.player.current_tex = world.player.small_tex;
    world.health.size = -1;
    world.fire.size = -1;
    world.fire.visible = false;
    world.enemy.x = 100;
    world.enemy.y = 100;
    world.enemy.direction = 0;
    world.enemy.direction_timer = 0.0;
    world.score_time = 0;
    world.health_counter = 0;
}

void Game_Reset(GameContext& ctx, GameWorld& world) {
    Game_DestroyTextures(world);
    Game_LoadTextures(ctx, world);
    Game_ResetState(world);
}

void Game_RunFrame(GameContext& ctx, GameWorld& world, const Events& event, Uint32 deltatime) {

    //-------------------- Player shrink -------------------------------
    if (world.state == GameState::Playing) {
        Player_Shrink(world.player, deltatime);
    }

    // B11: pick this frame's sprite before drawing it, not after.
    Player_UpdateSprite(world.player);

    //-------------------- Enemy movement -------------------------------
    // B9: stays still outside Playing instead of wandering behind the
    // ending overlay; re-rolls immediately when blocked (see Enemy.cpp).
    if (world.state == GameState::Playing) {
        Enemy_Update(world.enemy, deltatime, world.window_size);
    }

    //-------------------- Health item spawn/shrink ----------------------
    Health_SpawnIfNeeded(world.health, world.player, world.enemy, world.fire, world.window_size);
    if (world.state == GameState::Playing) {
        Health_Shrink(world.health, deltatime);
    }

    //-------------------- Fire item spawn/shrink -------------------------
    Fire_SpawnIfNeeded(world.fire, world.player, world.enemy, world.health, world.window_size);
    Fire_Shrink(world.fire, deltatime); // not gated by Playing - see Fire.cpp

    //-------------------- Player movement --------------------------------
    if (world.state == GameState::Playing) {
        Player_HandleMovement(world.player, event, deltatime, world.window_size);
    }

    //==================== DETECT COLLISIONS ===============================
    if (CollidedWithHealth(world.player, world.health) && (world.player.size < (world.window_size - 200))) {
        world.player.size += HEALTH_GROWTH;
        world.health.size = -1;
        Player_ClampToWindow(world.player, world.window_size);

        if (!world.fire.visible) {
            world.health_counter++;
            if (world.health_counter >= 3) {
                world.fire.visible = true;
                world.fire.size = 50;
                world.health_counter = 0;
            }
        }
    }

    if (CollidedWithFire(world.player, world.fire) && world.fire.visible && (world.player.size < (world.window_size - 200))) {
        world.player.size -= FIRE_PENALTY;
        world.fire.size = -1;
        world.fire.visible = false;
    }

    if (CollidedWithEnemy(world.player, world.enemy) && (world.player.size < (world.window_size - 200))) {
        world.state = GameState::GameOver;
    }

    if (world.state == GameState::Playing) {
        world.score_time += deltatime / 1000.0;
    }
    if (world.player.size < 3) {
        world.state = GameState::GameOver;
    }
    if (world.player.size >= (world.window_size - 200)) {
        world.state = GameState::Victory;
    }

    // Restart: still a state update, so it runs before render - the new
    // round's very first frame renders correctly rather than one frame
    // behind the reset.
    if ((world.state == GameState::GameOver || world.state == GameState::Victory) && event.space) {
        Game_Reset(ctx, world);
    }
}
