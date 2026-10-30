#include "Player.hpp"
#include "GameConstants.hpp"

void Player_UpdateSprite(PlayerState& player) {
    player.current_tex = (player.size > 100) ? player.big_tex : player.small_tex;
}

void Player_Shrink(PlayerState& player, Uint32 deltatime) {
    player.size -= 8.3 * log(player.size) * 0.3 * deltatime / 1000.0;
}

void Player_ClampToWindow(PlayerState& player, int window_size) {
    double max_x = window_size - player.size;
    double max_y = window_size - player.size;
    if (player.x < 0) player.x = 0;
    if (player.x > max_x) player.x = max_x;
    if (player.y < 0) player.y = 0;
    if (player.y > max_y) player.y = max_y;
}

void Player_HandleMovement(PlayerState& player, const Events& event, Uint32 deltatime, int window_size) {
    // B19: build a direction vector and normalize it instead of scaling
    // each axis independently - the old code gave diagonal movement
    // ~180*sqrt(2) ≈ 254 px/s instead of the intended 180 px/s. Opposite
    // keys still cancel exactly as before (both up+down held -> dy=0).
    double dx = 0;
    double dy = 0;
    if (event.up)    dy -= 1;
    if (event.down)  dy += 1;
    if (event.left)  dx -= 1;
    if (event.right) dx += 1;

    if (dx != 0 || dy != 0) {
        double length = sqrt(dx * dx + dy * dy);
        dx /= length;
        dy /= length;

        player.x += dx * PLAYER_SPEED * deltatime / 1000.0;
        player.y += dy * PLAYER_SPEED * deltatime / 1000.0;
    }

    Player_ClampToWindow(player, window_size);
}