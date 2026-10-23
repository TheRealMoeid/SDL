#include "SDLHelpers.hpp"
#include "GameLogic.hpp"
#include "Renderer.hpp"

int main() {
    GameContext ctx;
    GameWorld world;
    world.window_size = 700;

    if (SDL__CreateWindow(ctx, "Square Baby", world.window_size, world.window_size) != 0) {
        SDL_Log("Failed to initialize SDL/window: %s", SDL_GetError());
        return 1;
    }

    Game_LoadTextures(ctx, world);
    // Initial round setup - mirrors what Game_ResetState() does internally,
    // kept inline here since it runs once and does not need a texture
    // destroy/reload pass the way an actual restart does.
    world.player.size = 50;
    world.player.x = (world.window_size - (int)world.player.size) / 2;
    world.player.y = (world.window_size - (int)world.player.size) / 2;
    world.player.current_tex = world.player.small_tex;

    Uint64 pasttime = SDL_GetTicks();

    while (true) {

        //==================== 1. PROCESS EVENTS ================================
        Events event = SDL__Events(ctx);
        if (event.exit) {
            break;
        }

        Uint32 deltatime = SDL_GetTicks() - pasttime;
        pasttime = SDL_GetTicks();

        //==================== 2 & 3. UPDATE STATE + DETECT COLLISIONS ==========
        Game_RunFrame(ctx, world, event, deltatime);

        //==================== 4. RENDER =========================================
        Render_Draw(ctx, world);

        //==================== 5. PRESENT ========================================
        SDL_RenderPresent(ctx.renderer);
    }

    SDL__Destroy(ctx);
    return 0;
}