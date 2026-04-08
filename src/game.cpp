#include "../include/game.hpp"
#include "../include/json.hpp"
#include "../include/raylib/raylib.h"
#include "../include/settings.hpp"
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
using json = nlohmann::json;
namespace fs = std::filesystem;

// Single post-process pass: color matrix + optional blur + optional vignette.
// All uniforms default to identity / zero so the shader is a no-op when
// no impairment is active.
static const char *CB_FRAG = R"(
#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3  rRow;         // color matrix rows (identity = no change)
uniform vec3  gRow;
uniform vec3  bRow;
uniform float blurRadius;   // 0 = off; ~4 = low acuity
uniform float vignetteStr;  // 0 = off; ~1.2 = tunnel vision
uniform vec2  jitterOff;    // nystagmus UV offset (0,0 = off)
out vec4 finalColor;
void main() {
    vec2 uv = fragTexCoord + jitterOff;

    // --- Blur (5x5 box, configurable radius) ---
    vec4 c = vec4(0.0);
    if (blurRadius > 0.0) {
        vec2 ts = blurRadius / vec2(textureSize(texture0, 0));
        for (int x = -2; x <= 2; x++)
            for (int y = -2; y <= 2; y++)
                c += texture(texture0, uv + vec2(x, y) * ts);
        c /= 25.0;
    } else {
        c = texture(texture0, uv);
    }

    // --- Color matrix ---
    vec3 rgb = vec3(dot(rRow, c.rgb), dot(gRow, c.rgb), dot(bRow, c.rgb));

    // --- Vignette (tunnel vision) ---
    if (vignetteStr > 0.0) {
        float dist = length(uv - 0.5) * 2.0;
        float v = 1.0 - smoothstep(0.35, 0.85, dist * vignetteStr);
        rgb *= v;
    }

    finalColor = vec4(rgb, c.a) * fragColor * colDiffuse;
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
  blurLoc      = GetShaderLocation(cbShader, "blurRadius");
  vignetteLoc  = GetShaderLocation(cbShader, "vignetteStr");
  jitterLoc    = GetShaderLocation(cbShader, "jitterOff");
}

// Sets rRow/gRow/bRow uniforms from current Settings flags.
// Colorblind matrices: Brettel/Viénot LMS simulation.
void Game::setCbUniforms() {
  float r[3] = {1, 0, 0};
  float g[3] = {0, 1, 0};
  float b[3] = {0, 0, 1};

  // levelCbMode overrides Settings flags so a level can force a colorblind perspective
  bool deut  = Settings::deuteranopia  || levelCbMode == 1;
  bool prot  = Settings::protanopia    || levelCbMode == 2;
  bool achro = Settings::achromatopsia || levelCbMode == 3;

  if (deut) {
    // Deuteranopia: strong red-green confusion. Red and green collapse to yellow-brown.
    // Machado et al. severity ~0.9 — makes fire (red) and water (green) nearly identical.
    float nr[] = {0.80f, 0.20f, 0.0f};
    float ng[] = {0.26f, 0.74f, 0.0f};
    float nb[] = {0.0f,  0.14f, 0.86f};
    memcpy(r, nr, 12); memcpy(g, ng, 12); memcpy(b, nb, 12);
  } else if (prot) {
    // Protanopia: strong red-green confusion. Red looks dark/brownish, green stays similar.
    float nr[] = {0.15f, 0.85f, 0.0f};
    float ng[] = {0.15f, 0.85f, 0.0f};
    float nb[] = {0.0f,  0.12f, 0.88f};
    memcpy(r, nr, 12); memcpy(g, ng, 12); memcpy(b, nb, 12);
  } else if (achro) {
    // Achromatopsia: total colorblindness (greyscale)
    float nr[] = {0.299f, 0.587f, 0.114f};
    float ng[] = {0.299f, 0.587f, 0.114f};
    float nb[] = {0.299f, 0.587f, 0.114f};
    memcpy(r, nr, 12); memcpy(g, ng, 12); memcpy(b, nb, 12);
  }

  SetShaderValue(cbShader, rRowLoc, r, SHADER_UNIFORM_VEC3);
  SetShaderValue(cbShader, gRowLoc, g, SHADER_UNIFORM_VEC3);
  SetShaderValue(cbShader, bRowLoc, b, SHADER_UNIFORM_VEC3);

  float blur     = Settings::lowAcuity    ? 4.0f : 0.0f;
  float vignette = Settings::tunnelVision ? 1.2f : 0.0f;
  SetShaderValue(cbShader, blurLoc,     &blur,     SHADER_UNIFORM_FLOAT);
  SetShaderValue(cbShader, vignetteLoc, &vignette, SHADER_UNIFORM_FLOAT);

  // Nystagmus: involuntary rapid eye jitter — random UV offset each frame
  float jitter[2] = {0.0f, 0.0f};
  if (Settings::nystagmus) {
    float t = (float)GetTime();
    jitter[0] = sinf(t * 37.0f) * 0.008f + sinf(t * 53.0f) * 0.004f;
    jitter[1] = cosf(t * 41.0f) * 0.006f + cosf(t * 67.0f) * 0.003f;
  }
  SetShaderValue(cbShader, jitterLoc, jitter, SHADER_UNIFORM_VEC2);
}

void Game::loadLevel(const std::string &path) {
  tiles.clear();
  objects.clear();
  currentLevelPath = path;
  std::ifstream f(path);
  if (!f.is_open() || f.peek() == std::ifstream::traits_type::eof())
    return;
  try {
    json j = json::parse(f);
    const float ts = Config::TILESIZE;

    for (auto &t : j["tiles"]) {
      TileType type = TileType::SOLID;
      if (t.contains("type"))
        type = static_cast<TileType>(t["type"].get<int>());
      float x = (float)t["x"].get<int>() * ts;
      float y = (float)t["y"].get<int>() * ts;
      if (type == TileType::FIRE || type == TileType::WATER || type == TileType::SPIKE) {
        // Hazard tiles: bottom half is solid ground, top half is the hazard
        Rectangle hazardRect = {x, y, ts, ts * 0.5f};          // top half = danger zone
        Rectangle solidRect  = {x, y + ts * 0.5f, ts, ts * 0.5f}; // bottom half = walkable
        tiles.push_back({type, hazardRect, solidRect});
      } else {
        tiles.push_back({type, {x, y, ts, ts}, {x, y, ts, ts}});
      }
    }

    // Reset per-level state
    p1Spawn    = {200, 200};
    p2Spawn    = {240, 200};
    levelCbMode = j.contains("colorblind") ? j["colorblind"].get<int>() : 0;

    if (j.contains("objects")) {
      for (auto &o : j["objects"]) {
        LevelObject lo{};
        lo.type   = static_cast<ObjType>(o["type"].get<int>());
        lo.linkId = o.contains("link") ? o["link"].get<int>() : 0;
        float tx = (float)o["x"].get<int>() * ts;
        float ty = (float)o["y"].get<int>() * ts;

        // Spawn markers set positions and are not added to the object list
        if (lo.type == ObjType::SPAWN_P1) { p1Spawn = {tx, ty}; continue; }
        if (lo.type == ObjType::SPAWN_P2) { p2Spawn = {tx, ty}; continue; }

        switch (lo.type) {
        case ObjType::PRESSURE_PLATE:
          lo.rect = {tx, ty, ts, ts};
          break;
        case ObjType::LEVER:
          lo.rect = {tx + ts*0.35f, ty + ts*0.1f, ts*0.3f, ts*0.7f};
          break;
        case ObjType::GATE:
          lo.rect = {tx, ty, ts, ts*2.0f};
          break;
        case ObjType::MOVING_PLATFORM: {
          float etx = o.contains("ex") ? (float)o["ex"].get<int>()*ts : tx + ts*4;
          float ety = o.contains("ey") ? (float)o["ey"].get<int>()*ts : ty;
          lo.origin   = {tx,  ty};
          lo.endpoint = {etx, ety};
          lo.rect     = {tx,  ty, ts, ts*0.4f};
          lo.speed    = o.contains("speed") ? o["speed"].get<float>() : 80.0f;
          break;
        }
        case ObjType::CONVEYOR_LEFT:
        case ObjType::CONVEYOR_RIGHT:
          lo.rect = {tx, ty, ts, ts*0.4f};
          break;
        case ObjType::FALLING_PLATFORM:
          lo.rect = {tx, ty, ts, ts*0.4f};
          break;
        case ObjType::PUSHABLE_BLOCK:
          lo.rect = {tx, ty, ts, ts};
          break;
        case ObjType::CRUSHER: {
          float etx = o.contains("ex") ? (float)o["ex"].get<int>()*ts : tx + ts*4;
          float ety = o.contains("ey") ? (float)o["ey"].get<int>()*ts : ty;
          lo.origin   = {tx,  ty};
          lo.endpoint = {etx, ety};
          lo.rect     = {tx,  ty, ts, ts};
          lo.speed    = o.contains("speed") ? o["speed"].get<float>() : 120.0f;
          break;
        }
        case ObjType::ELECTRIC:
          lo.rect   = {tx, ty, ts, ts * 0.25f};
          lo.speed  = o.contains("speed") ? o["speed"].get<float>() : 1.5f; // period in seconds
          lo.tParam = 0.0f;
          lo.active = true;
          break;
        default: break;
        }
        objects.push_back(lo);
      }
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

  // Animated floating particles
  double time = GetTime();
  for (int i = 0; i < 15; i++) {
    float seed = (float)i * 1.7f;
    float px = W * 0.5f + std::sin(seed + time * 0.3f) * W * 0.35f;
    float py = (float)H - std::fmod((float)(time * 20.0 + seed * 80.0), (float)H);
    // Cycle between dim red and dim blue
    float blend = std::sin(seed + (float)time * 0.5f) * 0.5f + 0.5f;
    unsigned char r = (unsigned char)(120 * (1.0f - blend));
    unsigned char b = (unsigned char)(120 * blend);
    DrawCircleV({px, py}, 3.0f, {r, 20, b, 120});
  }

  // Pulsing glow on title
  const char *title = "COLORLESS";
  int titleW = MeasureText(title, 52);
  float pulse = (std::sin((float)time * 2.0f) * 0.5f + 0.5f);
  unsigned char ga = (unsigned char)(40 + pulse * 60);
  DrawText(title, W / 2 - MeasureText(title, 58) / 2, H / 3 - 3, 58, {255, 255, 255, ga});
  DrawText(title, W / 2 - titleW / 2, H / 3, 52, WHITE);

  const char *sub = "Press ENTER";
  DrawText(sub, W / 2 - MeasureText(sub, 22) / 2, H / 2, 22, GRAY);
  EndDrawing();
}

void Game::levelSelect() {
  selector->update();

  if (selector->wantsPlay()) {
    loadLevel(selector->getSelected());
    p  = Entity(p1Spawn, {KEY_A, KEY_D, KEY_W, KEY_S},           RED,  ElementType::FIRE);
    p2 = Entity(p2Spawn, {KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN}, BLUE, ElementType::WATER);
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

// Draws a single tile with optional accessibility overlays.
static void drawTile(const LevelTile &t) {
  const int TS = (int)Config::TILESIZE;
  const int HTS = TS / 2; // half tile size
  const bool isHazard = (t.type == TileType::FIRE || t.type == TileType::WATER || t.type == TileType::SPIKE);

  // For hazards: rect = top half (hazard visual), solidRect = bottom half (ground)
  // Draw the solid ground portion first for hazard tiles
  if (isHazard) {
    Color groundColor = Settings::highContrast ? Color{80, 80, 80, 255} : BLACK;
    DrawRectangleRec(t.solidRect, groundColor);
    if (Settings::highContrast) {
      Color light = ColorBrightness(groundColor, 0.35f);
      Color dark  = ColorBrightness(groundColor, -0.35f);
      DrawRectangle((int)t.solidRect.x, (int)t.solidRect.y, (int)t.solidRect.width, 2, light);
      DrawRectangle((int)t.solidRect.x, (int)t.solidRect.y, 2, (int)t.solidRect.height, light);
      DrawRectangle((int)t.solidRect.x, (int)(t.solidRect.y + t.solidRect.height - 2), (int)t.solidRect.width, 2, dark);
      DrawRectangle((int)(t.solidRect.x + t.solidRect.width - 2), (int)t.solidRect.y, 2, (int)t.solidRect.height, dark);
    }
  }

  // Position/size for the drawable part (full tile for solids, top half for hazards)
  const int x  = (int)t.rect.x, y = (int)t.rect.y;
  const int drawH = isHazard ? HTS : TS;
  const float cx = x + TS * 0.5f, cy = y + drawH * 0.5f;

  // High contrast: swap tile color to high-luminance palette meeting 7:1 ratio
  Color baseColor = tileColor(t.type);
  if (Settings::highContrast) {
    switch (t.type) {
    case TileType::SOLID:   baseColor = {80,  80,  80,  255}; break;
    case TileType::FIRE:    baseColor = {255, 200, 0,   255}; break;
    case TileType::WATER:   baseColor = {0,   220, 255, 255}; break;
    case TileType::EXIT_P1: baseColor = {255, 80,  80,  255}; break;
    case TileType::EXIT_P2: baseColor = {80,  160, 255, 255}; break;
    case TileType::SPIKE:   baseColor = {200, 200, 220, 255}; break;
    }
  }

  // Spike tiles: draw the base rect then triangle spikes pointing upward
  if (t.type == TileType::SPIKE) {
    DrawRectangleRec(t.rect, baseColor);
    constexpr int N = 3;
    float sw = (float)TS / N;
    float spikeH = t.rect.height;
    for (int i = 0; i < N; i++) {
      float sx = (float)x + i * sw;
      DrawTriangle({sx + sw * 0.5f, (float)y},
                   {sx,             (float)(y + spikeH)},
                   {sx + sw,        (float)(y + spikeH)},
                   {30, 30, 40, 220});
    }
    if (Settings::highContrast) DrawRectangleLinesEx(t.rect, 2, WHITE);
    return; // skip generic DrawRectangleRec below
  }

  DrawRectangleRec(t.rect, baseColor);

  // Bevel/3D effect on solid tiles
  if (t.type == TileType::SOLID) {
    Color light = ColorBrightness(baseColor, 0.35f);
    Color dark  = ColorBrightness(baseColor, -0.35f);
    DrawRectangle(x, y, TS, 2, light);           // top edge
    DrawRectangle(x, y, 2, TS, light);           // left edge
    DrawRectangle(x, y + TS - 2, TS, 2, dark);  // bottom edge
    DrawRectangle(x + TS - 2, y, 2, TS, dark);  // right edge
  }

  // Inner gradient on fire/water tiles (lighter at top)
  if (t.type == TileType::FIRE || t.type == TileType::WATER) {
    int bandH = drawH / 3;
    if (bandH < 1) bandH = 1;
    for (int band = 0; band < 3; band++) {
      unsigned char a = (unsigned char)(50 - band * 20); // 50, 30, 10
      DrawRectangle(x, y + band * bandH, TS, bandH, {255, 255, 255, a});
    }
  }

  if (Settings::highContrast)
    DrawRectangleLinesEx(t.rect, 2, WHITE);

  if (Settings::patternTiles) {
    // Alpha 220 so patterns survive the colorblind shader.
    // Orientation is the distinguishing property (pre-attentive), not just color.
    switch (t.type) {
    case TileType::SOLID:
      // diagonal cross-hatch (45°)
      for (int i = 0; i < TS * 2; i += 7) {
        DrawLine(x + i, y, x, y + i, {255, 255, 255, 220});
        DrawLine(x + TS - i, y + TS, x + TS, y + TS - i, {255, 255, 255, 220});
      }
      break;
    case TileType::FIRE:
      // chevron / zigzag (ascending diagonal, distinct from vertical)
      for (int row = 0; row < drawH; row += 7) {
        for (int col = 0; col < TS - 4; col += 8) {
          DrawLine(x + col,     y + row + 4, x + col + 4, y + row,     {255, 255, 255, 220});
          DrawLine(x + col + 4, y + row,     x + col + 8, y + row + 4, {255, 255, 255, 220});
        }
      }
      break;
    case TileType::WATER:
      // sine-wave approximation (horizontal undulation, distinct from fire chevrons)
      for (int row = 4; row < drawH; row += 7) {
        for (int col = 0; col < TS - 1; col++) {
          int y0 = (int)(std::sin((col)      * 0.4f) * 2.0f);
          int y1 = (int)(std::sin((col + 1)  * 0.4f) * 2.0f);
          DrawLine(x + col, y + row + y0, x + col + 1, y + row + y1, {255, 255, 255, 220});
        }
      }
      break;
    case TileType::EXIT_P1:
    case TileType::EXIT_P2:
      // dot grid (distinct from all line patterns)
      for (int dx = 5; dx < TS; dx += 7)
        for (int dy = 5; dy < TS; dy += 7)
          DrawRectangle(x + dx, y + dy, 3, 3, {255, 255, 255, 220});
      break;
    case TileType::SPIKE:
      // vertical lines (distinct from diagonal/wave/dot patterns)
      for (int col = 4; col < TS; col += 6)
        DrawLine(x + col, y, x + col, y + drawH, {255, 255, 255, 220});
      break;
    }
  }

  // Shape labels: geometric symbols (pre-attentive, no reading required)
  if (Settings::shapeLabels && t.type != TileType::SOLID) {
    const float sz = TS * 0.28f;
    switch (t.type) {
    case TileType::FIRE:
      DrawTriangle({cx,      cy - sz}, {cx - sz, cy + sz*0.6f}, {cx + sz, cy + sz*0.6f}, {0,0,0,200});
      break;
    case TileType::WATER:
      DrawCircleV({cx, cy}, sz * 0.85f, {0, 0, 0, 200});
      break;
    case TileType::EXIT_P1:
      // downward triangle (door shape) + "1"
      DrawTriangle({cx - sz, cy - sz*0.3f}, {cx + sz, cy - sz*0.3f}, {cx, cy + sz}, {0,0,0,200});
      DrawText("1", (int)cx - 3, (int)(cy - sz*0.25f), 10, WHITE);
      break;
    case TileType::EXIT_P2:
      DrawTriangle({cx - sz, cy - sz*0.3f}, {cx + sz, cy - sz*0.3f}, {cx, cy + sz}, {0,0,0,200});
      DrawText("2", (int)cx - 3, (int)(cy - sz*0.25f), 10, WHITE);
      break;
    case TileType::SPIKE: {
      // downward-pointing triangle = danger marker
      float sw = sz * 1.3f;
      DrawTriangle({cx - sw, cy - sz*0.5f}, {cx + sw, cy - sz*0.5f}, {cx, cy + sz}, {0,0,0,200});
      break;
    }
    default: break;
    }
  }
}

void Game::runLevel() {
  if (IsKeyPressed(KEY_ESCAPE) && winTimer <= 0.0f) {
    paused = !paused;
  } else if (paused) {
    pauseMenu.update();
    if (pauseMenu.wantsResume()) paused = false;
    if (pauseMenu.wantsQuit())   { paused = false; state = GameState::LEVEL_SELECT; }
  }

  // Build dynamic solid list (tiles + closed gates + platforms; no pushable blocks)
  std::vector<Rectangle> solids = buildSolids();

  if (!paused && winTimer <= 0.0f) {
    preUpdateObjects();      // conveyor forces applied to entity velocity
    p.update(solids);
    p2.update(solids);
    postUpdateObjects(solids); // carry, push, gates, falling, etc.

    // Death: hazard tiles, hazard objects, or falling off the bottom — always respawn both
    // t.rect is the hazard zone (top half); player dies if they overlap it
    bool death = false;
    for (auto &t : tiles) {
      if (t.type == TileType::FIRE  && CheckCollisionRecs(p2.getBody(), t.rect)) death = true;
      if (t.type == TileType::WATER && CheckCollisionRecs(p.getBody(),  t.rect)) death = true;
      if (t.type == TileType::SPIKE) {
        if (CheckCollisionRecs(p.getBody(),  t.rect)) death = true;
        if (CheckCollisionRecs(p2.getBody(), t.rect)) death = true;
      }
    }
    for (auto &o : objects) {
      if (o.type == ObjType::CRUSHER) {
        if (CheckCollisionRecs(p.getBody(),  o.rect)) death = true;
        if (CheckCollisionRecs(p2.getBody(), o.rect)) death = true;
      }
      if (o.type == ObjType::ELECTRIC && o.active) {
        if (CheckCollisionRecs(p.getBody(),  o.rect)) death = true;
        if (CheckCollisionRecs(p2.getBody(), o.rect)) death = true;
      }
    }
    if (p.getBody().y  > GetScreenHeight()) death = true;
    if (p2.getBody().y > GetScreenHeight()) death = true;
    if (death) {
      // Full level reset: reload all objects (gates, levers, blocks, etc.)
      loadLevel(currentLevelPath);
      p  = Entity(p1Spawn, {KEY_A, KEY_D, KEY_W, KEY_S},           RED,  ElementType::FIRE);
      p2 = Entity(p2Spawn, {KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN}, BLUE, ElementType::WATER);
    }

    // Win: p1 on EXIT_P1 and p2 on EXIT_P2 simultaneously
    bool p1Exit = false, p2Exit = false;
    for (auto &t : tiles) {
      if (t.type == TileType::EXIT_P1 && CheckCollisionRecs(p.getBody(),  t.rect)) p1Exit = true;
      if (t.type == TileType::EXIT_P2 && CheckCollisionRecs(p2.getBody(), t.rect)) p2Exit = true;
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
  ClearBackground(Settings::highContrast ? DARKGRAY : WHITE);

  // Background dot grid pattern
  for (int dx = 0; dx < Config::WIDTH; dx += 64)
    for (int dy = 0; dy < Config::HEIGHT; dy += 64)
      DrawRectangle(dx, dy, 2, 2, {200, 200, 200, 80});

  for (auto &t : tiles)
    drawTile(t);
  drawObjects();
  p.draw();
  p2.draw();
  if (Settings::highContrast) {
    // Differentiated outlines: YELLOW for fire player, WHITE for water player
    DrawRectangleLinesEx(p.getBody(),  3, YELLOW);
    DrawRectangleLinesEx(p2.getBody(), 3, WHITE);
  }
  // Shape markers inside players so they're distinguishable without color
  if (Settings::shapeLabels) {
    Rectangle b1 = p.getBody(), b2 = p2.getBody();
    float cx1 = b1.x + b1.width * 0.5f, cy1 = b1.y + b1.height * 0.5f;
    float cx2 = b2.x + b2.width * 0.5f, cy2 = b2.y + b2.height * 0.5f;
    float sz  = b1.width * 0.22f;
    // fire player: small upward triangle
    DrawTriangle({cx1,      cy1 - sz},
                 {cx1 - sz, cy1 + sz * 0.6f},
                 {cx1 + sz, cy1 + sz * 0.6f},
                 {0, 0, 0, 220});
    // water player: small circle
    DrawCircleV({cx2, cy2}, sz * 0.85f, {0, 0, 0, 220});
  }
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

    // Expanding rings emanating from center
    float elapsed = 2.0f - winTimer; // 0..2
    for (int i = 0; i < 4; i++) {
      float ringT = elapsed - i * 0.3f;
      if (ringT > 0.0f) {
        float radius = ringT * 200.0f;
        unsigned char ra = (unsigned char)std::max(0.0f, 180.0f - ringT * 120.0f);
        DrawRing({(float)(W/2), (float)(H/2)}, radius - 2, radius + 2, 0, 360, 36, {255, 255, 0, ra});
      }
    }

    // Scale text up from small
    const char *msg = "YOU WIN!";
    float scale = std::min(1.0f, elapsed * 2.0f); // reaches full size at 0.5s
    int fontSize = (int)(60 * scale);
    if (fontSize < 8) fontSize = 8;
    DrawText(msg, W / 2 - MeasureText(msg, fontSize) / 2, H / 2 - fontSize / 2, fontSize, YELLOW);
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

// ---- Object system -------------------------------------------------------

std::vector<Rectangle> Game::buildSolids() const {
  std::vector<Rectangle> s;
  for (auto &t : tiles) {
    if (t.type == TileType::SOLID) s.push_back(t.rect);
    if (t.type == TileType::FIRE || t.type == TileType::WATER || t.type == TileType::SPIKE)
      s.push_back(t.solidRect);  // bottom half is walkable ground
  }
  for (auto &o : objects) {
    switch (o.type) {
    case ObjType::PRESSURE_PLATE:
    case ObjType::MOVING_PLATFORM:
    case ObjType::CONVEYOR_LEFT:
    case ObjType::CONVEYOR_RIGHT:
      s.push_back(o.rect); break;
    case ObjType::GATE:
      if (!o.active) s.push_back(o.rect); break;
    case ObjType::FALLING_PLATFORM:
      if (!o.falling) s.push_back(o.rect); break;
    case ObjType::CRUSHER:
      s.push_back(o.rect); break;
    default: break;
    }
    // Pushable blocks intentionally excluded — push resolution handles them separately
  }
  return s;
}

// Must be called BEFORE entity physics so conveyor velocity is included in this frame's update.
void Game::preUpdateObjects() {
  const float dt  = GetFrameTime();

  // Electric hazard: tick the phase timer, update active state
  for (auto &o : objects) {
    if (o.type != ObjType::ELECTRIC) continue;
    o.tParam += dt;
    if (o.tParam >= o.speed) o.tParam = 0.0f;
    o.active = (o.tParam < o.speed * 0.5f);
  }

  const float spd = 100.0f; // conveyor belt speed px/s
  for (auto &o : objects) {
    if (o.type != ObjType::CONVEYOR_LEFT && o.type != ObjType::CONVEYOR_RIGHT) continue;
    float vx = (o.type == ObjType::CONVEYOR_RIGHT ? 1.0f : -1.0f) * spd;
    for (Entity *e : {&p, &p2}) {
      Rectangle pb = e->getBody();
      if (std::abs((pb.y + pb.height) - o.rect.y) <= 3.0f &&
          pb.x + pb.width > o.rect.x && pb.x < o.rect.x + o.rect.width)
        e->applyConveyorX(vx);
    }
  }
}

// Called AFTER entity physics.
void Game::postUpdateObjects(const std::vector<Rectangle> &solids) {
  const float dt = GetFrameTime();

  // 1. Moving platforms: advance patrol, carry players
  for (auto &o : objects) {
    if (o.type != ObjType::MOVING_PLATFORM && o.type != ObjType::CRUSHER) continue;
    float dist = Vector2Distance(o.origin, o.endpoint);
    if (dist < 1.0f) continue;
    Vector2 prev = {o.rect.x, o.rect.y};
    o.tParam += o.patDir * o.speed * dt / dist;
    if (o.tParam >= 1.0f) { o.tParam = 1.0f; o.patDir = -1; }
    if (o.tParam <= 0.0f) { o.tParam = 0.0f; o.patDir =  1; }
    o.rect.x = o.origin.x + (o.endpoint.x - o.origin.x) * o.tParam;
    o.rect.y = o.origin.y + (o.endpoint.y - o.origin.y) * o.tParam;
    Vector2 delta = {o.rect.x - prev.x, o.rect.y - prev.y};

    // Only moving platforms carry players; crushers kill them (handled in runLevel)
    if (o.type == ObjType::MOVING_PLATFORM) {
      for (Entity *e : {&p, &p2}) {
        Rectangle pb = e->getBody();
        if (std::abs((pb.y + pb.height) - prev.y) <= 4.0f &&
            pb.x + pb.width > prev.x && pb.x < prev.x + o.rect.width)
          e->nudge(delta);
      }
    }
  }

  // 2. Pressure plate activation (player bottom resting on plate top)
  for (auto &o : objects) {
    if (o.type != ObjType::PRESSURE_PLATE) continue;
    bool pressed = false;
    for (Entity *e : {&p, &p2}) {
      Rectangle pb = e->getBody();
      if (std::abs((pb.y + pb.height) - o.rect.y) <= 3.0f &&
          pb.x + pb.width > o.rect.x && pb.x < o.rect.x + o.rect.width)
        pressed = true;
    }
    // Pushable blocks can also hold plates
    for (auto &b : objects) {
      if (b.type != ObjType::PUSHABLE_BLOCK) continue;
      if (std::abs((b.rect.y + b.rect.height) - o.rect.y) <= 3.0f &&
          b.rect.x + b.rect.width > o.rect.x && b.rect.x < o.rect.x + o.rect.width)
        pressed = true;
    }
    o.active = pressed;
  }

  // 3. Lever: proximity interact
  for (auto &o : objects) {
    if (o.type != ObjType::LEVER) continue;
    Rectangle zone = {o.rect.x - 12, o.rect.y - 12,
                      o.rect.width + 24, o.rect.height + 24};
    if (CheckCollisionRecs(p.getBody(),  zone) && p.isInteracting())  o.active = !o.active;
    if (CheckCollisionRecs(p2.getBody(), zone) && p2.isInteracting()) o.active = !o.active;
  }

  // 4. Gate: open when any linked plate/lever is active
  for (auto &gate : objects) {
    if (gate.type != ObjType::GATE) continue;
    bool open = false;
    for (auto &src : objects) {
      if (src.linkId != gate.linkId) continue;
      if ((src.type == ObjType::PRESSURE_PLATE || src.type == ObjType::LEVER) && src.active)
        { open = true; break; }
    }
    gate.active = open;
  }

  // 5. Falling platforms: trigger countdown when player stands on them
  for (auto &o : objects) {
    if (o.type != ObjType::FALLING_PLATFORM || o.falling) continue;
    if (o.fallTimer < 0.0f) {
      for (Entity *e : {&p, &p2}) {
        Rectangle pb = e->getBody();
        if (std::abs((pb.y + pb.height) - o.rect.y) <= 3.0f &&
            pb.x + pb.width > o.rect.x && pb.x < o.rect.x + o.rect.width)
          o.fallTimer = 0.5f;
      }
    } else {
      o.fallTimer -= dt;
      if (o.fallTimer <= 0.0f) o.falling = true;
    }
  }
  // Advance falling
  for (auto &o : objects) {
    if (o.type == ObjType::FALLING_PLATFORM && o.falling) {
      o.velY += 500.0f * dt;
      o.rect.y += o.velY * dt;
    }
  }

  // 6. Pushable blocks: gravity + solid resolution
  for (auto &o : objects) {
    if (o.type != ObjType::PUSHABLE_BLOCK) continue;
    o.velY += 500.0f * dt;
    o.rect.y += o.velY * dt;
    o.grounded = false;
    for (auto &s : solids) {
      if (!CheckCollisionRecs(o.rect, s)) continue;
      Rectangle ov = GetCollisionRec(o.rect, s);
      if (o.velY >= 0) { o.rect.y -= ov.height; o.velY = 0; o.grounded = true; }
      else             { o.rect.y += ov.height; o.velY = 0; }
    }
  }

  // 7. Pushable block <-> player push resolution
  for (auto &o : objects) {
    if (o.type != ObjType::PUSHABLE_BLOCK) continue;
    for (Entity *e : {&p, &p2}) {
      Rectangle pb = e->getBody();
      if (!CheckCollisionRecs(pb, o.rect)) continue;
      Rectangle ov = GetCollisionRec(pb, o.rect);

      if (ov.height <= ov.width) {
        // Vertical: player landed on top of block (or block rose into player)
        float eCy = pb.y + pb.height * 0.5f;
        float oCy = o.rect.y + o.rect.height * 0.5f;
        if (eCy < oCy) { // player above block
          e->nudge({0, -ov.height});
          e->stopY();
        }
        continue;
      }

      // Horizontal push
      float eCx = pb.x + pb.width * 0.5f;
      float oCx = o.rect.x + o.rect.width * 0.5f;
      float dx  = (eCx < oCx) ? ov.width : -ov.width;
      bool pushingThisWay = (dx > 0) ? e->isPushingRight() : e->isPushingLeft();
      if (!pushingThisWay) { e->nudge({-dx, 0}); e->stopX(); continue; }

      // Try to move block
      Rectangle test = {o.rect.x + dx, o.rect.y, o.rect.width, o.rect.height};
      bool blocked = false;
      for (auto &s : solids)
        if (CheckCollisionRecs(test, s)) { blocked = true; break; }
      for (auto &b : objects) // block vs block
        if (&b != &o && b.type == ObjType::PUSHABLE_BLOCK && CheckCollisionRecs(test, b.rect))
          { blocked = true; break; }

      if (!blocked) o.rect.x += dx;
      else { e->nudge({-dx, 0}); e->stopX(); }
    }
  }
}

static void drawObjArrows(Rectangle rect, float dirX, Color col) {
  int n = std::max(1, (int)(rect.width / 12));
  for (int i = 0; i < n; i++) {
    float ax = rect.x + (i + 0.5f) * rect.width / n;
    float ay = rect.y + rect.height * 0.5f;
    DrawTriangle({ax + dirX*5, ay},
                 {ax - dirX*3, ay - 4},
                 {ax - dirX*3, ay + 4}, col);
  }
}

void Game::drawObjects() {
  char buf[4];
  for (auto &o : objects) {
    if (o.type == ObjType::FALLING_PLATFORM && o.rect.y > GetScreenHeight() + 64) continue;

    Color col = objColor(o.type, o.active);
    DrawRectangleRec(o.rect, col);

    switch (o.type) {
    case ObjType::PRESSURE_PLATE:
      DrawRectangleLinesEx(o.rect, 2, o.active ? YELLOW : DARKGRAY);
      snprintf(buf, sizeof(buf), "%d", o.linkId);
      DrawText(buf, (int)o.rect.x + 3, (int)o.rect.y + 3, 11, BLACK);
      break;
    case ObjType::LEVER: {
      float cx = o.rect.x + o.rect.width*0.5f, cy = o.rect.y + o.rect.height*0.5f;
      float ang = (o.active ? -0.5f : 0.5f);
      DrawLineEx({cx, cy}, {cx + sinf(ang)*o.rect.height*0.5f,
                             cy - cosf(ang)*o.rect.height*0.5f}, 4, o.active ? YELLOW : LIGHTGRAY);
      snprintf(buf, sizeof(buf), "%d", o.linkId);
      DrawText(buf, (int)o.rect.x, (int)o.rect.y - 13, 11, WHITE);
      break;
    }
    case ObjType::GATE:
      if (!o.active) {
        DrawRectangleLinesEx(o.rect, 2, WHITE);
        for (int i = 1; i <= 3; i++) {
          float bx = o.rect.x + o.rect.width * i / 4.0f;
          DrawRectangle((int)bx - 2, (int)o.rect.y, 4, (int)o.rect.height, {50,90,200,255});
        }
      } else {
        // Shimmer effect when gate is open (transitioning)
        float shimmer = std::sin((float)GetTime() * 8.0f) * 0.5f + 0.5f;
        unsigned char sa = (unsigned char)(shimmer * 80);
        DrawRectangleRec(o.rect, {180, 220, 255, sa});
      }
      snprintf(buf, sizeof(buf), "%d", o.linkId);
      DrawText(buf, (int)o.rect.x + 3, (int)o.rect.y + 3, 11, WHITE);
      break;
    case ObjType::MOVING_PLATFORM: {
      // Bobbing shadow beneath platform
      DrawEllipse((int)(o.rect.x + o.rect.width * 0.5f),
                  (int)(o.rect.y + o.rect.height + 4),
                  o.rect.width * 0.4f, 3.0f, {0, 0, 0, 50});
      float dx = o.endpoint.x - o.origin.x;
      float dy = o.endpoint.y - o.origin.y;
      float len = sqrtf(dx*dx + dy*dy);
      if (len > 0) {
        DrawTriangle({o.rect.x + o.rect.width*0.5f + dx/len*8, o.rect.y + o.rect.height*0.5f},
                     {o.rect.x + o.rect.width*0.5f - dx/len*5, o.rect.y + o.rect.height*0.5f - 5},
                     {o.rect.x + o.rect.width*0.5f - dx/len*5, o.rect.y + o.rect.height*0.5f + 5},
                     {40,160,40,200});
      }
      break;
    }
    case ObjType::CONVEYOR_LEFT:
      drawObjArrows(o.rect, -1.0f, {200,100,220,200}); break;
    case ObjType::CONVEYOR_RIGHT:
      drawObjArrows(o.rect,  1.0f, {200,100,220,200}); break;
    case ObjType::FALLING_PLATFORM:
      if (o.fallTimer >= 0.0f && !o.falling) {
        int alpha = (int)(o.fallTimer / 0.5f * 180);
        DrawRectangleRec(o.rect, {255, 80, 40, (unsigned char)alpha});
      }
      break;
    case ObjType::PUSHABLE_BLOCK:
      DrawLine((int)o.rect.x, (int)o.rect.y, (int)(o.rect.x+o.rect.width), (int)(o.rect.y+o.rect.height), {90,90,100,180});
      DrawLine((int)(o.rect.x+o.rect.width), (int)o.rect.y, (int)o.rect.x, (int)(o.rect.y+o.rect.height), {90,90,100,180});
      break;
    case ObjType::CRUSHER:
      // X marks the deadly block
      DrawLine((int)o.rect.x,              (int)o.rect.y,              (int)(o.rect.x+o.rect.width), (int)(o.rect.y+o.rect.height), {255,80,80,200});
      DrawLine((int)(o.rect.x+o.rect.width),(int)o.rect.y,             (int)o.rect.x,                (int)(o.rect.y+o.rect.height), {255,80,80,200});
      DrawRectangleLinesEx(o.rect, 2, {255, 60, 60, 255});
      break;
    case ObjType::ELECTRIC:
      DrawRectangleLinesEx(o.rect, 1, o.active ? Color{255,255,100,255} : Color{60,60,20,255});
      if (o.active) {
        // Animated crackle: small bright highlight sweeping across
        float phase = o.tParam / (o.speed * 0.5f); // 0..1 within active half
        float bx = o.rect.x + phase * (o.rect.width - 4);
        DrawRectangle((int)bx, (int)o.rect.y, 4, (int)o.rect.height, {255, 255, 220, 180});
        // Random jagged crackle lines
        for (int j = 0; j < 4; j++) {
          int x1 = GetRandomValue((int)o.rect.x, (int)(o.rect.x + o.rect.width));
          int y1 = GetRandomValue((int)o.rect.y, (int)(o.rect.y + o.rect.height));
          int x2 = x1 + GetRandomValue(-6, 6);
          int y2 = y1 + GetRandomValue(-4, 4);
          DrawLine(x1, y1, x2, y2, {255, 255, 200, 220});
        }
      }
      break;
    default: break;
    }
  }
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
