#include "SDLHelpers.hpp"   // keep this as the first include (_CRT_SECURE_NO_WARNINGS lives there)
#include "GameConstants.hpp"

int SDL__CreateWindow(GameContext& ctx, const char* title, int width, int height) {

    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("Error on SDL Initialization: %s\n", SDL_GetError());
        return 1;
    }

    if (IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) == 0) {
        printf("Error on SDL_image Initialization: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    if (TTF_Init() < 0) {
        printf("Error on SDL_ttf Initialization: %s\n", TTF_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    //remove old windows if exist
    if (ctx.renderer != NULL) {
        SDL_DestroyRenderer(ctx.renderer);
    }
    if (ctx.window != NULL) {
        SDL_DestroyWindow(ctx.window);
    }


    ctx.window = SDL_CreateWindow(title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN);
    if (!ctx.window) {
        printf("Error on Creation of Window: %s\n", SDL_GetError());
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Create a renderer
    ctx.renderer = SDL_CreateRenderer(ctx.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ctx.renderer) {
        printf("Error on Definition of Render Engine: %s\n", SDL_GetError());
        SDL_DestroyWindow(ctx.window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Set background color
    SDL_SetRenderDrawColor(ctx.renderer, 0, 0, 0, 255); // Black
    SDL_RenderClear(ctx.renderer);
    return 0;
}

SDL_Color SDL__SetColor(GameContext& ctx, const char* color) {

    // Check if the hex string is valid
    if (strlen(color) != 7) {
        printf("Invalid hex color string\n");
        return SDL_Color{ 0,0,0 };
    }

    char tmp[3];
    strncpy(tmp, color + 1, 2);
    tmp[2] = '\0';
    int r = strtol(tmp, NULL, 16);

    strncpy(tmp, color + 3, 2);
    tmp[2] = '\0';
    int g = strtol(tmp, NULL, 16);

    strncpy(tmp, color + 5, 2);
    tmp[2] = '\0';
    int b = strtol(tmp, NULL, 16);

    SDL_SetRenderDrawColor(ctx.renderer, r, g, b, 255);

    return SDL_Color{ (unsigned char)r, (unsigned char)g, (unsigned char)b };
}

void SDL__DrawPoint(GameContext& ctx, int x, int y, const char* color) {

    SDL__SetColor(ctx, color);
    SDL_RenderDrawPoint(ctx.renderer, x, y);
}

void SDL__DrawLine(GameContext& ctx, int x1, int y1, int x2, int y2, int width, const char* color) {

    SDL__SetColor(ctx, color);

    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = sqrt(dx * dx + dy * dy);

    // Normalize the direction vector
    dx /= length;
    dy /= length;

    // Draw multiple lines with small width
    for (int i = -width / 2; i <= width / 2; ++i) {
        int _x = i * dx;
        int _y = -i * dy;
        SDL_RenderDrawLine(ctx.renderer, x1 + _x, y1 + _y, x2 + _x, y2 + _y);
    }
}

void SDL__DrawRect(GameContext& ctx, int x, int y, int w, int h, int width, const char* color) {

    SDL__SetColor(ctx, color);

    for (int i = -width / 2; i <= width / 2; ++i) {

        SDL_Rect rect;
        rect.x = x + i;
        rect.y = y + i;
        rect.w = w - 2 * i;
        rect.h = h - 2 * i;

        SDL_RenderDrawRect(ctx.renderer, &rect);
    }
}

void SDL__FillRect(GameContext& ctx, int x, int y, int w, int h, const char* color) {

    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;

    SDL__SetColor(ctx, color);
    SDL_RenderFillRect(ctx.renderer, &rect);
}

void SDL__DrawCircle(GameContext& ctx, int cx, int cy, int radius, int width, const char* color)
{
    SDL__SetColor(ctx, color);

    for (int y = -(radius + width + 1); y <= (radius + width + 1); ++y) {
        for (int x = -(radius + width + 1); x <= (radius + width + 1); ++x) {
            int distance = sqrt(x * x + y * y);
            if (distance >= radius - width / 2 && distance <= radius + width / 2) {
                SDL_RenderDrawPoint(ctx.renderer, cx + x, cy + y);
            }
        }
    }
}

void SDL__FillCircle(GameContext& ctx, int cx, int cy, int radius, const char* color) {

    SDL__SetColor(ctx, color);

    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radius * radius) {
                SDL_RenderDrawPoint(ctx.renderer, cx + x, cy + y);
            }
        }
    }
}

void SDL__DrawText(GameContext& ctx, const char* text, int x, int y, int size, const char* color) {

    // Load a font
    // consola.ttf (Microsoft Consolas) replaced with an openly licensed
    // font - see CLAUDE.md section 10.
    TTF_Font* font = TTF_OpenFont(ASSET_FONTS_DIR "DejaVuSansMono.ttf", size);
    if (!font) {
        printf("Error on Loading Font: %s\n", TTF_GetError());
        return;
    }

    // Create a text surface
    SDL_Surface* surface = TTF_RenderUTF8_Solid(font, text, SDL__SetColor(ctx, color));
    if (!surface) {
        printf("Error on Rendering Text: %s\n", TTF_GetError());
        return;
    }

    // Create a texture from the surface
    SDL_Texture* texture = SDL_CreateTextureFromSurface(ctx.renderer, surface);
    SDL_FreeSurface(surface);

    int text_width, text_height;
    TTF_SizeUTF8(font, text, &text_width, &text_height);
    SDL_Rect dest_rect = { x, y, text_width, text_height };

    // Render the text within the specified region
    SDL_RenderCopy(ctx.renderer, texture, NULL, &dest_rect);
    SDL_DestroyTexture(texture);
    TTF_CloseFont(font);
}

int SDL__DrawImage(GameContext& ctx, const char* path, int x, int y, int w, int h) {

    // Load the image
    SDL_Surface* surface = IMG_Load(path);
    if (surface == NULL) {
        printf("Unable to load image %s! SDL_image Error: %s\n", path, IMG_GetError());
        return 1;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(ctx.renderer, surface);
    SDL_FreeSurface(surface);

    // Get the image dimensions
    int image_width, image_height;
    SDL_QueryTexture(texture, NULL, NULL, &image_width, &image_height);

    // Define the destination rectangle
    SDL_Rect dst_rect = { x, y, w, h };

    // Render the image
    SDL_RenderCopy(ctx.renderer, texture, NULL, &dst_rect);
    //SDL_RenderPresent(ctx.renderer);

    // ... (Cleanup as before)

    return 0;
}

void SDL__Clear(GameContext& ctx, const char* color) {
    SDL__SetColor(ctx, color);
    SDL_RenderClear(ctx.renderer);
}

Events SDL__Events(GameContext& ctx) {

    SDL_Event sdl_event;

    while (SDL_PollEvent(&sdl_event) > 0)
    {
        if (sdl_event.type == SDL_QUIT)
        {
            ctx.events.exit = true;
            return ctx.events;
        }

        if (sdl_event.type == SDL_KEYDOWN) {

            switch (sdl_event.key.keysym.sym) {
            case SDLK_UP:
                ctx.events.up = true;
                break;
            case SDLK_DOWN:
                ctx.events.down = true;
                break;
            case SDLK_LEFT:
                ctx.events.left = true;
                break;
            case SDLK_RIGHT:
                ctx.events.right = true;
                break;
            case SDLK_SPACE:
                ctx.events.space = true;
                break;
            case SDLK_p:
                ctx.events.pause = true;
            }
        }

        if (sdl_event.type == SDL_KEYUP) {

            switch (sdl_event.key.keysym.sym) {
            case SDLK_UP:
                ctx.events.up = false;
                break;
            case SDLK_DOWN:
                ctx.events.down = false;
                break;
            case SDLK_LEFT:
                ctx.events.left = false;
                break;
            case SDLK_RIGHT:
                ctx.events.right = false;
                break;
            case SDLK_SPACE:
                ctx.events.space = false;
                break;
            case SDLK_p:
                ctx.events.pause = false;
                break;

            }

        }
    }

    return ctx.events;
}

void SDL__Present(GameContext& ctx, int delay) {

    SDL_RenderPresent(ctx.renderer);

    int executeTime = ((double)(clock() - ctx.lastRun) / CLOCKS_PER_SEC) * 1000;
    delay -= executeTime;
    if (delay > 0) {
        SDL_Delay(delay);
    }

    ctx.lastRun = clock();
}

void SDL__Destroy(GameContext& ctx) {

    if (ctx.renderer) {
        SDL_DestroyRenderer(ctx.renderer);
        ctx.renderer = nullptr;
    }
    if (ctx.window) {
        SDL_DestroyWindow(ctx.window);
        ctx.window = nullptr;
    }
    TTF_Quit();
    SDL_Quit();
}