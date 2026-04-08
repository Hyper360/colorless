#pragma once
#include "raylib/raylib.h"

enum class TileType : int { SOLID = 0, FIRE = 1, WATER = 2, EXIT_P1 = 3, EXIT_P2 = 4, SPIKE = 5 };

inline Color tileColor(TileType t) {
  switch (t) {
  case TileType::SOLID:   return BLACK;
  case TileType::FIRE:    return {220, 50, 30, 255};   // red (confusable with green under protanopia/deuteranopia)
  case TileType::WATER:   return {40, 180, 50, 255};    // green (confusable with red under protanopia/deuteranopia);
  case TileType::EXIT_P1: return {220, 80,  80,  255}; // red-tinted: fire player's door
  case TileType::EXIT_P2: return {80,  200, 100, 255}; // green-tinted: water player's door
  case TileType::SPIKE:   return {140, 140, 155, 255}; // metallic gray
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
  case TileType::SPIKE:   return "Spike";
  default:                return "?";
  }
}
