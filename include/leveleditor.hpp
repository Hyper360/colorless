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
  int tx, ty;
  int linkId = 0;
  int etx = -1, ety = -1;
};

class LevelEditor {
public:
  explicit LevelEditor(const std::string &path);
  void update();
  void draw();
  bool wantsExit() const { return exitRequested; }

private:
  std::string levelPath;

  // Editor state
  std::vector<EditorTile> tiles;
  std::vector<EditorObject> objects;
  std::vector<EditorTile> undoTiles;
  std::vector<EditorObject> undoObjects;
  bool objMode = false;
  bool showHelp = true;
  float savedTimer = 0.0f;
  bool exitRequested = false;
  int levelCbMode = 0;

  // Tile mode state
  TileType selectedTile = TileType::SOLID;

  // Object mode state
  ObjType selectedObj = ObjType::PRESSURE_PLATE;
  int linkId = 1;
  bool awaitingSecondClick = false;
  int firstClickX = 0, firstClickY = 0;

  // Tile helpers
  bool hasTile(int x, int y) const;
  void removeTile(int x, int y);
  void placeTile(int x, int y, TileType type);

  // Object helpers
  void removeObjectAt(int tx, int ty);
  EditorObject* findObjectAt(int tx, int ty);
  void placeObject(int tx, int ty);

  // Two-click objects (moving platform, crusher)
  bool isTwoClickObject(ObjType type) const;
  void handleTwoClickFirst(int tx, int ty);
  void handleTwoClickSecond(int tx, int ty);

  // Spawn handling
  bool isSpawnType(ObjType type) const;
  void placeSpawn(int tx, int ty);

  // File operations
  void save();
  void load();
  void saveUndoState();

  // Drawing helpers
  void drawTilePreview(int gridX, int gridY);
  void drawObject(const EditorObject &o);
  void drawToolbar();
  void drawHelpPanel();
};