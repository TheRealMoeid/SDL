#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define SDL_MAIN_HANDLED
#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2_ttf.lib")
#pragma comment(lib,"SDL2_image.lib")

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include "sdl/SDL.h"
#include "sdl/SDL_ttf.h"
#include "sdl/SDL_image.h"

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
};

int       SDL__CreateWindow(GameContext& ctx, const char* title, int width, int height);
SDL_Color SDL__SetColor(GameContext& ctx, const char* color);
void      SDL__DrawPoint(GameContext& ctx, int x, int y, const char* color);
void      SDL__DrawLine(GameContext& ctx, int x1, int y1, int x2, int y2, int width, const char* color);
void      SDL__DrawRect(GameContext& ctx, int x, int y, int w, int h, int width, const char* color);
void      SDL__FillRect(GameContext& ctx, int x, int y, int w, int h, const char* color);
void      SDL__DrawCircle(GameContext& ctx, int cx, int cy, int radius, int width, const char* color);
void      SDL__FillCircle(GameContext& ctx, int cx, int cy, int radius, const char* color);
void      SDL__DrawText(GameContext& ctx, const char* text, int x, int y, int size, const char* color);
int       SDL__DrawImage(GameContext& ctx, const char* path, int x, int y, int w, int h);
void      SDL__Clear(GameContext& ctx, const char* color);
Events    SDL__Events(GameContext& ctx);
void      SDL__Present(GameContext& ctx, int delay);
void      SDL__Destroy(GameContext& ctx);