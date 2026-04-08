#include "../include/leveleditor.hpp"
#include "../include/json.hpp"
#include "../include/raylib/raymath.h"
#include <algorithm>
#include <fstream>
using json = nlohmann::json;

constexpr int TS = (int)Config::TILESIZE;

// Tile type shortcuts for toolbar display
static const char* TILE_SHORTCUTS = "1:Solid 2:Fire 3:Water 4:Exit1 5:Exit2 6:Spike";
static const char* OBJ_SHORTCUTS = "1:Plate 2:Lever 3:Gate 4:MvPlat 5:Conv< 6:Conv> 7:Fall 8:Block 9:Sp1 0:Sp2 Q:Crush E:Elec";

static const char* COLORBLIND_NAMES[] = {"none", "deut", "prot", "trit", "achro"};

LevelEditor::LevelEditor(const std::string &path) : levelPath(path) { load(); }

// ---- Tile Helpers ----------------------------------------------------------

bool LevelEditor::hasTile(int x, int y) const {
  for (const auto &t : tiles)
    if (t.x == x && t.y == y) return true;
  return false;
}

void LevelEditor::removeTile(int x, int y) {
  tiles.erase(std::remove_if(tiles.begin(), tiles.end(),
    [x, y](const EditorTile &t) { return t.x == x && t.y == y; }), tiles.end());
}

void LevelEditor::placeTile(int x, int y, TileType type) {
  removeTile(x, y);
  tiles.push_back({x, y, type});
}

// ---- Object Helpers --------------------------------------------------------

void LevelEditor::removeObjectAt(int tx, int ty) {
  objects.erase(std::remove_if(objects.begin(), objects.end(),
    [tx, ty](const EditorObject &o) { return o.tx == tx && o.ty == ty; }), objects.end());
}

EditorObject* LevelEditor::findObjectAt(int tx, int ty) {
  for (auto &o : objects)
    if (o.tx == tx && o.ty == ty) return &o;
  return nullptr;
}

bool LevelEditor::isTwoClickObject(ObjType type) const {
  return type == ObjType::MOVING_PLATFORM || type == ObjType::CRUSHER;
}

bool LevelEditor::isSpawnType(ObjType type) const {
  return type == ObjType::SPAWN_P1 || type == ObjType::SPAWN_P2;
}

void LevelEditor::handleTwoClickFirst(int tx, int ty) {
  awaitingSecondClick = true;
  firstClickX = tx;
  firstClickY = ty;
}

void LevelEditor::handleTwoClickSecond(int tx, int ty) {
  removeObjectAt(firstClickX, firstClickY);
  objects.push_back({selectedObj, firstClickX, firstClickY, linkId, tx, ty});
  awaitingSecondClick = false;
}

void LevelEditor::placeSpawn(int tx, int ty) {
  // Remove existing spawn of same type (only one per player)
  objects.erase(std::remove_if(objects.begin(), objects.end(),
    [this](const EditorObject &o) { return o.type == selectedObj; }), objects.end());
  objects.push_back({selectedObj, tx, ty, 0, -1, -1});
}

void LevelEditor::placeObject(int tx, int ty) {
  if (isTwoClickObject(selectedObj)) {
    if (!awaitingSecondClick) {
      handleTwoClickFirst(tx, ty);
    } else {
      handleTwoClickSecond(tx, ty);
    }
  } else if (isSpawnType(selectedObj)) {
    placeSpawn(tx, ty);
  } else {
    removeObjectAt(tx, ty);
    objects.push_back({selectedObj, tx, ty, linkId, -1, -1});
  }
}

// ---- Undo ------------------------------------------------------------------

void LevelEditor::saveUndoState() {
  undoTiles = tiles;
  undoObjects = objects;
}

// ---- Save / Load -----------------------------------------------------------

void LevelEditor::save() {
  json j;
  if (levelCbMode != 0) j["colorblind"] = levelCbMode;

  j["tiles"] = json::array();
  for (const auto &t : tiles)
    j["tiles"].push_back({{"x", t.x}, {"y", t.y}, {"type", (int)t.type}});

  j["objects"] = json::array();
  for (const auto &o : objects) {
    json entry = {{"type", (int)o.type}, {"x", o.tx}, {"y", o.ty}};
    if (o.linkId != 0) entry["link"] = o.linkId;
    if (o.etx >= 0)    entry["ex"] = o.etx;
    if (o.ety >= 0)    entry["ey"] = o.ety;
    j["objects"].push_back(entry);
  }

  std::ofstream f(levelPath);
  f << j.dump(2);
  savedTimer = 2.5f;
}

void LevelEditor::load() {
  tiles.clear();
  objects.clear();
  std::ifstream f(levelPath);
  if (!f.is_open() || f.peek() == std::ifstream::traits_type::eof()) return;
  try {
    json j = json::parse(f);
    levelCbMode = j.value("colorblind", 0);

    for (const auto &t : j["tiles"]) {
      int typeInt = t.value("type", 0);
      tiles.push_back({t["x"].get<int>(), t["y"].get<int>(), static_cast<TileType>(typeInt)});
    }

    if (j.contains("objects")) {
      for (const auto &o : j["objects"]) {
        EditorObject eo{};
        eo.type   = static_cast<ObjType>(o["type"].get<int>());
        eo.tx     = o["x"].get<int>();
        eo.ty     = o["y"].get<int>();
        eo.linkId = o.value("link", 0);
        eo.etx    = o.value("ex", -1);
        eo.ety    = o.value("ey", -1);
        objects.push_back(eo);
      }
    }
  } catch (...) {}
}

// ---- Update ----------------------------------------------------------------

void LevelEditor::update() {
  exitRequested = false;

  const int W = GetScreenWidth();
  const int H = GetScreenHeight();
  const int COLS = W / TS;
  const int ROWS = H / TS;

  Vector2 mouse = GetMousePosition();
  int gx = Clamp((int)(mouse.x / TS), 0, COLS - 1);
  int gy = Clamp((int)(mouse.y / TS), 0, ROWS - 1);

  // Toggle tile/object mode
  if (IsKeyPressed(KEY_TAB)) {
    objMode = !objMode;
    awaitingSecondClick = false;
  }

  // Toggle help panel
  if (IsKeyPressed(KEY_H)) showHelp = !showHelp;

  if (!objMode) {
    // --- Tile Mode ---
    if (IsKeyPressed(KEY_ONE))   selectedTile = TileType::SOLID;
    if (IsKeyPressed(KEY_TWO))   selectedTile = TileType::FIRE;
    if (IsKeyPressed(KEY_THREE)) selectedTile = TileType::WATER;
    if (IsKeyPressed(KEY_FOUR))  selectedTile = TileType::EXIT_P1;
    if (IsKeyPressed(KEY_FIVE))  selectedTile = TileType::EXIT_P2;
    if (IsKeyPressed(KEY_SIX))   selectedTile = TileType::SPIKE;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      saveUndoState();
      placeTile(gx, gy, selectedTile);
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      placeTile(gx, gy, selectedTile);
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
      saveUndoState();
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
      removeTile(gx, gy);
    }

    // Fill tool (SHIFT + LMB)
    if (IsKeyDown(KEY_LEFT_SHIFT) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      // Fill current tile and all connected empty tiles of same type
      // (simplified: just fill a 3x3 area for now)
      for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
          placeTile(gx + dx, gy + dy, selectedTile);
        }
      }
    }
  } else {
    // --- Object Mode ---
    if (IsKeyPressed(KEY_ONE))   selectedObj = ObjType::PRESSURE_PLATE;
    if (IsKeyPressed(KEY_TWO))   selectedObj = ObjType::LEVER;
    if (IsKeyPressed(KEY_THREE)) selectedObj = ObjType::GATE;
    if (IsKeyPressed(KEY_FOUR))  selectedObj = ObjType::MOVING_PLATFORM;
    if (IsKeyPressed(KEY_FIVE))  selectedObj = ObjType::CONVEYOR_LEFT;
    if (IsKeyPressed(KEY_SIX))   selectedObj = ObjType::CONVEYOR_RIGHT;
    if (IsKeyPressed(KEY_SEVEN)) selectedObj = ObjType::FALLING_PLATFORM;
    if (IsKeyPressed(KEY_EIGHT)) selectedObj = ObjType::PUSHABLE_BLOCK;
    if (IsKeyPressed(KEY_NINE))  selectedObj = ObjType::SPAWN_P1;
    if (IsKeyPressed(KEY_ZERO))  selectedObj = ObjType::SPAWN_P2;
    if (IsKeyPressed(KEY_Q))     selectedObj = ObjType::CRUSHER;
    if (IsKeyPressed(KEY_E))     selectedObj = ObjType::ELECTRIC;

    // Link ID adjustment
    if (IsKeyPressed(KEY_LEFT_BRACKET))  linkId = std::max(0, linkId - 1);
    if (IsKeyPressed(KEY_RIGHT_BRACKET)) linkId = std::min(9, linkId + 1);

    // Cancel two-click placement with right click or ESC handled elsewhere
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
      if (awaitingSecondClick) {
        awaitingSecondClick = false;
      } else {
        saveUndoState();
        removeObjectAt(gx, gy);
      }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      saveUndoState();
      placeObject(gx, gy);
    }
  }

  // Global shortcuts
  if (IsKeyPressed(KEY_S)) { saveUndoState(); save(); }
  if (IsKeyPressed(KEY_F5)) { saveUndoState(); save(); }
  if (IsKeyPressed(KEY_L)) load();
  if (IsKeyPressed(KEY_Z) && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))) {
    tiles = undoTiles;
    objects = undoObjects;
  }
  if (IsKeyPressed(KEY_C)) {
    saveUndoState();
    tiles.clear();
    objects.clear();
    awaitingSecondClick = false;
  }
  if (IsKeyPressed(KEY_M)) levelCbMode = (levelCbMode + 1) % 5;
  if (IsKeyPressed(KEY_ESCAPE)) {
    if (awaitingSecondClick) {
      awaitingSecondClick = false;
    } else {
      exitRequested = true;
    }
  }

  savedTimer -= GetFrameTime();
}

// ---- Drawing ---------------------------------------------------------------

void LevelEditor::drawTilePreview(int gridX, int gridY) {
  Color previewColor = tileColor(selectedTile);
  previewColor.a = 100;
  DrawRectangle(gridX * TS, gridY * TS, TS, TS, previewColor);
  DrawRectangleLines(gridX * TS, gridY * TS, TS, TS, DARKGRAY);
}

void LevelEditor::drawObject(const EditorObject &o) {
  Color col = objColor(o.type);
  int ox = o.tx * TS, oy = o.ty * TS;

  // Draw path line for two-click objects
  if (isTwoClickObject(o.type) && o.etx >= 0) {
    DrawLine(o.tx * TS + TS / 2, o.ty * TS + TS / 4,
             o.etx * TS + TS / 2, o.ety * TS + TS / 4, {100, 200, 100, 180});
  }

  switch (o.type) {
  case ObjType::PRESSURE_PLATE:
    DrawRectangle(ox, oy, TS, TS, col);
    break;
  case ObjType::LEVER:
    DrawRectangle(ox + TS / 3, oy + TS / 10, TS / 3, TS * 7 / 10, col);
    break;
  case ObjType::GATE:
    DrawRectangle(ox, oy, TS, TS * 2, col);
    break;
  case ObjType::MOVING_PLATFORM:
  case ObjType::CONVEYOR_LEFT:
  case ObjType::CONVEYOR_RIGHT:
  case ObjType::FALLING_PLATFORM:
    DrawRectangle(ox, oy, TS, TS * 2 / 5, col);
    break;
  case ObjType::PUSHABLE_BLOCK:
    DrawRectangle(ox, oy, TS, TS, col);
    break;
  case ObjType::SPAWN_P1:
  case ObjType::SPAWN_P2:
    DrawCircle(ox + TS / 2, oy + TS / 2, TS * 0.4f, col);
    DrawText(o.type == ObjType::SPAWN_P1 ? "P1" : "P2", ox + TS / 2 - 7, oy + TS / 2 - 6, 12, WHITE);
    break;
  case ObjType::CRUSHER:
    DrawRectangle(ox, oy, TS, TS, col);
    DrawLine(ox, oy, ox + TS, oy + TS, {255, 80, 80, 200});
    DrawLine(ox + TS, oy, ox, oy + TS, {255, 80, 80, 200});
    break;
  case ObjType::ELECTRIC:
    DrawRectangle(ox, oy, TS, TS / 4, col);
    break;
  default:
    DrawRectangle(ox, oy, TS, TS, col);
    break;
  }

  // Link ID overlay for linkable objects
  if (o.type == ObjType::PRESSURE_PLATE || o.type == ObjType::LEVER || o.type == ObjType::GATE) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", o.linkId);
    DrawText(buf, ox + 3, oy + 3, 12, BLACK);
  }

  // Direction arrows for conveyors
  if (o.type == ObjType::CONVEYOR_LEFT)  DrawText("<", ox + TS / 3, oy + 1, 11, WHITE);
  if (o.type == ObjType::CONVEYOR_RIGHT) DrawText(">", ox + TS / 3, oy + 1, 11, WHITE);
}

void LevelEditor::drawToolbar() {
  const int W = GetScreenWidth();

  // Header bar background
  DrawRectangle(0, 0, W, 22, {0, 0, 0, 200});

  // Mode indicator and selected item
  const char *modeText = objMode ? "OBJECT MODE" : "TILE MODE";
  DrawText(modeText, 4, 4, 12, objMode ? SKYBLUE : YELLOW);

  // Level name (centered)
  std::string name = levelPath;
  auto slash = name.find_last_of("/\\");
  if (slash != std::string::npos) name = name.substr(slash + 1);
  DrawText(name.c_str(), W / 2 - MeasureText(name.c_str(), 12) / 2, 4, 12, LIGHTGRAY);

  // Selected item info
  char selBuf[64];
  if (!objMode) {
    snprintf(selBuf, sizeof(selBuf), "[%s]", tileName(selectedTile));
    DrawText(selBuf, W - MeasureText(selBuf, 13) - 4, 4, 13, tileColor(selectedTile));
  } else {
    snprintf(selBuf, sizeof(selBuf), "[%s  link:%d]", objTypeName(selectedObj), linkId);
    DrawText(selBuf, W - MeasureText(selBuf, 13) - 4, 4, 13, objColor(selectedObj));
  }

  // Colorblind mode indicator
  if (levelCbMode != 0) {
    char cbBuf[32];
    snprintf(cbBuf, sizeof(cbBuf), "CB:%s", COLORBLIND_NAMES[levelCbMode]);
    DrawText(cbBuf, W / 2 - MeasureText(cbBuf, 12) / 2 + 80, 4, 12, YELLOW);
  }

  // Saved indicator
  if (savedTimer > 0) {
    DrawText("Saved!", W - 65, 25, 14, GREEN);
  }
}

void LevelEditor::drawHelpPanel() {
  const int W = GetScreenWidth();
  const int H = GetScreenHeight();

  // Semi-transparent overlay
  DrawRectangle(W / 2 - 200, H / 2 - 180, 400, 360, {0, 0, 0, 220});
  DrawRectangleLines(W / 2 - 200, H / 2 - 180, 400, 360, YELLOW);

  int y = H / 2 - 165;
  const int LINE_H = 18;

  DrawText("CONTROLS (H to toggle)", W / 2 - 90, y, 14, YELLOW);
  y += LINE_H * 2;

  DrawText("TAB         Switch Tile/Object mode", W / 2 - 190, y, 12, WHITE);
  y += LINE_H;
  DrawText("H           Toggle this help", W / 2 - 190, y, 12, WHITE);
  y += LINE_H;
  DrawText("S / F5      Save level", W / 2 - 190, y, 12, WHITE);
  y += LINE_H;
  DrawText("L           Reload level", W / 2 - 190, y, 12, WHITE);
  y += LINE_H;
  DrawText("C           Clear all", W / 2 - 190, y, 12, WHITE);
  y += LINE_H;
  DrawText("M           Cycle colorblind mode", W / 2 - 190, y, 12, WHITE);
  y += LINE_H;
  DrawText("Ctrl+Z      Undo", W / 2 - 190, y, 12, WHITE);
  y += LINE_H * 2;

  DrawText("--- TILE MODE ---", W / 2 - 85, y, 13, YELLOW);
  y += LINE_H;
  DrawText(TILE_SHORTCUTS, W / 2 - 190, y, 11, LIGHTGRAY);
  y += LINE_H;
  DrawText("LMB: Place tile  RMB: Erase tile", W / 2 - 190, y, 11, WHITE);
  y += LINE_H;
  DrawText("Shift+LMB: Fill 3x3 area", W / 2 - 190, y, 11, WHITE);
  y += LINE_H * 2;

  DrawText("--- OBJECT MODE ---", W / 2 - 95, y, 13, YELLOW);
  y += LINE_H;
  DrawText(OBJ_SHORTCUTS, W / 2 - 190, y, 10, LIGHTGRAY);
  y += LINE_H;
  DrawText("[ / ]        Change link ID", W / 2 - 190, y, 11, WHITE);
  y += LINE_H;
  DrawText("LMB: Place object  RMB: Erase", W / 2 - 190, y, 11, WHITE);
  y += LINE_H;
  DrawText("Moving platforms/crushers: 2 clicks", W / 2 - 190, y, 11, WHITE);
  y += LINE_H * 2;

  DrawText("ESC: Exit editor", W / 2 - 85, y, 12, GRAY);
}

void LevelEditor::draw() {
  const int W = GetScreenWidth();
  const int H = GetScreenHeight();

  ClearBackground(WHITE);

  // Draw grid
  for (int x = 0; x <= W; x += TS) DrawLine(x, 0, x, H, {200, 200, 200, 255});
  for (int y = 0; y <= H; y += TS) DrawLine(0, y, W, y, {200, 200, 200, 255});

  // Draw tiles
  for (const auto &t : tiles) {
    DrawRectangle(t.x * TS, t.y * TS, TS, TS, tileColor(t.type));
  }

  // Draw objects
  for (const auto &o : objects) {
    drawObject(o);
  }

  // Draw pending two-click object (first click placed, awaiting second)
  if (awaitingSecondClick) {
    Color pendingColor = (selectedObj == ObjType::CRUSHER) ? Color{255, 80, 80, 180} : Color{100, 255, 100, 180};
    int pendingHeight = (selectedObj == ObjType::CRUSHER) ? TS : TS * 2 / 5;
    DrawRectangle(firstClickX * TS, firstClickY * TS, TS, pendingHeight, pendingColor);
    DrawText("Click to set endpoint (ESC/RMB to cancel)", firstClickX * TS, firstClickY * TS - 18, 12, GREEN);
  }

  // Draw hover preview
  Vector2 mouse = GetMousePosition();
  int gx = Clamp((int)(mouse.x / TS), 0, W / TS - 1);
  int gy = Clamp((int)(mouse.y / TS), 0, H / TS - 1);

  if (!objMode) {
    drawTilePreview(gx, gy);
  } else {
    Color previewColor = objColor(selectedObj);
    previewColor.a = 100;
    DrawRectangle(gx * TS, gy * TS, TS, TS, previewColor);
    DrawRectangleLines(gx * TS, gy * TS, TS, TS, DARKGRAY);
  }

  // Draw UI
  drawToolbar();
  if (showHelp) drawHelpPanel();
}