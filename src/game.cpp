#include "../include/game.hpp"
#include "../include/json.hpp"
#include "../include/raylib/raylib.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
using json = nlohmann::json;
namespace fs = std::filesystem;

// ---- Setup ---------------------------------------------------------------

Game::Game() {
  if (!fs::exists("levels/"))
    fs::create_directory("levels/");
  selector = std::make_unique<LevelSelector>();
}

void Game::loadLevel(const std::string &path) {
  levelTiles.clear();
  currentLevelPath = path;
  std::ifstream f(path);
  if (!f.is_open() || f.peek() == std::ifstream::traits_type::eof())
    return;
  try {
    json j = json::parse(f);
    for (auto &t : j["tiles"])
      levelTiles.push_back({(float)t["x"].get<int>() * Config::TILESIZE,
                            (float)t["y"].get<int>() * Config::TILESIZE,
                            Config::TILESIZE, Config::TILESIZE});
  } catch (...) {}
}

std::string Game::newLevelPath() {
  int n = 1;
  while (fs::exists("levels/level" + std::to_string(n) + ".json"))
    n++;
  return "levels/level" + std::to_string(n) + ".json";
}

// ---- States --------------------------------------------------------------

void Game::menu() {
  if (IsKeyPressed(KEY_ENTER))
    state = GameState::LEVEL_SELECT;
  if (IsKeyPressed(KEY_ESCAPE))
    state = GameState::EXIT;

  const int W = GetScreenWidth(), H = GetScreenHeight();
  BeginDrawing();
  ClearBackground(BLACK);
  const char *title = "COLORLESS";
  DrawText(title, W / 2 - MeasureText(title, 52) / 2, H / 3, 52, WHITE);
  const char *sub = "Press ENTER";
  DrawText(sub, W / 2 - MeasureText(sub, 22) / 2, H / 2, 22, GRAY);
  EndDrawing();
}

void Game::levelSelect() {
  selector->update();

  if (selector->wantsPlay()) {
    loadLevel(selector->getSelected());
    p  = Entity({200, 200}, {KEY_A, KEY_D, KEY_W},          RED);
    p2 = Entity({240, 200}, {KEY_LEFT, KEY_RIGHT, KEY_UP},  BLUE);
    state = GameState::LEVEL;
  } else if (selector->wantsEdit()) {
    editor = std::make_unique<LevelEditor>(selector->getSelected());
    state = GameState::LEVEL_EDITOR;
  } else if (selector->wantsNew()) {
    std::string path = newLevelPath();
    { std::ofstream f(path); f << "{\"tiles\": []}"; } // close before editor reads it
    editor = std::make_unique<LevelEditor>(path);
    selector->refresh();
    state = GameState::LEVEL_EDITOR;
  } else if (selector->wantsBack()) {
    state = GameState::MENU;
  }

  BeginDrawing();
  selector->draw();
  EndDrawing();
}

void Game::runLevel() {
  if (IsKeyPressed(KEY_ESCAPE))
    state = GameState::LEVEL_SELECT;

  p.update(levelTiles);
  p2.update(levelTiles);

  BeginDrawing();
  ClearBackground(WHITE);
  for (auto &r : levelTiles)
    DrawRectangleRec(r, BLACK);
  p.draw();
  p2.draw();
  DrawText("ESC: Back", 4, 4, 14, DARKGRAY);
  EndDrawing();
}

void Game::runLevelEditor() {
  editor->update();

  if (editor->wantsExit()) {
    selector->refresh();
    state = GameState::LEVEL_SELECT;
  }

  BeginDrawing();
  editor->draw();
  EndDrawing();
}

void Game::exit() { printf("Exiting!\n"); }

void Game::stateManager() {
  switch (state) {
  case GameState::MENU:
    menu();
    break;
  case GameState::LEVEL_SELECT:
    levelSelect();
    break;
  case GameState::LEVEL:
    runLevel();
    break;
  case GameState::LEVEL_EDITOR:
    runLevelEditor();
    break;
  default:
    break;
  }
}
