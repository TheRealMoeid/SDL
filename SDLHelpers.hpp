#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define SDL_MAIN_HANDLED


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

struct Events
{
    bool exit = false;
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool space = false;
    bool pause = false;
};

struct GameContext
{
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    Events        events;                // key state has to persist between calls
    clock_t       lastRun = clock();    // only SDL__Present uses it
    TTF_Font* score_font = nullptr;  // B22: loaded once, reused every SDL__DrawText call
};

int       SDL__CreateWindow(GameContext& ctx, const char* title, int width, int height);
SDL_Color SDL__SetColor(GameContext& ctx, const char* color);
void      SDL__DrawText(GameContext& ctx, const char* text, int x, int y, int size, const char* color);
void      SDL__Clear(GameContext& ctx, const char* color);
Events    SDL__Events(GameContext& ctx);
void      SDL__Present(GameContext& ctx, int delay);
void      SDL__Destroy(GameContext& ctx);