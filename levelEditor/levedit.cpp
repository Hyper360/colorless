#include "../include/json.hpp"
#include "../include/raylib/raylib.h"
#include "../include/config.hpp"
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
using json = nlohmann::json;

const int W = 800;
const int H = 800;
const int TS = (int)Config::TILESIZE;
const int COLS = W / TS;
const int ROWS = H / TS;

struct Tile {
  int x, y;
};

bool hasTile(const std::vector<Tile> &tiles, int x, int y) {
  for (auto &t : tiles)
    if (t.x == x && t.y == y)
      return true;
  return false;
}

void removeTile(std::vector<Tile> &tiles, int x, int y) {
  tiles.erase(std::remove_if(tiles.begin(), tiles.end(),
                             [x, y](const Tile &t) {
                               return t.x == x && t.y == y;
                             }),
              tiles.end());
}

void saveLevel(const std::vector<Tile> &tiles, const std::string &path) {
  json j;
  j["tiles"] = json::array();
  for (auto &t : tiles)
    j["tiles"].push_back({{"x", t.x}, {"y", t.y}});
  std::ofstream f(path);
  f << j.dump(2);
}

std::vector<Tile> loadLevel(const std::string &path) {
  std::vector<Tile> tiles;
  std::ifstream f(path);
  if (!f.is_open())
    return tiles;
  json j = json::parse(f);
  for (auto &t : j["tiles"])
    tiles.push_back({t["x"], t["y"]});
  return tiles;
}

int main() {
  InitWindow(W, H, "Level Editor");
  SetTargetFPS(120);

  std::vector<Tile> tiles;
  float savedTimer = 0.0f;

  while (!WindowShouldClose()) {
    Vector2 mouse = GetMousePosition();
    int gx = Clamp((int)(mouse.x / TS), 0, COLS - 1);
    int gy = Clamp((int)(mouse.y / TS), 0, ROWS - 1);

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
      if (!hasTile(tiles, gx, gy))
        tiles.push_back({gx, gy});

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
      removeTile(tiles, gx, gy);

    if (IsKeyPressed(KEY_S)) {
      saveLevel(tiles, "level.json");
      savedTimer = 2.0f;
    }
    if (IsKeyPressed(KEY_L))
      tiles = loadLevel("level.json");
    if (IsKeyPressed(KEY_C))
      tiles.clear();

    savedTimer -= GetFrameTime();

    BeginDrawing();
    ClearBackground(WHITE);

    // Placed tiles
    for (auto &t : tiles)
      DrawRectangle(t.x * TS, t.y * TS, TS, TS, BLACK);

    // Grid lines
    for (int x = 0; x <= W; x += TS)
      DrawLine(x, 0, x, H, {180, 180, 180, 255});
    for (int y = 0; y <= H; y += TS)
      DrawLine(0, y, W, y, {180, 180, 180, 255});

    // Hovered tile
    DrawRectangle(gx * TS, gy * TS, TS, TS, {0, 0, 0, 60});

    // UI
    DrawText("LMB: Place  RMB: Erase  C: Clear  S: Save  L: Load", 4, 4, 14,
             DARKGRAY);
    if (savedTimer > 0)
      DrawText("Saved!", W - 70, 4, 14, GREEN);

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
