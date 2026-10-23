#include "Player.hpp"
#include "GameConstants.hpp"

void Player_UpdateSprite(PlayerState& player) {
    // B11: called before the player is drawn this frame, so the sprite
    // matches the current size instead of lagging one frame behind.
    player.current_tex = (player.size > 100) ? player.big_tex : player.small_tex;
}

void Player_Shrink(PlayerState& player, Uint32 deltatime) {
    player.size -= 8.3 * log(player.size) * 0.3 * deltatime / 1000.0;
}

void Player_ClampToWindow(PlayerState& player, int window_size) {
    // B8: clamp after the fact (movement, and growth in GameLogic.cpp)
    // instead of guarding before it, which could still overshoot.
    double max_x = window_size - player.size;
    double max_y = window_size - player.size;
    if (player.x < 0) player.x = 0;
    if (player.x > max_x) player.x = max_x;
    if (player.y < 0) player.y = 0;
    if (player.y > max_y) player.y = max_y;
}

void Player_HandleMovement(PlayerState& player, const Events& event, Uint32 deltatime, int window_size) {
    if (event.up)    player.y -= PLAYER_SPEED * deltatime / 1000.0;
    if (event.right) player.x += PLAYER_SPEED * deltatime / 1000.0;
    if (event.down)  player.y += PLAYER_SPEED * deltatime / 1000.0;
    if (event.left)  player.x -= PLAYER_SPEED * deltatime / 1000.0;

    Player_ClampToWindow(player, window_size);
}
