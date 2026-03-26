#pragma once
#include "entity.hpp"
#include "leveleditor.hpp"
#include "levelselector.hpp"
#include "pausemenu.hpp"
#include "tile.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

enum class GameState : uint16_t { MENU, LEVEL_SELECT, LEVEL, LEVEL_EDITOR, EXIT };

struct LevelTile {
  TileType type;
  Rectangle rect;
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
  Entity p{{200, 200},  {KEY_A, KEY_D, KEY_W},          RED,  ElementType::FIRE};
  Entity p2{{240, 200}, {KEY_LEFT, KEY_RIGHT, KEY_UP},   BLUE, ElementType::WATER};
  std::vector<LevelTile> tiles;
  std::string currentLevelPath;
  float winTimer = 0.0f;
  bool paused = false;
  PauseMenu pauseMenu;

  // Screen objects (created on demand)
  std::unique_ptr<LevelSelector> selector;
  std::unique_ptr<LevelEditor> editor;

  void loadLevel(const std::string &path);
  static std::string newLevelPath();
};
