#pragma once
#include "entity.hpp"
#include <cstdint>

enum class GameState : uint16_t { MENU, LEVEL, EXIT };

class Game {
public:
  Game();
  void stateManager();
  GameState getState() { return state; }
  void exit();

private:
  // States
  GameState state{GameState::MENU};
  void menu();
  void level();
  Entity p = Entity({200, 200});
  Rectangle r = (Rectangle){200, 400, 500, 30};
};
