#pragma once
#include "entity.hpp"
#include "leveleditor.hpp"
#include "levelselector.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

enum class GameState : uint16_t { MENU, LEVEL_SELECT, LEVEL, LEVEL_EDITOR, EXIT };

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
  Entity p{Vector2{200, 200}};
  std::vector<Rectangle> levelTiles;
  std::string currentLevelPath;

  // Screen objects (created on demand)
  std::unique_ptr<LevelSelector> selector;
  std::unique_ptr<LevelEditor> editor;

  void loadLevel(const std::string &path);
  static std::string newLevelPath();
};
