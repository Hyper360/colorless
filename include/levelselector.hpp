#pragma once
#include "raylib/raylib.h"
#include <string>
#include <vector>

class LevelSelector {
public:
  LevelSelector();
  void update();
  void draw();
  void refresh();

  bool wantsPlay() const { return playRequested; }
  bool wantsEdit() const { return editRequested; }
  bool wantsNew() const { return newRequested; }
  bool wantsBack() const { return backRequested; }
  std::string getSelected() const;

private:
  std::vector<std::string> levels;
  int cursor = 0;
  bool playRequested = false;
  bool editRequested = false;
  bool newRequested = false;
  bool backRequested = false;

  void resetFlags();
};
