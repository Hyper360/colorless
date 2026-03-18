#pragma once
#include "config.hpp"
#include "raylib/raylib.h"
#include <string>
#include <vector>

struct EditorTile {
  int x, y;
};

class LevelEditor {
public:
  explicit LevelEditor(const std::string &path);
  void update();
  void draw();
  bool wantsExit() const { return exitRequested; }

private:
  std::string levelPath;
  std::vector<EditorTile> tiles;
  float savedTimer = 0.0f;
  bool exitRequested = false;

  bool hasTile(int x, int y) const;
  void removeTile(int x, int y);
  void save();
  void load();
};
