#include "../include/leveleditor.hpp"
#include "../include/json.hpp"
#include "../include/raylib/raymath.h"
#include <algorithm>
#include <fstream>
using json = nlohmann::json;

const int TS = (int)Config::TILESIZE;

LevelEditor::LevelEditor(const std::string &path) : levelPath(path) { load(); }

bool LevelEditor::hasTile(int x, int y) const {
  for (auto &t : tiles)
    if (t.x == x && t.y == y)
      return true;
  return false;
}

void LevelEditor::removeTile(int x, int y) {
  tiles.erase(std::remove_if(tiles.begin(), tiles.end(),
                             [x, y](const EditorTile &t) {
                               return t.x == x && t.y == y;
                             }),
              tiles.end());
}

void LevelEditor::save() {
  json j;
  j["tiles"] = json::array();
  for (auto &t : tiles)
    j["tiles"].push_back({{"x", t.x}, {"y", t.y}, {"type", (int)t.type}});
  std::ofstream f(levelPath);
  f << j.dump(2);
  savedTimer = 2.0f;
}

void LevelEditor::load() {
  tiles.clear();
  std::ifstream f(levelPath);
  if (!f.is_open() || f.peek() == std::ifstream::traits_type::eof())
    return;
  try {
    json j = json::parse(f);
    for (auto &t : j["tiles"]) {
      TileType type = TileType::SOLID;
      if (t.contains("type"))
        type = static_cast<TileType>(t["type"].get<int>());
      tiles.push_back({t["x"].get<int>(), t["y"].get<int>(), type});
    }
  } catch (...) {}
}

void LevelEditor::update() {
  exitRequested = false;

  const int W = GetScreenWidth();
  const int H = GetScreenHeight();
  const int COLS = W / TS;
  const int ROWS = H / TS;

  Vector2 mouse = GetMousePosition();
  int gx = Clamp((int)(mouse.x / TS), 0, COLS - 1);
  int gy = Clamp((int)(mouse.y / TS), 0, ROWS - 1);

  // Number keys 1-4 select tile type
  if (IsKeyPressed(KEY_ONE))   selectedType = TileType::SOLID;
  if (IsKeyPressed(KEY_TWO))   selectedType = TileType::FIRE;
  if (IsKeyPressed(KEY_THREE)) selectedType = TileType::WATER;
  if (IsKeyPressed(KEY_FOUR))  selectedType = TileType::EXIT;

  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    removeTile(gx, gy); // replace if already there
    tiles.push_back({gx, gy, selectedType});
  }
  if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    removeTile(gx, gy);

  if (IsKeyPressed(KEY_S))
    save();
  if (IsKeyPressed(KEY_L))
    load();
  if (IsKeyPressed(KEY_C))
    tiles.clear();
  if (IsKeyPressed(KEY_ESCAPE))
    exitRequested = true;

  savedTimer -= GetFrameTime();
}

void LevelEditor::draw() {
  const int W = GetScreenWidth();
  const int H = GetScreenHeight();

  ClearBackground(WHITE);

  for (auto &t : tiles)
    DrawRectangle(t.x * TS, t.y * TS, TS, TS, tileColor(t.type));

  // Grid
  for (int x = 0; x <= W; x += TS)
    DrawLine(x, 0, x, H, {180, 180, 180, 255});
  for (int y = 0; y <= H; y += TS)
    DrawLine(0, y, W, y, {180, 180, 180, 255});

  // Hover highlight using selected type color
  Vector2 mouse = GetMousePosition();
  Color hoverCol = tileColor(selectedType);
  hoverCol.a = 120;
  DrawRectangle((int)(mouse.x / TS) * TS, (int)(mouse.y / TS) * TS, TS, TS, hoverCol);

  // Toolbar
  DrawRectangle(0, 0, W, 22, {0, 0, 0, 180});
  DrawText("LMB:Place RMB:Erase C:Clear S:Save L:Load ESC:Back | 1:Solid 2:Fire 3:Water 4:Exit",
           4, 4, 13, WHITE);

  // Selected type indicator (right side)
  const char *typeName = tileName(selectedType);
  std::string sel = std::string("[ ") + typeName + " ]";
  DrawText(sel.c_str(), W - MeasureText(sel.c_str(), 14) - 4, 4, 14, tileColor(selectedType));

  // Level name (centered)
  std::string name = levelPath;
  auto slash = name.find_last_of("/\\");
  if (slash != std::string::npos)
    name = name.substr(slash + 1);
  DrawText(name.c_str(),
           W / 2 - MeasureText(name.c_str(), 14) / 2, 4, 14, YELLOW);

  if (savedTimer > 0)
    DrawText("Saved!", W - 70, 4, 14, GREEN);
}
