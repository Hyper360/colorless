#include "../include/game.hpp"
#include "../include/raylib/raylib.h"
#include <cstdio>

Game::Game() {}

void Game::menu() {
  if (IsKeyPressed(KEY_ENTER)) {
    state = GameState::LEVEL;
  }

  BeginDrawing();
  ClearBackground(ORANGE);
  DrawText("Menu Template", 0, 0, 24, RED);
  EndDrawing();
}

void Game::level() {
  if (IsKeyPressed(KEY_ENTER)) {
    state = GameState::MENU;
  }
  if (IsKeyPressed(KEY_ESCAPE)) {
    state = GameState::EXIT;
  }

  p.update();

  BeginDrawing();
  ClearBackground(WHITE);
  p.draw();
  DrawRectangleRec(r, BLACK);
  EndDrawing();
}

void Game::exit() { printf("Exiting!"); }

void Game::stateManager() {
  switch (state) {
  case GameState::MENU:
    menu();
    break;
  case GameState::LEVEL:
    level();
    break;
  default:
    printf("Invalid same state %i, quitting application", state);
  }
}
