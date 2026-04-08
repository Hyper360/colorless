#pragma once
#include "entity.hpp"
#include "leveleditor.hpp"
#include "levelselector.hpp"
#include "levelobject.hpp"
#include "pausemenu.hpp"
#include "tile.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

enum class GameState : uint16_t { MENU, LEVEL_SELECT, LEVEL, LEVEL_EDITOR, EXIT };

struct LevelTile {
  TileType type;
  Rectangle rect;       // full tile or hazard visual (top half for hazards)
  Rectangle solidRect;  // walkable portion (bottom half for hazards, same as rect for solid)
};

class Game {
public:
  Game();
  void stateManager();
  GameState getState() const { return state; }
  void exit();

private:
  GameState state{GameState::MENU};

  // Screens
  void menu();
  void levelSelect();
  void runLevel();
  void runLevelEditor();

  // Level gameplay
  Vector2 p1Spawn{200, 200};
  Vector2 p2Spawn{240, 200};
  Entity p{{200, 200},  {KEY_A, KEY_D, KEY_W, KEY_S},             RED,  ElementType::FIRE};
  Entity p2{{240, 200}, {KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN},   BLUE, ElementType::WATER};
  std::vector<LevelTile>   tiles;
  std::vector<LevelObject> objects;
  std::string currentLevelPath;
  float winTimer = 0.0f;
  bool paused = false;
  int  levelCbMode = 0; // 0=none 1=deut 2=prot 3=trit 4=achro (per-level override)
  PauseMenu pauseMenu;

  // Post-process colorblind shader
  RenderTexture2D renderTarget;
  Shader          cbShader;
  int             rRowLoc = 0, gRowLoc = 0, bRowLoc = 0;
  int             blurLoc = 0, vignetteLoc = 0;
  void setCbUniforms();

  // Screen objects (created on demand)
  std::unique_ptr<LevelSelector> selector;
  std::unique_ptr<LevelEditor> editor;

  void loadLevel(const std::string &path);
  static std::string newLevelPath();

  // Object system
  std::vector<Rectangle> buildSolids() const; // tiles + closed gates + platforms
  void preUpdateObjects();                     // conveyor forces (before entity physics)
  void postUpdateObjects(const std::vector<Rectangle> &solids); // carry, push, gates, etc.
  void drawObjects();
};
