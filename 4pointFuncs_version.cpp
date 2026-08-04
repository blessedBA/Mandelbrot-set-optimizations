#include <cmath>
#include <stdio.h>

#include "raylib.h"

#define PPB 4 // points per batch 
static Color escapeColor(const int n, const int max_iter);

typedef float point_t;

static void fl_mul (point_t* __restrict dst, point_t* __restrict src1, point_t* __restrict src2);
static void fl_add (point_t* __restrict dst, point_t* __restrict src1, point_t* __restrict src2);
static void fl_sub (point_t* __restrict dst, point_t* __restrict src1, point_t* __restrict src2);
static void int_sub (int* __restrict dst, int* __restrict src1, int* __restrict src2);
static void int_add (int* __restrict dst, int* __restrict src1, int* __restrict src2);

int main()
{
    const int screen_width = 1500;
    const int screen_height = 700;
    const int max_iter = 256;
    const point_t rad_sqr_max = 4.0;

    InitWindow(screen_width, screen_height, "Mandelbrot set");
    SetTargetFPS(120);

    point_t x_min = (float)-2.5;
    point_t x_max = (float)1.0;
    point_t y_min = (float)-1.2;
    point_t y_max = (float)1.2;

    Image image = GenImageColor(screen_width, screen_height, BLACK);
    Texture2D texture = LoadTextureFromImage(image);

    bool view_dirty = true;

    while (!WindowShouldClose())
    {
        const point_t width_before_input = x_max - x_min;
        const point_t height_before_input = y_max - y_min;

        if (IsKeyPressed(KEY_R))
        {
            x_min = (float)-2.5;
            x_max = (float)1.0;
            y_min = (float)-1.2;
            y_max = (float)1.2;
            view_dirty = true;
        }

        const bool zoom_in_key_held = IsKeyDown(KEY_EQUAL) || IsKeyDown(KEY_KP_ADD);
        const bool zoom_out_key_held = IsKeyDown(KEY_MINUS) || IsKeyDown(KEY_KP_SUBTRACT);
        if (zoom_in_key_held || zoom_out_key_held)
        {
            const point_t rel_x = 0.5;
            const point_t rel_y = 0.5;
            const point_t c_x = x_min + rel_x * width_before_input;
            const point_t c_y = y_max - rel_y * height_before_input;
            const point_t zoom_factor = zoom_in_key_held ? (float)0.97 : (float)1.03;
            const point_t new_width = width_before_input * zoom_factor;
            const point_t new_height = height_before_input * zoom_factor;

            x_min = c_x - rel_x * new_width;
            x_max = x_min + new_width;
            y_max = c_y + rel_y * new_height;
            y_min = y_max - new_height;
            view_dirty = true;
        }

        const point_t pan_step = (float)0.015;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
        {
            const point_t dx = width_before_input * pan_step;
            x_min -= dx;
            x_max -= dx;
            view_dirty = true;
        }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        {
            const point_t dx = width_before_input * pan_step;
            x_min += dx;
            x_max += dx;
            view_dirty = true;
        }
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
        {
            const point_t dy = height_before_input * pan_step;
            y_min += dy;
            y_max += dy;
            view_dirty = true;
        }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
        {
            const point_t dy = height_before_input * pan_step;
            y_min -= dy;
            y_max -= dy;
            view_dirty = true;
        }

        if (view_dirty)
        {
            Color *pixels = (Color *)image.data;
            const point_t width = x_max - x_min;
            const point_t height = y_max - y_min;
            const point_t dx = width / screen_width;
            int mask = 0;

            for (int py = 0; py < screen_height; ++py)
            {
                const point_t y0 = y_max - ((point_t)py / (point_t)screen_height) * height;
                for (int px = 0; px < screen_width; px += PPB)
                {
                    mask = 0;
                    point_t x0_batch[PPB] = {0.0, 0.0, 0.0, 0.0};
                    point_t y0_batch[PPB] = {y0,  y0,  y0,  y0 };
                    point_t x_batch[PPB] = {0.0, 0.0, 0.0, 0.0};
                    point_t y_batch[PPB] = {0.0, 0.0, 0.0, 0.0};
                    int n_batch[PPB] = {0, 0, 0, 0};
                    // bool active_batch[PPB] = {false, false, false, false};
                    const point_t x0_min = x_min + px * width / screen_width;

                    for (int n = 0; n < max_iter; n++)
                    {

                        for (int lane = 0; lane < PPB; ++lane)
                            x0_batch[lane] = x0_min + lane * dx;

                        point_t x_batch_sqr[PPB] = {};
                        point_t y_batch_sqr[PPB] = {};
                        point_t xy_batch[PPB] = {};
                        point_t rad_sqr[PPB] = {};

                        fl_mul (x_batch_sqr, x_batch, x_batch);
                        fl_mul (y_batch_sqr, y_batch, y_batch);
                        fl_mul (xy_batch, x_batch, y_batch);
                        fl_add (rad_sqr, x_batch_sqr, y_batch_sqr);
                            
                        int cmp[PPB] = {};
                        for (int lane = 0; lane < PPB; lane++)
                            if (rad_sqr[lane] <= rad_sqr_max) cmp[lane] = 1;

                        for (int lane = 0; lane < PPB; lane++)
                            mask |= (cmp[lane] << lane);
                        if (!mask) break;
                            
                        int_add (n_batch, n_batch, cmp);

                        fl_sub (x_batch, x_batch_sqr, y_batch_sqr);
                        fl_add (x_batch, x_batch, x0_batch);
                        fl_add (y_batch, xy_batch, xy_batch);
                        fl_add (y_batch, y_batch, y0_batch);
                    }

                    for (int lane = 0; lane < PPB; ++lane)
                    {
                        const int index = py * screen_width + (px + lane);
                        const int n = n_batch[lane];
                        // printf("n in cycle of painting = %d\n", n);
                        pixels[index] = (n >= max_iter) ? BLACK : escapeColor(n, max_iter);
                    }
                }
            }

            UpdateTexture(texture, pixels);
           // view_dirty = false;       for calculating performance programm shouldn't stop painting while nothing is happened
        }

        BeginDrawing();
        ClearBackground(BLACK);

        DrawTexture(texture, 0, 0, WHITE);

        if (x_min < 0.0 && x_max > 0.0)
        {
            const int axis_x = (int)std::lround((0.0 - x_min) * (point_t)screen_width / (x_max - x_min));
            DrawLine(axis_x, 0, axis_x, screen_height, Fade(WHITE, 0.20f));
        }
        if (y_min < 0.0 && y_max > 0.0)
        {
            const int axis_y = (int)std::lround((y_max - 0.0) * (point_t)screen_height / (y_max - y_min));
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

static void fl_mul (point_t* __restrict dst, point_t* __restrict src1, point_t* __restrict src2)
{
    for (int lane = 0; lane < PPB; lane++)
    {
        dst[lane] = src1[lane] * src2[lane];
    }    
}

static void fl_add (point_t* __restrict dst, point_t* __restrict src1, point_t* __restrict src2)
{
    for (int lane = 0; lane < PPB; lane++)
    {
        dst[lane] = src1[lane] + src2[lane];
    }
}

static void fl_sub (point_t* __restrict dst, point_t* __restrict src1, point_t* __restrict src2)
{
    for (int lane = 0; lane < PPB; lane++)
    {
        dst[lane] = src1[lane] - src2[lane];
    }
}

static void int_sub (int* __restrict dst, int* __restrict src1, int* __restrict src2)
{
    for (int lane = 0; lane < PPB; lane++)
    {
        dst[lane] = src1[lane] - src2[lane];
    }
}

static void int_add (int* __restrict dst, int* __restrict src1, int* __restrict src2)
{
    for (int lane = 0; lane < PPB; lane++)
    {
        dst[lane] = src1[lane] + src2[lane];
    }
}