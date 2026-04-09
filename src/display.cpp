#include <SDL.h>
#include <iostream>
#include "display.h"

struct DisplayContext {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    int width = 0;
    int height = 0;
    bool should_close = false;
};

DisplayContext* create_display(int display_index, const std::string& title) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return nullptr;
    }

    auto* ctx = new DisplayContext();

    // Get display bounds
    int num_displays = SDL_GetNumVideoDisplays();
    if (display_index >= num_displays) {
        std::cerr << "Display index " << display_index << " out of range (0-" << num_displays - 1 << "), using 0\n";
        display_index = 0;
    }

    SDL_Rect bounds;
    SDL_GetDisplayBounds(display_index, &bounds);

    ctx->window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_UNDEFINED_DISPLAY(display_index),
        SDL_WINDOWPOS_UNDEFINED_DISPLAY(display_index),
        bounds.w, bounds.h,
        SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS
    );

    if (!ctx->window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        delete ctx;
        return nullptr;
    }

    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ctx->renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(ctx->window);
        SDL_Quit();
        delete ctx;
        return nullptr;
    }

    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 255);
    SDL_RenderClear(ctx->renderer);
    SDL_RenderPresent(ctx->renderer);

    return ctx;
}

void update_display(DisplayContext* ctx, const void* pixels, int width, int height) {
    if (!ctx || !ctx->renderer) return;

    // Validate dimensions to prevent crashes
    if (width <= 0 || height <= 0) {
        std::cerr << "update_display: invalid dimensions " << width << "x" << height << "\n";
        return;
    }

    // Recreate texture if size changed
    if (!ctx->texture || ctx->width != width || ctx->height != height) {
        if (ctx->texture) SDL_DestroyTexture(ctx->texture);
        ctx->texture = SDL_CreateTexture(
            ctx->renderer,
            SDL_PIXELFORMAT_BGR24,
            SDL_TEXTUREACCESS_STREAMING,
            width, height
        );
        if (!ctx->texture) {
            std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << "\n";
            return;
        }
        ctx->width = width;
        ctx->height = height;
    }

    if (ctx->texture) {
        void* tex_pixels;
        int tex_pitch;
        if (SDL_LockTexture(ctx->texture, nullptr, &tex_pixels, &tex_pitch) == 0) {
            // Copy row by row (source may have different pitch)
            const uint8_t* src = static_cast<const uint8_t*>(pixels);
            uint8_t* dst = static_cast<uint8_t*>(tex_pixels);
            int src_pitch = width * 3;
            for (int y = 0; y < height; y++) {
                memcpy(dst + y * tex_pitch, src + y * src_pitch, src_pitch);
            }
            SDL_UnlockTexture(ctx->texture);
        }

        SDL_RenderClear(ctx->renderer);
        SDL_RenderCopy(ctx->renderer, ctx->texture, nullptr, nullptr);
        SDL_RenderPresent(ctx->renderer);
    }
}

void destroy_display(DisplayContext* ctx) {
    if (!ctx) return;
    if (ctx->texture) SDL_DestroyTexture(ctx->texture);
    if (ctx->renderer) SDL_DestroyRenderer(ctx->renderer);
    if (ctx->window) SDL_DestroyWindow(ctx->window);
    SDL_Quit();
    delete ctx;
}

bool display_should_close(DisplayContext* ctx) {
    return ctx && ctx->should_close;
}

void display_pump_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            // Find context and mark for close
            // This is a simplification; in production you'd use a better event system
        }
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
            // Signal close - handled by main loop
        }
    }
}

void display_draw_text(DisplayContext* ctx, const std::string& text, int x, int y) {
    if (!ctx || !ctx->renderer) return;

    // Simple status indicator: draw a colored rectangle
    // Green = connected, red = no signal, yellow = reconnecting
    SDL_Rect rect = {x, y, 16, 16};

    if (text == "connected") {
        SDL_SetRenderDrawColor(ctx->renderer, 0, 200, 0, 255);
    } else if (text == "no_signal") {
        SDL_SetRenderDrawColor(ctx->renderer, 200, 0, 0, 255);
    } else if (text == "reconnecting") {
        SDL_SetRenderDrawColor(ctx->renderer, 200, 200, 0, 255);
    } else {
        return;
    }

    SDL_RenderFillRect(ctx->renderer, &rect);
}
