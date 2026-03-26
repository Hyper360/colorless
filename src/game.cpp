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
  tiles.clear();
  currentLevelPath = path;
  std::ifstream f(path);
  if (!f.is_open() || f.peek() == std::ifstream::traits_type::eof())
    return;
  try {
    json j = json::parse(f);
    for (auto &t : j["tiles"]) {
      TileType type = TileType::SOLID;
      if (t.contains("type"))
        type = static_cast<TileType>(t["type"].get<int>());
      float x = (float)t["x"].get<int>() * Config::TILESIZE;
      float y = (float)t["y"].get<int>() * Config::TILESIZE;
      tiles.push_back({type, {x, y, Config::TILESIZE, Config::TILESIZE}});
    }
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
    p  = Entity({200, 200}, {KEY_A, KEY_D, KEY_W},         RED,  ElementType::FIRE);
    p2 = Entity({240, 200}, {KEY_LEFT, KEY_RIGHT, KEY_UP}, BLUE, ElementType::WATER);
    winTimer = 0.0f;
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
  if (IsKeyPressed(KEY_ESCAPE) && winTimer <= 0.0f)
    paused = !paused;

  if (paused) {
    pauseMenu.update();
    if (pauseMenu.wantsResume()) paused = false;
    if (pauseMenu.wantsQuit())   { paused = false; state = GameState::LEVEL_SELECT; }
  }

  // Build solid list for physics
  std::vector<Rectangle> solids;
  for (auto &t : tiles)
    if (t.type == TileType::SOLID)
      solids.push_back(t.rect);

  if (!paused && winTimer <= 0.0f) {
    p.update(solids);
    p2.update(solids);

    // Hazard check: fire kills water player, water kills fire player
    for (auto &t : tiles) {
      if (t.type == TileType::FIRE &&
          p2.getElement() == ElementType::WATER &&
          CheckCollisionRecs(p2.getBody(), t.rect))
        p2.respawn();
      if (t.type == TileType::WATER &&
          p.getElement() == ElementType::FIRE &&
          CheckCollisionRecs(p.getBody(), t.rect))
        p.respawn();
    }

    // Win: both players overlap an exit tile simultaneously
    bool p1Exit = false, p2Exit = false;
    for (auto &t : tiles) {
      if (t.type == TileType::EXIT) {
        if (CheckCollisionRecs(p.getBody(), t.rect))  p1Exit = true;
        if (CheckCollisionRecs(p2.getBody(), t.rect)) p2Exit = true;
      }
    }
    if (p1Exit && p2Exit)
      winTimer = 2.0f;
  }

  if (winTimer > 0.0f) {
    winTimer -= GetFrameTime();
    if (winTimer <= 0.0f)
      state = GameState::LEVEL_SELECT;
  }

  BeginDrawing();
  ClearBackground(WHITE);
  for (auto &t : tiles)
    DrawRectangleRec(t.rect, tileColor(t.type));
  p.draw();
  p2.draw();
  DrawText("ESC: Pause", 4, 4, 14, DARKGRAY);
  if (winTimer > 0.0f) {
    const int W = GetScreenWidth(), H = GetScreenHeight();
    DrawRectangle(0, 0, W, H, {0, 0, 0, 160});
    const char *msg = "YOU WIN!";
    DrawText(msg, W / 2 - MeasureText(msg, 60) / 2, H / 2 - 30, 60, YELLOW);
  }
  if (paused)
    pauseMenu.draw();
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
