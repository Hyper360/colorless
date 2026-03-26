#pragma once
#include "raylib/raylib.h"

enum class TileType : int { SOLID = 0, FIRE = 1, WATER = 2, EXIT = 3 };

inline Color tileColor(TileType t) {
  switch (t) {
  case TileType::SOLID: return BLACK;
  case TileType::FIRE:  return ORANGE;
  case TileType::WATER: return SKYBLUE;
  case TileType::EXIT:  return GREEN;
  default:              return BLACK;
  }
}

inline const char *tileName(TileType t) {
  switch (t) {
  case TileType::SOLID: return "Solid";
  case TileType::FIRE:  return "Fire";
  case TileType::WATER: return "Water";
  case TileType::EXIT:  return "Exit";
  default:              return "?";
  }
}
