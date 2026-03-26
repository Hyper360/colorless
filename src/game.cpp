#include "../include/game.hpp"
#include "../include/json.hpp"
#include "../include/raylib/raylib.h"
#include "../include/settings.hpp"
#include <cstdio>
#include <filesystem>
#include <fstream>
using json = nlohmann::json;
namespace fs = std::filesystem;

// Fragment shader: applies a 3x3 color matrix via three row vec3 uniforms.
// Identity rows (default) produce no change.
static const char *CB_FRAG = R"(
#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 rRow;
uniform vec3 gRow;
uniform vec3 bRow;
out vec4 finalColor;
void main() {
    vec4 c = texture(texture0, fragTexCoord);
    finalColor = vec4(dot(rRow, c.rgb),
                      dot(gRow, c.rgb),
                      dot(bRow, c.rgb),
                      c.a) * fragColor * colDiffuse;
}
)";

// ---- Setup ---------------------------------------------------------------

Game::Game() {
  if (!fs::exists("levels/"))
    fs::create_directory("levels/");
  selector = std::make_unique<LevelSelector>();

  renderTarget = LoadRenderTexture(Config::WIDTH, Config::HEIGHT);
  cbShader     = LoadShaderFromMemory(nullptr, CB_FRAG);
  rRowLoc      = GetShaderLocation(cbShader, "rRow");
  gRowLoc      = GetShaderLocation(cbShader, "gRow");
  bRowLoc      = GetShaderLocation(cbShader, "bRow");
}

// Sets rRow/gRow/bRow uniforms from current Settings flags.
// Colorblind matrices: Brettel/Viénot LMS simulation.
void Game::setCbUniforms() {
  float r[3] = {1, 0, 0};
  float g[3] = {0, 1, 0};
  float b[3] = {0, 0, 1};

  if (Settings::deuteranopia) {
    float nr[] = {0.625f, 0.375f, 0.0f};
    float ng[] = {0.700f, 0.300f, 0.0f};
    float nb[] = {0.0f,   0.300f, 0.700f};
    memcpy(r, nr, 12); memcpy(g, ng, 12); memcpy(b, nb, 12);
  } else if (Settings::protanopia) {
    float nr[] = {0.567f, 0.433f, 0.0f};
    float ng[] = {0.558f, 0.442f, 0.0f};
    float nb[] = {0.0f,   0.242f, 0.758f};
    memcpy(r, nr, 12); memcpy(g, ng, 12); memcpy(b, nb, 12);
  } else if (Settings::tritanopia) {
    float nr[] = {0.950f, 0.050f, 0.0f};
    float ng[] = {0.0f,   0.433f, 0.567f};
    float nb[] = {0.0f,   0.475f, 0.525f};
    memcpy(r, nr, 12); memcpy(g, ng, 12); memcpy(b, nb, 12);
  } else if (Settings::achromatopsia) {
    float nr[] = {0.299f, 0.587f, 0.114f};
    float ng[] = {0.299f, 0.587f, 0.114f};
    float nb[] = {0.299f, 0.587f, 0.114f};
    memcpy(r, nr, 12); memcpy(g, ng, 12); memcpy(b, nb, 12);
  }

  SetShaderValue(cbShader, rRowLoc, r, SHADER_UNIFORM_VEC3);
  SetShaderValue(cbShader, gRowLoc, g, SHADER_UNIFORM_VEC3);
  SetShaderValue(cbShader, bRowLoc, b, SHADER_UNIFORM_VEC3);
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
  if (IsKeyPressed(KEY_ESCAPE) && winTimer <= 0.0f) {
    paused = !paused;
  } else if (paused) {
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

  // --- Render game world to offscreen texture ---
  BeginTextureMode(renderTarget);
  ClearBackground(WHITE);
  for (auto &t : tiles)
    DrawRectangleRec(t.rect, tileColor(t.type));
  p.draw();
  p2.draw();
  EndTextureMode();

  // --- Draw texture to screen through colorblind shader ---
  setCbUniforms();
  const int W = GetScreenWidth(), H = GetScreenHeight();
  Rectangle src = {0, 0, (float)renderTarget.texture.width,
                         -(float)renderTarget.texture.height}; // flip Y
  Rectangle dst = {0, 0, (float)W, (float)H};

  BeginDrawing();
  ClearBackground(BLACK);
  BeginShaderMode(cbShader);
  DrawTexturePro(renderTarget.texture, src, dst, {0, 0}, 0.0f, WHITE);
  EndShaderMode();

  // HUD and overlays drawn after shader — unfiltered so they stay readable
  DrawText("ESC: Pause", 4, 4, 14, DARKGRAY);
  if (winTimer > 0.0f) {
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

void Game::exit() {
  UnloadShader(cbShader);
  UnloadRenderTexture(renderTarget);
  printf("Exiting!\n");
}

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
