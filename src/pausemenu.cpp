#include "../include/pausemenu.hpp"
#include "../include/raylib/raylib.h"
#include "../include/settings.hpp"
#include <cstring>

// ---------------------------------------------------------------------------
// Menu layout
// nullptr toggle = action row (Resume / Quit to Menu)
// ---------------------------------------------------------------------------

// clang-format off
PauseMenu::Item PauseMenu::items[] = {
  // --- section headers (toggle == nullptr, label starts with "##") ---
  {"## IMPAIRMENTS",      nullptr},
  {"  Deuteranopia",      &Settings::deuteranopia},
  {"  Protanopia",        &Settings::protanopia},
  {"  Tritanopia",        &Settings::tritanopia},
  {"  Achromatopsia",     &Settings::achromatopsia},
  {"  Low Acuity",        &Settings::lowAcuity},
  {"  Tunnel Vision",     &Settings::tunnelVision},

  {"## ACCESSIBILITY",    nullptr},
  {"  High Contrast",     &Settings::highContrast},
  {"  Pattern on Tiles",  &Settings::patternTiles},
  {"  Shape Labels",      &Settings::shapeLabels},

  {"## ",                 nullptr}, // spacer
  {"  Resume",            nullptr},
  {"  Quit to Menu",      nullptr},
};
// clang-format on

int PauseMenu::itemCount() {
  return (int)(sizeof(items) / sizeof(items[0]));
}

// Returns true if this row can be interacted with (not a section header/spacer)
static bool isSelectable(const PauseMenu::Item &item) {
  return !(item.label[0] == '#');
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void PauseMenu::update() {
  resumeRequested = false;
  quitRequested   = false;

  const int N = itemCount();

  if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
    do { cursor = (cursor + 1) % N; } while (!isSelectable(items[cursor]));
  }
  if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
    do { cursor = (cursor - 1 + N) % N; } while (!isSelectable(items[cursor]));
  }

  if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
    Item &item = items[cursor];
    if (strcmp(item.label, "  Resume") == 0) {
      resumeRequested = true;
    } else if (strcmp(item.label, "  Quit to Menu") == 0) {
      quitRequested = true;
    } else if (item.toggle) {
      // Colorblind modes are mutually exclusive
      bool *cb[] = {&Settings::deuteranopia, &Settings::protanopia,
                    &Settings::tritanopia,   &Settings::achromatopsia};
      bool isColorblind = false;
      for (auto *p : cb)
        if (p == item.toggle) { isColorblind = true; break; }

      if (isColorblind)
        Settings::setColorblindMode(*item.toggle ? nullptr : item.toggle);
      else
        *item.toggle = !*item.toggle;
    }
  }

}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

void PauseMenu::draw() {
  const int W  = GetScreenWidth();
  const int H  = GetScreenHeight();

  // Dimmed overlay
  DrawRectangle(0, 0, W, H, {0, 0, 0, 160});

  // Panel
  const int PW = 320, PH = 460;
  const int PX = W / 2 - PW / 2;
  const int PY = H / 2 - PH / 2;
  DrawRectangle(PX, PY, PW, PH, {20, 20, 20, 240});
  DrawRectangleLines(PX, PY, PW, PH, GRAY);

  // Title
  const char *title = "PAUSED";
  DrawText(title, W / 2 - MeasureText(title, 24) / 2, PY + 12, 24, WHITE);

  // Items
  const int N       = itemCount();
  const int startY  = PY + 48;
  const int lineH   = 26;

  for (int i = 0; i < N; i++) {
    Item &item = items[i];
    int   y    = startY + i * lineH;

    // Section header
    if (item.label[0] == '#') {
      const char *hdr = item.label + 2; // skip "##"
      if (*hdr == ' ') hdr++;           // skip leading space
      if (*hdr)
        DrawText(hdr, PX + 12, y + 4, 13, DARKGRAY);
      continue;
    }

    bool selected = (i == cursor);
    Color fg      = selected ? YELLOW : LIGHTGRAY;

    // Highlight bar
    if (selected)
      DrawRectangle(PX + 4, y, PW - 8, lineH - 2, {60, 60, 60, 255});

    // Checkbox for toggleable items
    if (item.toggle) {
      bool on = *item.toggle;
      DrawRectangleLines(PX + 14, y + 6, 14, 14, fg);
      if (on)
        DrawRectangle(PX + 17, y + 9, 8, 8, fg);
    }

    DrawText(item.label + 2, PX + 36, y + 5, 14, fg); // +2 skips leading spaces
  }

  // Footer hint
  const char *hint = "WASD/Arrows: navigate   Enter/Space: toggle   ESC: resume";
  DrawText(hint, W / 2 - MeasureText(hint, 11) / 2, PY + PH + 6, 11, DARKGRAY);
}
