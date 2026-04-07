#include "raylib.h"

int main(void) {
    InitWindow(1000, 700, "Medical Management System");
    SetTargetFPS(60);


    int codepoints[] = { 32, 33, 44, 45, 46, 48, 57, 65, 90, 97, 122 }; // ASCII 英文+数字
    Font font = LoadFontEx("AlibabaPuHuiTi-3-115-Black.ttf", 28, codepoints, 12);

    int current_page = 0;

    while (!WindowShouldClose()) {
        // 数字键切换页面
        if (IsKeyPressed(KEY_ONE))      current_page = 1;
        if (IsKeyPressed(KEY_TWO))      current_page = 2;
        if (IsKeyPressed(KEY_THREE))    current_page = 3;
        if (IsKeyPressed(KEY_FOUR))     current_page = 4;
        if (IsKeyPressed(KEY_FIVE))     current_page = 5;
        if (IsKeyPressed(KEY_ZERO))     break;

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // 顶部标题栏
        DrawRectangle(0, 0, 1000, 80, DARKBLUE);
        DrawTextEx(font, "Medical System", (Vector2){360, 20}, 42, 2, WHITE);

        DrawRectangle(0, 80, 200, 620, LIGHTGRAY);
        DrawTextEx(font, "[1] Patient",       (Vector2){25, 100}, 24, 1, BLACK);
        DrawTextEx(font, "[2] Doctor",        (Vector2){25, 150}, 24, 1, BLACK);
        DrawTextEx(font, "[3] Register",      (Vector2){25, 200}, 24, 1, BLACK);
        DrawTextEx(font, "[4] Inpatient",     (Vector2){25, 250}, 24, 1, BLACK);
        DrawTextEx(font, "[5] Pharmacy",      (Vector2){25, 300}, 24, 1, BLACK);
        DrawTextEx(font, "[0] Exit",          (Vector2){25, 550}, 24, 1, RED);

        // 右侧内容框
        DrawRectangleLines(220, 100, 750, 550, GRAY);

        // 页面内容
        switch (current_page) {
            case 1:
                DrawTextEx(font, "Patient List", (Vector2){260, 120}, 32, 1, DARKBLUE);
                DrawTextEx(font, "ID | Name | Age", (Vector2){260, 180}, 24, 1, BLACK);
                DrawTextEx(font, "1  | Zhang | 25", (Vector2){260, 220}, 24, 1, BLACK);
                DrawTextEx(font, "2  | Li    | 30", (Vector2){260, 260}, 24, 1, BLACK);
                break;
            case 2:
                DrawTextEx(font, "Doctor List", (Vector2){260, 120}, 32, 1, DARKBLUE);
                break;
            case 3:
                DrawTextEx(font, "Registration", (Vector2){260, 120}, 32, 1, DARKBLUE);
                break;
            case 4:
                DrawTextEx(font, "Inpatient Manage", (Vector2){260, 120}, 32, 1, DARKBLUE);
                break;
            case 5:
                DrawTextEx(font, "Pharmacy Manage", (Vector2){260, 120}, 32, 1, DARKBLUE);
                break;
            default:
                DrawTextEx(font, "Home", (Vector2){260, 120}, 32, 1, DARKBLUE);
                DrawTextEx(font, "Press number keys to switch", (Vector2){260, 200}, 24, 1, GRAY);
                break;
        }

        EndDrawing();
    }

    UnloadFont(font);
    CloseWindow();
    return 0;
}
