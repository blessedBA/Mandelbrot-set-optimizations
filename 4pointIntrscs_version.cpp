#include <cmath>
#include <stdio.h>

#include <arm_neon.h>

#include "raylib.h"

#define PPB 4 // points per batch, tied to float32x4_t / NEON register width
static Color escapeColor(const int n, const int max_iter);

typedef float point_t;

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

            const float32x4_t lane_idx = {0.0f, 1.0f, 2.0f, 3.0f};
            const float32x4_t dx_lane = vmulq_f32(lane_idx, vdupq_n_f32(dx));
            const float32x4_t rad_sqr_max_vec = vdupq_n_f32(rad_sqr_max);

            for (int py = 0; py < screen_height; ++py)
            {
                const point_t y0 = y_max - ((point_t)py / (point_t)screen_height) * height;
                const float32x4_t y0_batch = vdupq_n_f32(y0); // initializate a register with a constant value

                for (int px = 0; px < screen_width; px += PPB)
                {
                    const point_t x0_min = x_min + px * width / screen_width;
                    const float32x4_t x0_batch = vaddq_f32(vdupq_n_f32(x0_min), dx_lane);

                    float32x4_t x_batch = vdupq_n_f32(0.0f);
                    float32x4_t y_batch = vdupq_n_f32(0.0f);
                    int32x4_t n_batch = vdupq_n_s32(0);

                    for (int n = 0; n < max_iter; n++)
                    {
                        const float32x4_t x_sqr = vmulq_f32(x_batch, x_batch); // multiplication of 4-float registers
                        const float32x4_t y_sqr = vmulq_f32(y_batch, y_batch);
                        const float32x4_t xy = vmulq_f32(x_batch, y_batch);
                        const float32x4_t rad_sqr = vaddq_f32(x_sqr, y_sqr); // adding of 4-float registers

                        const uint32x4_t cmp = vcleq_f32(rad_sqr, rad_sqr_max_vec); // comparing less or equal 
                        if (vmaxvq_u32(cmp) == 0) break; // return maximum (uint32_t) from 4-float register

                        n_batch = vsubq_s32(n_batch, vreinterpretq_s32_u32(cmp));

                        x_batch = vaddq_f32(vsubq_f32(x_sqr, y_sqr), x0_batch);
                        y_batch = vaddq_f32(vaddq_f32(xy, xy), y0_batch);
                    }

                    alignas(16) int n_lanes[PPB];
                    /* vst1q_s32 — инструкция NEON, которая пишет 128 бит (весь регистр) одной операцией.
                    На ARM невыровненный доступ обычно не падает
                    (в отличие от x86 SSE, где _mm_store_si128 жёстко требует выравнивания и крашится без него),
                    но выровненный write быстрее и это общепринятая практика при работе с SIMD-буферами —
                    компилятор может сгенерировать более простую/быструю инструкцию, зная, что адрес точно выровнен */
                    vst1q_s32(n_lanes, n_batch);

                    for (int lane = 0; lane < PPB; ++lane)
                    {
                        const int index = py * screen_width + (px + lane);
                        const int n = n_lanes[lane];
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
