#include "../include/json.hpp"
#include "../include/raylib/raylib.h"
#include "config.hpp"
#include <fstream>
#include <string>
#include <vector>
using json = nlohmann::json;

std::vector loadLevel(std::string) {
  std::ifstream f("level.json");
  json levelData = json::parse(f);
}

int main() {
  InitWindow(800, 800, "Level Editor");
  SetTargetFPS(120);

  std::vector<Rectangle> rects;

  while (!WindowShouldClose()) {
    ClearBackground(WHITE);
    BeginDrawing();
    for (Rectangle &r : rects) {
      DrawRectangleRec(r, BLACK);
    }
    EndDrawing();
  }

  CloseWindow();
}
