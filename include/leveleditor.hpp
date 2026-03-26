#pragma once
#include "config.hpp"
#include "raylib/raylib.h"
#include "tile.hpp"
#include <string>
#include <vector>

struct EditorTile {
  int x, y;
  TileType type = TileType::SOLID;
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
  TileType selectedType = TileType::SOLID;
  float savedTimer = 0.0f;
  bool exitRequested = false;

  bool hasTile(int x, int y) const;
  void removeTile(int x, int y);
  void save();
  void load();
};
