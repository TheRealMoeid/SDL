#pragma once
#include "SDLHelpers.hpp"

struct PlayerState {
    double size = 50;
    double x = 0;
    double y = 0;
    SDL_Texture* small_tex = nullptr;   // was A_Texture
    SDL_Texture* big_tex = nullptr;     // was B_Texture
    SDL_Texture* current_tex = nullptr; // was C_Texture - alias, never owns a texture (B1)
};

void Player_UpdateSprite(PlayerState& player);
void Player_Shrink(PlayerState& player, Uint32 deltatime);
void Player_HandleMovement(PlayerState& player, const Events& event, Uint32 deltatime, int window_size);
void Player_ClampToWindow(PlayerState& player, int window_size);
