#pragma once
#include "config.hpp"
#include "raylib/raylib.h"
#include "raylib/raymath.h"
#include <vector>

enum DIR {
  UP = 1,
  RIGHT = 1,
  DOWN = -1,
  LEFT = -1,
};

class Entity {
  Rectangle body;
  Vector2 acceleration = {0, Config::ACCELERATION};
  Vector2 velocity = Vector2Zero();

public:
  Entity(Vector2 pos);
  void changePos(Vector2);
  void moveX(int right);
  void moveY(int up);
  Vector2 getPos() { return {body.x, body.y}; }
  void draw();
  void update(const std::vector<Rectangle> &level);
};
