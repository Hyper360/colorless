#include "../include/leveleditor.hpp"
#include "../include/json.hpp"
#include "../include/raylib/raymath.h"
#include <algorithm>
#include <fstream>
using json = nlohmann::json;

const int TS = (int)Config::TILESIZE;

LevelEditor::LevelEditor(const std::string &path) : levelPath(path) { load(); }

// ---- Tile helpers ----------------------------------------------------------

bool LevelEditor::hasTile(int x, int y) const {
  for (auto &t : tiles)
    if (t.x == x && t.y == y) return true;
  return false;
}

void LevelEditor::removeTile(int x, int y) {
  tiles.erase(std::remove_if(tiles.begin(), tiles.end(),
                             [x,y](const EditorTile &t){ return t.x==x && t.y==y; }),
              tiles.end());
}

// ---- Object helpers --------------------------------------------------------

void LevelEditor::removeObjectAt(int tx, int ty) {
  objects.erase(std::remove_if(objects.begin(), objects.end(),
                               [tx,ty](const EditorObject &o){ return o.tx==tx && o.ty==ty; }),
                objects.end());
}

// ---- Save / Load -----------------------------------------------------------

void LevelEditor::save() {
  json j;
  j["tiles"] = json::array();
  for (auto &t : tiles)
    j["tiles"].push_back({{"x", t.x}, {"y", t.y}, {"type", (int)t.type}});

  j["objects"] = json::array();
  for (auto &o : objects) {
    json entry = {{"type", (int)o.type}, {"x", o.tx}, {"y", o.ty}};
    if (o.linkId != 0) entry["link"] = o.linkId;
    if (o.etx >= 0)    entry["ex"]   = o.etx;
    if (o.ety >= 0)    entry["ey"]   = o.ety;
    j["objects"].push_back(entry);
  }

  std::ofstream f(levelPath);
  f << j.dump(2);
  savedTimer = 2.0f;
}

void LevelEditor::load() {
  tiles.clear();
  objects.clear();
  std::ifstream f(levelPath);
  if (!f.is_open() || f.peek() == std::ifstream::traits_type::eof()) return;
  try {
    json j = json::parse(f);
    for (auto &t : j["tiles"]) {
      TileType type = TileType::SOLID;
      if (t.contains("type")) type = static_cast<TileType>(t["type"].get<int>());
      tiles.push_back({t["x"].get<int>(), t["y"].get<int>(), type});
    }
    if (j.contains("objects")) {
      for (auto &o : j["objects"]) {
        EditorObject eo{};
        eo.type   = static_cast<ObjType>(o["type"].get<int>());
        eo.tx     = o["x"].get<int>();
        eo.ty     = o["y"].get<int>();
        eo.linkId = o.contains("link") ? o["link"].get<int>() : 0;
        eo.etx    = o.contains("ex")   ? o["ex"].get<int>()   : -1;
        eo.ety    = o.contains("ey")   ? o["ey"].get<int>()   : -1;
        objects.push_back(eo);
      }
    }
  } catch (...) {}
}

// ---- Update ----------------------------------------------------------------

void LevelEditor::update() {
  exitRequested = false;

  const int W    = GetScreenWidth();
  const int H    = GetScreenHeight();
  const int COLS = W / TS;
  const int ROWS = H / TS;

  Vector2 mouse = GetMousePosition();
  int gx = Clamp((int)(mouse.x / TS), 0, COLS - 1);
  int gy = Clamp((int)(mouse.y / TS), 0, ROWS - 1);

  // Tab: toggle tile / object mode
  if (IsKeyPressed(KEY_TAB)) { objMode = !objMode; pendingMP = false; }

  if (!objMode) {
    // ---- Tile mode ----
    if (IsKeyPressed(KEY_ONE))   selectedType = TileType::SOLID;
    if (IsKeyPressed(KEY_TWO))   selectedType = TileType::FIRE;
    if (IsKeyPressed(KEY_THREE)) selectedType = TileType::WATER;
    if (IsKeyPressed(KEY_FOUR))  selectedType = TileType::EXIT_P1;
    if (IsKeyPressed(KEY_FIVE))  selectedType = TileType::EXIT_P2;

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))  { removeTile(gx, gy); tiles.push_back({gx, gy, selectedType}); }
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) removeTile(gx, gy);
  } else {
    // ---- Object mode ----
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

    // Link ID: [ and ]
    if (IsKeyPressed(KEY_LEFT_BRACKET))  linkId = std::max(0, linkId - 1);
    if (IsKeyPressed(KEY_RIGHT_BRACKET)) linkId = std::min(9, linkId + 1);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      if (selectedObj == ObjType::MOVING_PLATFORM) {
        if (!pendingMP) {
          pendingMP = true; mpTX = gx; mpTY = gy;
        } else {
          EditorObject o{ObjType::MOVING_PLATFORM, mpTX, mpTY, linkId, gx, gy};
          removeObjectAt(mpTX, mpTY);
          objects.push_back(o);
          pendingMP = false;
        }
      } else if (selectedObj == ObjType::SPAWN_P1 || selectedObj == ObjType::SPAWN_P2) {
        // Only one spawn per player — remove any existing ones of this type first
        objects.erase(std::remove_if(objects.begin(), objects.end(),
          [this](const EditorObject &o){ return o.type == selectedObj; }), objects.end());
        objects.push_back({selectedObj, gx, gy, 0, -1, -1});
      } else {
        EditorObject o{selectedObj, gx, gy, linkId, -1, -1};
        removeObjectAt(gx, gy);
        objects.push_back(o);
      }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
      removeObjectAt(gx, gy);
      if (pendingMP) pendingMP = false;
    }
  }

  // Shared keys
  if (IsKeyPressed(KEY_S) && !objMode) save();      // S only in tile mode (avoid conflict with p1 interact)
  if (IsKeyPressed(KEY_F5))            save();       // F5 always saves
  if (IsKeyPressed(KEY_L))             load();
  if (IsKeyPressed(KEY_C))           { tiles.clear(); objects.clear(); }
  if (IsKeyPressed(KEY_ESCAPE))        exitRequested = true;

  savedTimer -= GetFrameTime();
}

// ---- Draw ------------------------------------------------------------------

void LevelEditor::draw() {
  const int W = GetScreenWidth();
  const int H = GetScreenHeight();

  ClearBackground(WHITE);

  // Tiles
  for (auto &t : tiles)
    DrawRectangle(t.x * TS, t.y * TS, TS, TS, tileColor(t.type));

  // Objects
  for (auto &o : objects) {
    Color col = objColor(o.type);

    if (o.type == ObjType::MOVING_PLATFORM && o.etx >= 0) {
      // Draw path line
      DrawLine(o.tx*TS + TS/2, o.ty*TS + TS/4,
               o.etx*TS + TS/2, o.ety*TS + TS/4, {100,200,100,180});
    }

    // Draw object rect approximation
    int ox = o.tx * TS, oy = o.ty * TS;
    switch (o.type) {
    case ObjType::PRESSURE_PLATE:   DrawRectangle(ox, oy, TS, TS, col); break;
    case ObjType::LEVER:            DrawRectangle(ox + TS/3, oy + TS/10, TS/3, TS*7/10, col); break;
    case ObjType::GATE:             DrawRectangle(ox, oy, TS, TS*2, col); break;
    case ObjType::MOVING_PLATFORM:
    case ObjType::CONVEYOR_LEFT:
    case ObjType::CONVEYOR_RIGHT:
    case ObjType::FALLING_PLATFORM: DrawRectangle(ox, oy, TS, TS*2/5, col); break;
    case ObjType::PUSHABLE_BLOCK:   DrawRectangle(ox, oy, TS, TS, col); break;
    case ObjType::SPAWN_P1:
    case ObjType::SPAWN_P2:
      DrawCircle(ox + TS/2, oy + TS/2, TS*0.4f, col);
      DrawText(o.type == ObjType::SPAWN_P1 ? "P1" : "P2",
               ox + TS/2 - 7, oy + TS/2 - 6, 12, WHITE);
      break;
    default: DrawRectangle(ox, oy, TS, TS, col); break;
    }

    // Link ID overlay
    if (o.type == ObjType::PRESSURE_PLATE || o.type == ObjType::LEVER || o.type == ObjType::GATE) {
      char buf[4]; snprintf(buf, sizeof(buf), "%d", o.linkId);
      DrawText(buf, ox + 3, oy + 3, 12, BLACK);
    }
    // Conveyor direction
    if (o.type == ObjType::CONVEYOR_LEFT)  DrawText("<", ox + TS/3, oy + 1, 11, WHITE);
    if (o.type == ObjType::CONVEYOR_RIGHT) DrawText(">", ox + TS/3, oy + 1, 11, WHITE);
  }

  // Pending moving platform origin
  if (pendingMP) {
    DrawRectangle(mpTX*TS, mpTY*TS, TS, TS*2/5, {100,255,100,180});
    DrawText("2nd click = endpoint", mpTX*TS, mpTY*TS - 16, 13, GREEN);
  }

  // Grid
  for (int x = 0; x <= W; x += TS) DrawLine(x, 0, x, H, {180,180,180,255});
  for (int y = 0; y <= H; y += TS) DrawLine(0, y, W, y, {180,180,180,255});

  // Hover highlight
  Vector2 mouse = GetMousePosition();
  int hx = (int)(mouse.x / TS) * TS, hy = (int)(mouse.y / TS) * TS;
  if (!objMode) {
    Color hc = tileColor(selectedType); hc.a = 120;
    DrawRectangle(hx, hy, TS, TS, hc);
  } else {
    Color hc = objColor(selectedObj); hc.a = 120;
    DrawRectangle(hx, hy, TS, TS, hc);
  }

  // Toolbar
  DrawRectangle(0, 0, W, 22, {0,0,0,190});
  if (!objMode) {
    DrawText("LMB:Place RMB:Erase C:Clear S/F5:Save L:Load ESC:Back | 1-5:Tile TAB:ObjMode",
             4, 4, 12, WHITE);
    const char *tn = tileName(selectedType);
    std::string sel = std::string("[ ") + tn + " ]";
    DrawText(sel.c_str(), W - MeasureText(sel.c_str(), 13) - 4, 4, 13, tileColor(selectedType));
  } else {
    DrawText("LMB:Place RMB:Erase C:Clear F5:Save L:Load ESC:Back | 1-8:Obj 9:SP1 0:SP2 [/]:Link TAB:TileMode",
             4, 4, 12, WHITE);
    char selBuf[64];
    snprintf(selBuf, sizeof(selBuf), "[ %s  link:%d ]", objTypeName(selectedObj), linkId);
    DrawText(selBuf, W - MeasureText(selBuf, 13) - 4, 4, 13, objColor(selectedObj));
  }

  // Level name
  std::string name = levelPath;
  auto slash = name.find_last_of("/\\");
  if (slash != std::string::npos) name = name.substr(slash + 1);
  DrawText(name.c_str(), W/2 - MeasureText(name.c_str(),13)/2, 4, 13, YELLOW);

  if (savedTimer > 0) DrawText("Saved!", W - 65, 4, 13, GREEN);
}
