#pragma once
#include <cstdint>

enum class GameState : uint16_t {
  MENU,
  LEVEL,
  EXIT
};

class Game {
  public:
  Game();
  void stateManager();
  GameState getState(){ return state; }
  void exit();

  private:
  // States
  GameState state{GameState::MENU};
  void menu();
  void level();
};
