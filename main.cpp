#include <cmath>

#include "raylib.h"

static Color escapeColor(const int n, const int maxIter);

int main()
{
    const int screenWidth = 1500;
    const int screenHeight = 700;
    const int maxIter = 256;

    InitWindow(screenWidth, screenHeight, "Mandelbrot set");
    SetTargetFPS(120);

    double xMin = -2.5;
    double xMax = 1.0;
    double yMin = -1.2;
    double yMax = 1.2;

    Image image = GenImageColor(screenWidth, screenHeight, BLACK);
    Texture2D texture = LoadTextureFromImage(image);

    bool viewDirty = true;

    while (!WindowShouldClose())
    {
        const double widthBeforeInput = xMax - xMin;
        const double heightBeforeInput = yMax - yMin;

        if (IsKeyPressed(KEY_R))
        {
            xMin = -2.5;
            xMax = 1.0;
            yMin = -1.2;
            yMax = 1.2;
            viewDirty = true;
        }

        const bool zoomInKeyHeld = IsKeyDown(KEY_EQUAL) || IsKeyDown(KEY_KP_ADD);
        const bool zoomOutKeyHeld = IsKeyDown(KEY_MINUS) || IsKeyDown(KEY_KP_SUBTRACT);
        if (zoomInKeyHeld || zoomOutKeyHeld)
        {
            const double relX = 0.5;
            const double relY = 0.5;
            const double cX = xMin + relX * widthBeforeInput;
            const double cY = yMax - relY * heightBeforeInput;
            const double zoomFactor = zoomInKeyHeld ? 0.97 : 1.03;
            const double newWidth = widthBeforeInput * zoomFactor;
            const double newHeight = heightBeforeInput * zoomFactor;

            xMin = cX - relX * newWidth;
            xMax = xMin + newWidth;
            yMax = cY + relY * newHeight;
            yMin = yMax - newHeight;
            viewDirty = true;
        }

        const double panStep = 0.015;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
        {
            const double dx = widthBeforeInput * panStep;
            xMin -= dx;
            xMax -= dx;
            viewDirty = true;
        }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        {
            const double dx = widthBeforeInput * panStep;
            xMin += dx;
            xMax += dx;
            viewDirty = true;
        }
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
        {
            const double dy = heightBeforeInput * panStep;
            yMin += dy;
            yMax += dy;
            viewDirty = true;
        }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
        {
            const double dy = heightBeforeInput * panStep;
            yMin -= dy;
            yMax -= dy;
            viewDirty = true;
        }

        if (viewDirty)
        {
            Color *pixels = (Color *)image.data;
            const double width = xMax - xMin;
            const double height = yMax - yMin;

            for (int py = 0; py < screenHeight; ++py)
            {
                const double y0 = yMax - ((double)py / (double)screenHeight) * height;
                for (int px = 0; px < screenWidth; ++px)
                {
                    const double x0 = xMin + ((double)px / (double)screenWidth) * width;

                    double x = 0.0;
                    double y = 0.0;
                    int n = 0;
                    while ((x * x + y * y <= 4.0) && (n < maxIter))
                    {
                        const double nextX = x * x - y * y + x0;
                        const double nextY = 2.0 * x * y + y0;
                        x = nextX;
                        y = nextY;
                        ++n;
                    }

                    const int index = py * screenWidth + px;
                    pixels[index] = (n == maxIter) ? BLACK : escapeColor(n, maxIter);
                }
            }

            UpdateTexture(texture, pixels);
            viewDirty = false;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        DrawTexture(texture, 0, 0, WHITE);

        if (xMin < 0.0 && xMax > 0.0)
        {
            const int axisX = (int)std::lround((0.0 - xMin) * (double)screenWidth / (xMax - xMin));
            DrawLine(axisX, 0, axisX, screenHeight, Fade(WHITE, 0.20f));
        }
        if (yMin < 0.0 && yMax > 0.0)
        {
            const int axisY = (int)std::lround((yMax - 0.0) * (double)screenHeight / (yMax - yMin));
            DrawLine(0, axisY, screenWidth, axisY, Fade(WHITE, 0.20f));
        }

        DrawRectangle(10, 10, 780, 70, Fade(BLACK, 0.65f));
        DrawText("Wheel or +/-: zoom, LMB drag or WASD/arrows: pan, R: reset", 20, 20, 20, RAYWHITE);
        DrawText(TextFormat("x:[%.10f, %.10f]  y:[%.10f, %.10f]", xMin, xMax, yMin, yMax), 20, 44, 16, RAYWHITE);
        DrawFPS(screenWidth - 110, 16);

        EndDrawing();
    }

    UnloadTexture(texture);
    UnloadImage(image);
    CloseWindow();
    return 0;
}

static Color escapeColor(const int n, const int maxIter)
{
    const float t = (maxIter > 0) ? ((float)n / (float)maxIter) : 0.0f;
    const unsigned char r = (unsigned char)(9.0f * (1.0f - t) * t * t * t * 255.0f);
    const unsigned char g = (unsigned char)(15.0f * (1.0f - t) * (1.0f - t) * t * t * 255.0f);
    const unsigned char b = (unsigned char)(8.5f * (1.0f - t) * (1.0f - t) * (1.0f - t) * t * 255.0f);
    return (Color){r, g, b, 255};
}
