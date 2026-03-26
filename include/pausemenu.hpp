#pragma once
#include "raylib/raylib.h"

class PauseMenu {
public:
  struct Item {
    const char *label;
    bool       *toggle; // nullptr for section headers and action items
  };

  void update();
  void draw();

  bool wantsResume() const { return resumeRequested; }
  bool wantsQuit()   const { return quitRequested; }

private:
  bool resumeRequested = false;
  bool quitRequested   = false;
  int  cursor          = 0;

  // Built once, stays constant — items reference Settings:: booleans directly
  static Item items[];
  static int  itemCount();
};
