#pragma once
#include "raylib/raylib.h"

enum class TileType : int { SOLID = 0, FIRE = 1, WATER = 2, EXIT_P1 = 3, EXIT_P2 = 4 };

inline Color tileColor(TileType t) {
  switch (t) {
  case TileType::SOLID:   return BLACK;
  case TileType::FIRE:    return ORANGE;
  case TileType::WATER:   return SKYBLUE;
  case TileType::EXIT_P1: return {220, 80,  80,  255}; // red-tinted: fire player's door
  case TileType::EXIT_P2: return {80,  160, 220, 255}; // blue-tinted: water player's door
  default:                return BLACK;
  }
}

inline const char *tileName(TileType t) {
  switch (t) {
  case TileType::SOLID:   return "Solid";
  case TileType::FIRE:    return "Fire";
  case TileType::WATER:   return "Water";
  case TileType::EXIT_P1: return "Exit-P1";
  case TileType::EXIT_P2: return "Exit-P2";
  default:                return "?";
  }
}
