#pragma once

#include <string>

struct DisplayContext;

// Create fullscreen SDL2 window on the given display index.
DisplayContext* create_display(int display_index, const std::string& title);

// Set target framerate (default 30).
void display_set_framerate(DisplayContext* ctx, int fps);

// Update the display with new RGB pixel data.
void update_display(DisplayContext* ctx, const void* pixels, int width, int height);

// Present the frame (handles pacing and rendering). Call each frame in render loop.
void display_present(DisplayContext* ctx);

// Destroy the display context.
void destroy_display(DisplayContext* ctx);

// Check if the window should close (e.g. user pressed Escape or closed window).
bool display_should_close(DisplayContext* ctx);

// Process SDL events (call periodically).
void display_pump_events();

// Draw a text overlay on the display. Call before present.
void display_draw_text(DisplayContext* ctx, const std::string& text, int x, int y);
