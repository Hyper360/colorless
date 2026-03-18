#include "../include/levelselector.hpp"
#include "../include/raylib/raymath.h"
#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;

LevelSelector::LevelSelector() { refresh(); }

void LevelSelector::refresh() {
  levels.clear();
  if (fs::exists("levels/")) {
    for (auto &entry : fs::directory_iterator("levels/"))
      if (entry.path().extension() == ".json")
        levels.push_back(entry.path().string());
    std::sort(levels.begin(), levels.end());
  }
  cursor = Clamp(cursor, 0, (int)levels.size() - 1);
}

std::string LevelSelector::getSelected() const {
  if (levels.empty())
    return "";
  return levels[cursor];
}

void LevelSelector::resetFlags() {
  playRequested = editRequested = newRequested = backRequested = false;
}

void LevelSelector::update() {
  resetFlags();

  if (IsKeyPressed(KEY_DOWN) && cursor < (int)levels.size() - 1)
    cursor++;
  if (IsKeyPressed(KEY_UP) && cursor > 0)
    cursor--;
  if (IsKeyPressed(KEY_ENTER) && !levels.empty())
    playRequested = true;
  if (IsKeyPressed(KEY_E) && !levels.empty())
    editRequested = true;
  if (IsKeyPressed(KEY_N))
    newRequested = true;
  if (IsKeyPressed(KEY_ESCAPE))
    backRequested = true;
}

void LevelSelector::draw() {
  const int W = GetScreenWidth();
  const int H = GetScreenHeight();

  ClearBackground(BLACK);

  const char *title = "SELECT LEVEL";
  DrawText(title, W / 2 - MeasureText(title, 36) / 2, 60, 36, WHITE);

  if (levels.empty()) {
    const char *msg = "No levels found. Press N to create one.";
    DrawText(msg, W / 2 - MeasureText(msg, 18) / 2, H / 2 - 9, 18, GRAY);
  } else {
    for (int i = 0; i < (int)levels.size(); i++) {
      std::string name = levels[i];
      auto slash = name.find_last_of("/\\");
      if (slash != std::string::npos)
        name = name.substr(slash + 1);
      auto dot = name.rfind('.');
      if (dot != std::string::npos)
        name = name.substr(0, dot);

      int y = 150 + i * 44;
      Color col = (i == cursor) ? YELLOW : GRAY;
      if (i == cursor)
        DrawRectangle(0, y - 6, W, 40, {255, 255, 0, 25});
      DrawText(name.c_str(), W / 2 - MeasureText(name.c_str(), 26) / 2, y, 26,
               col);
    }
  }

  DrawText("UP/DOWN: Select  ENTER: Play  E: Edit  N: New  ESC: Back",
           W / 2 - MeasureText("UP/DOWN: Select  ENTER: Play  E: Edit  N: New  ESC: Back", 15) / 2,
           H - 36, 15, DARKGRAY);
}
