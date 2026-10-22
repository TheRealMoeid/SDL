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

// Globals: defined in SDLHelpers.cpp (removed in Commit B)
extern SDL_Window* window;
extern SDL_Renderer* renderer;
extern Events event;

int       SDL__CreateWindow(const char* title, int width, int height);
SDL_Color SDL__SetColor(const char* color);
void      SDL__DrawPoint(int x, int y, const char* color);
void      SDL__DrawLine(int x1, int y1, int x2, int y2, int width, const char* color);
void      SDL__DrawRect(int x, int y, int w, int h, int width, const char* color);
void      SDL__FillRect(int x, int y, int w, int h, const char* color);
void      SDL__DrawCircle(int cx, int cy, int radius, int width, const char* color);
void      SDL__FillCircle(int cx, int cy, int radius, const char* color);
void      SDL__DrawText(const char* text, int x, int y, int size, const char* color);
int       SDL__DrawImage(const char* path, int x, int y, int w, int h);
void      SDL__Clear(const char* color);
Events    SDL__Events();
void      SDL__Present(int delay);
void      SDL__Destroy();