#include <cmath>

#include "raylib.h"

static Color escapeColor(const int n, const int max_iter);

int main()
{
    const int screen_width = 1500;
    const int screen_height = 700;
    const int max_iter = 256;

    InitWindow(screen_width, screen_height, "Mandelbrot set");
    SetTargetFPS(120);

    double x_min = -2.5;
    double x_max = 1.0;
    double y_min = -1.2;
    double y_max = 1.2;

    Image image = GenImageColor(screen_width, screen_height, BLACK);
    Texture2D texture = LoadTextureFromImage(image);

    bool view_dirty = true;

    while (!WindowShouldClose())
    {
        const double width_before_input = x_max - x_min;
        const double height_before_input = y_max - y_min;

        if (IsKeyPressed(KEY_R))
        {
            x_min = -2.5;
            x_max = 1.0;
            y_min = -1.2;
            y_max = 1.2;
            view_dirty = true;
        }

        const bool zoom_in_key_held = IsKeyDown(KEY_EQUAL) || IsKeyDown(KEY_KP_ADD);
        const bool zoom_out_key_held = IsKeyDown(KEY_MINUS) || IsKeyDown(KEY_KP_SUBTRACT);
        if (zoom_in_key_held || zoom_out_key_held)
        {
            const double rel_x = 0.5;
            const double rel_y = 0.5;
            const double c_x = x_min + rel_x * width_before_input;
            const double c_y = y_max - rel_y * height_before_input;
            const double zoom_factor = zoom_in_key_held ? 0.97 : 1.03;
            const double new_width = width_before_input * zoom_factor;
            const double new_height = height_before_input * zoom_factor;

            x_min = c_x - rel_x * new_width;
            x_max = x_min + new_width;
            y_max = c_y + rel_y * new_height;
            y_min = y_max - new_height;
            view_dirty = true;
        }

        const double pan_step = 0.015;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
        {
            const double dx = width_before_input * pan_step;
            x_min -= dx;
            x_max -= dx;
            view_dirty = true;
        }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        {
            const double dx = width_before_input * pan_step;
            x_min += dx;
            x_max += dx;
            view_dirty = true;
        }
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
        {
            const double dy = height_before_input * pan_step;
            y_min += dy;
            y_max += dy;
            view_dirty = true;
        }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
        {
            const double dy = height_before_input * pan_step;
            y_min -= dy;
            y_max -= dy;
            view_dirty = true;
        }

        if (view_dirty)
        {
            Color *pixels = (Color *)image.data;
            const double width = x_max - x_min;
            const double height = y_max - y_min;

            for (int py = 0; py < screen_height; ++py)
            {
                const double y0 = y_max - ((double)py / (double)screen_height) * height;
                for (int px = 0; px < screen_width; ++px)
                {
                    const double x0 = x_min + ((double)px / (double)screen_width) * width;

                    double x = 0.0;
                    double y = 0.0;
                    int n = 0;
                    while ((x * x + y * y <= 4.0) && (n < max_iter))
                    {
                        const double next_x = x * x - y * y + x0;
                        const double next_y = 2.0 * x * y + y0;
                        x = next_x;
                        y = next_y;
                        ++n;
                    }

                    const int index = py * screen_width + px;
                    pixels[index] = (n == max_iter) ? BLACK : escapeColor(n, max_iter);
                }
            }

            UpdateTexture(texture, pixels);
            view_dirty = false;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        DrawTexture(texture, 0, 0, WHITE);

        if (x_min < 0.0 && x_max > 0.0)
        {
            const int axis_x = (int)std::lround((0.0 - x_min) * (double)screen_width / (x_max - x_min));
            DrawLine(axis_x, 0, axis_x, screen_height, Fade(WHITE, 0.20f));
        }
        if (y_min < 0.0 && y_max > 0.0)
        {
            const int axis_y = (int)std::lround((y_max - 0.0) * (double)screen_height / (y_max - y_min));
            DrawLine(0, axis_y, screen_width, axis_y, Fade(WHITE, 0.20f));
        }

        DrawRectangle(10, 10, 780, 70, Fade(BLACK, 0.65f));
        DrawText("Wheel or +/-: zoom, LMB drag or WASD/arrows: pan, R: reset", 20, 20, 20, RAYWHITE);
        DrawText(TextFormat("x:[%.10f, %.10f]  y:[%.10f, %.10f]", x_min, x_max, y_min, y_max), 20, 44, 16, RAYWHITE);
        DrawFPS(screen_width - 110, 16);

        EndDrawing();
#ifdef TEST
        CloseWindow();
#endif
    }

    UnloadTexture(texture);
    UnloadImage(image);
#ifdef RELEASE
    CloseWindow();
#endif
    return 0;
}

static Color escapeColor(const int n, const int max_iter)
{
    const float t = (max_iter > 0) ? ((float)n / (float)max_iter) : 0.0f;
    const unsigned char r = (unsigned char)(9.0f * (1.0f - t) * t * t * t * 255.0f);
    const unsigned char g = (unsigned char)(15.0f * (1.0f - t) * (1.0f - t) * t * t * 255.0f);
    const unsigned char b = (unsigned char)(8.5f * (1.0f - t) * (1.0f - t) * (1.0f - t) * t * 255.0f);
    return (Color){r, g, b, 255};
}
