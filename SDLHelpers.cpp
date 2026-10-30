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

    ctx.renderer = SDL_CreateRenderer(ctx.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ctx.renderer) {
        printf("Error on Definition of Render Engine: %s\n", SDL_GetError());
        SDL_DestroyWindow(ctx.window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // B22: load the score font once, here, instead of per-call in
    // SDL__DrawText. Size is fixed at the one value the game actually
    // draws text at (30px, see Renderer.cpp's score text).
    if (ctx.score_font == nullptr) {
        ctx.score_font = TTF_OpenFont(ASSET_FONTS_DIR "DejaVuSansMono.ttf", 30);
        if (!ctx.score_font) {
            printf("Error on Loading Font: %s\n", TTF_GetError());
            SDL_DestroyRenderer(ctx.renderer);
            SDL_DestroyWindow(ctx.window);
            TTF_Quit();
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
    }

    SDL_SetRenderDrawColor(ctx.renderer, 0, 0, 0, 255); // Black
    SDL_RenderClear(ctx.renderer);
    return 0;
}

SDL_Color SDL__SetColor(GameContext& ctx, const char* color) {

    // B24: validate the leading '#' and that all six remaining
    // characters are hex digits, not just the string length.
    if (strlen(color) != 7 || color[0] != '#') {
        printf("Invalid hex color string: %s\n", color);
        return SDL_Color{ 0,0,0 };
    }
    for (int i = 1; i < 7; ++i) {
        if (!isxdigit((unsigned char)color[i])) {
            printf("Invalid hex color string: %s\n", color);
            return SDL_Color{ 0,0,0 };
        }
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

void SDL__DrawText(GameContext& ctx, const char* text, int x, int y, int size, const char* color) {

    // B22: font is loaded once in SDL__CreateWindow and reused here -
    // no more TTF_OpenFont/TTF_CloseFont every call. `size` is accepted
    // for interface compatibility but no longer used to pick a font,
    // since only one size (30px) is ever actually requested.
    if (!ctx.score_font) {
        printf("Error on Rendering Text: score_font not loaded\n");
        return;
    }

    SDL_Surface* surface = TTF_RenderUTF8_Solid(ctx.score_font, text, SDL__SetColor(ctx, color));
    if (!surface) {
        printf("Error on Rendering Text: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(ctx.renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        printf("Error on Creating Text Texture: %s\n", SDL_GetError());
        return;
    }

    int text_width, text_height;
    TTF_SizeUTF8(ctx.score_font, text, &text_width, &text_height);
    SDL_Rect dest_rect = { x, y, text_width, text_height };

    SDL_RenderCopy(ctx.renderer, texture, NULL, &dest_rect);
    SDL_DestroyTexture(texture);
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

    if (ctx.score_font) {
        TTF_CloseFont(ctx.score_font);
        ctx.score_font = nullptr;
    }
    if (ctx.renderer) {
        SDL_DestroyRenderer(ctx.renderer);
        ctx.renderer = nullptr;
    }
    if (ctx.window) {
        SDL_DestroyWindow(ctx.window);
        ctx.window = nullptr;
    }
    TTF_Quit();
    IMG_Quit(); // B23: was never called - IMG_Init'd but never torn down
    SDL_Quit();
}