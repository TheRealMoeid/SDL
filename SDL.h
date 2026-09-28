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

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
clock_t lastRun = clock();

struct Events
{
    bool exit = false;
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool space = false;
    bool pause = false;
} event;


int SDL__CreateWindow(const char* title, int width, int height) {

    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | IMG_INIT_PNG | IMG_INIT_JPG) != 0 || TTF_Init() < 0) {
        printf("Error on SDL Initialization: %s\n", SDL_GetError());
        return 1;
    }

    //remove old windows if exist
    if (renderer != NULL) {
        SDL_DestroyRenderer(renderer);
    }
    if (window != NULL) {
        SDL_DestroyWindow(window);
    }
    

    window = SDL_CreateWindow(title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Error on Creation of Window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create a renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED );
    if (!renderer) {
        printf("Error on Definition of Render Engine: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Set background color
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black
    SDL_RenderClear(renderer);
    return 0;
};

SDL_Color SDL__SetColor(const char* color){

        // Check if the hex string is valid
        if (strlen(color) != 7) {
            printf("Invalid hex color string\n");
            return SDL_Color{ 0,0,0 };
        }

        char tmp[3];
        strncpy(tmp, color +1, 2);
        tmp[2] = '\0';
        int r = strtol(tmp, NULL, 16);

        strncpy(tmp, color + 3, 2);
        tmp[2] = '\0';
        int g = strtol(tmp, NULL, 16);

        strncpy(tmp, color + 5, 2);
        tmp[2] = '\0';
        int b = strtol(tmp, NULL, 16);

        SDL_SetRenderDrawColor(renderer, r, g, b, 255);

        return SDL_Color{ (unsigned char) r, (unsigned char) g, (unsigned char) b };
}

void SDL__DrawPoint(int x, int y, const char* color) {

    SDL__SetColor(color);
    SDL_RenderDrawPoint(renderer,x, y);
}

void SDL__DrawLine(int x1, int y1, int x2, int y2,int width, const char* color) {

    SDL__SetColor(color);
    
    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = sqrt(dx * dx + dy * dy);

    // Normalize the direction vector
    dx /= length;
    dy /= length;

    // Draw multiple lines with small width
    for (int i = -width / 2; i <= width / 2; ++i) {
        int _x = i * dx;
        int _y = - i * dy;
        SDL_RenderDrawLine(renderer, x1+_x, y1+_y, x2+_x, y2+_y);
    }
}

void SDL__DrawRect(int x, int y, int w, int h, int width, const char* color) {

    SDL__SetColor(color);

    for (int i = -width / 2; i <= width / 2; ++i) {

        SDL_Rect rect;
        rect.x = x+i;
        rect.y = y + i;
        rect.w = w - 2*i;
        rect.h = h - 2*i;

        SDL_RenderDrawRect(renderer, &rect);
    }
}

void SDL__FillRect(int x, int y, int w, int h, const char* color) {

    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;

    SDL__SetColor(color);
    SDL_RenderFillRect(renderer, &rect);
}

void SDL__DrawCircle(int cx, int cy, int radius, int width, const char* color)
{
    SDL__SetColor(color);

    for (int y = -(radius+width+1); y <= (radius + width+1); ++y) {
        for (int x = -(radius + width+1); x <= (radius + width+1); ++x) {
            int distance = sqrt(x * x + y * y);
            if (distance >= radius - width / 2 && distance <= radius + width / 2) {
                SDL_RenderDrawPoint(renderer, cx + x, cy + y);
            }
        }
    }
}

void SDL__FillCircle(int cx, int cy, int radius, const char* color) {

    SDL__SetColor(color);

    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radius * radius) {
                SDL_RenderDrawPoint(renderer, cx + x, cy + y);
            }
        }
    }
}

void SDL__DrawText(const char* text, int x,int y, int size, const char* color) {

    // Load a font
    TTF_Font* font = TTF_OpenFont("consola.ttf", size);
    if (!font) {
        printf("Error on Loading Font: %s\n", TTF_GetError());
        return;
    }

    // Create a text surface
    SDL_Surface* surface = TTF_RenderUTF8_Solid(font, text, SDL__SetColor(color));
    if (!surface) {
        printf("Error on Rendering Text: %s\n", TTF_GetError());
        return;
    }

    // Create a texture from the surface
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    int text_width, text_height;
    TTF_SizeUTF8(font, text, &text_width, &text_height);
    SDL_Rect dest_rect = { x, y, text_width, text_height };

    // Render the text within the specified region
    SDL_RenderCopy(renderer, texture, NULL, &dest_rect);
    SDL_DestroyTexture(texture);
    TTF_CloseFont(font);
}

int SDL__DrawImage(const char* path, int x, int y, int w, int h) {

    // Load the image
    SDL_Surface* surface = IMG_Load(path);
    if (surface == NULL) {
        printf("Unable to load image %s! SDL_image Error: %s\n", path, IMG_GetError());
        return 1;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    // Get the image dimensions
    int image_width, image_height;
    SDL_QueryTexture(texture, NULL, NULL, &image_width, &image_height);

    // Define the destination rectangle
    SDL_Rect dst_rect = { x, y, w, h };

    // Render the image
    SDL_RenderCopy(renderer, texture, NULL, &dst_rect);
    //SDL_RenderPresent(renderer);

    // ... (Cleanup as before)

    return 0;
}

void SDL__Clear(const char* color) {
    SDL__SetColor(color); 
    SDL_RenderClear(renderer);
}

Events SDL__Events() {

    SDL_Event sdl_event;

    while (SDL_PollEvent(&sdl_event) > 0)
    {
        if (sdl_event.type == SDL_QUIT)
        {
            event.exit = true;
            return event;
        }
      
        if (sdl_event.type == SDL_KEYDOWN) {

            switch (sdl_event.key.keysym.sym) {
            case SDLK_UP:
                event.up = true;
                break;
            case SDLK_DOWN:
                event.down = true;
                break;
            case SDLK_LEFT:
                event.left = true;
                break;
            case SDLK_RIGHT:
                event.right = true;
                break;
            case SDLK_SPACE:
                event.space = true;
                break;
            case SDLK_p:
                event.pause = true;
            }
        }

        if (sdl_event.type == SDL_KEYUP) {

            switch (sdl_event.key.keysym.sym) {
            case SDLK_UP:
                event.up = false;
                break;
            case SDLK_DOWN:
                event.down = false;
                break;
            case SDLK_LEFT:
                event.left = false;
                break;
            case SDLK_RIGHT:
                event.right = false;
                break;
            case SDLK_SPACE:
                event.space = false;
                break;
            case SDLK_p:
                event.pause = false;
                break;
            
            }

        }
    }

    return event;
}

void SDL__Present(int delay) {

    SDL_RenderPresent(renderer);

    int executeTime = ((double)(clock() - lastRun) / CLOCKS_PER_SEC) * 1000;
    delay -= executeTime;
    if (delay > 0) {
        SDL_Delay(delay);
    }

    lastRun = clock();
}

void SDL__Destroy() {

    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    TTF_Quit();
    SDL_Quit();
}