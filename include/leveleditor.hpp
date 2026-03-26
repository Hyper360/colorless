#pragma once
#include "config.hpp"
#include "levelobject.hpp"
#include "raylib/raylib.h"
#include "tile.hpp"
#include <string>
#include <vector>

struct EditorTile {
  int x, y;
  TileType type = TileType::SOLID;
};

struct EditorObject {
  ObjType type;
  int tx, ty;         // top-left tile position
  int linkId = 0;
  int etx = -1, ety = -1; // endpoint (moving platform only; -1 = default)
};

class LevelEditor {
public:
  explicit LevelEditor(const std::string &path);
  void update();
  void draw();
  bool wantsExit() const { return exitRequested; }

private:
  std::string levelPath;

  // --- Tile mode ---
  std::vector<EditorTile> tiles;
  TileType selectedType = TileType::SOLID;
  bool hasTile(int x, int y) const;
  void removeTile(int x, int y);

  // --- Object mode ---
  std::vector<EditorObject> objects;
  ObjType selectedObj = ObjType::PRESSURE_PLATE;
  int     linkId      = 1;          // current link channel for plate/lever/gate
  bool    objMode     = false;      // false=tile, true=object
  bool    pendingMP     = false;      // awaiting endpoint for moving platform / crusher
  ObjType pendingMPType = ObjType::MOVING_PLATFORM; // which 2-click object is pending
  int     mpTX = 0, mpTY = 0;
  void removeObjectAt(int tx, int ty);

  // --- Shared ---
  float savedTimer    = 0.0f;
  bool  exitRequested = false;
  int   levelCbMode   = 0;  // 0=none 1=deut 2=prot 3=trit 4=achro
  void save();
  void load();
};
