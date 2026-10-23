#include "Renderer.hpp"
#include "GameLogic.hpp"
#include <string>

void Render_Draw(GameContext& ctx, const GameWorld& world) {
    SDL_RenderClear(ctx.renderer);
    SDL_RenderCopy(ctx.renderer, world.background_tex, NULL, NULL);

    if (world.state == GameState::Playing) {
        SDL_Rect playerRect = { (int)world.player.x, (int)world.player.y, (int)world.player.size, (int)world.player.size };
        SDL_RenderCopy(ctx.renderer, world.player.current_tex, NULL, &playerRect);

        SDL_Rect enemyRect = { (int)world.enemy.x, (int)world.enemy.y, 50, 50 };
        SDL_RenderCopy(ctx.renderer, world.enemy.texture, NULL, &enemyRect);

        SDL_Rect healthRect = { world.health.x, world.health.y, (int)world.health.size, (int)world.health.size };
        SDL_RenderCopy(ctx.renderer, world.health.texture, NULL, &healthRect);

        if (world.fire.visible) {
            SDL_Rect fireRect = { world.fire.x, world.fire.y, (int)world.fire.size, (int)world.fire.size };
            SDL_RenderCopy(ctx.renderer, world.fire.texture, NULL, &fireRect);
        }

        std::string string_score = "score : " + std::to_string((int)world.score_time);
        SDL__DrawText(ctx, string_score.c_str(), 0, 0, 30, "#FF0000");
    }

    if (world.state == GameState::GameOver) {
        SDL_RenderCopy(ctx.renderer, world.gameover_tex, NULL, NULL);
    }
    if (world.state == GameState::Victory) {
        SDL_RenderCopy(ctx.renderer, world.win_tex, NULL, NULL);
    }
}
