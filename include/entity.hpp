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

struct Controls {
  int left;
  int right;
  int jump;
};

enum class ElementType { FIRE, WATER };

class Entity {
  Rectangle body;
  Vector2 velocity = Vector2Zero();
  bool grounded = false;
  float coyoteTimer = 0.0f;
  float jumpBufferTimer = 0.0f;
  Controls controls;
  Color color;
  ElementType element;
  Vector2 spawnPos;

public:
  Entity(Vector2 pos, Controls ctrl, Color col, ElementType elem);
  void respawn();
  Rectangle getBody() const { return body; }
  ElementType getElement() const { return element; }
  Vector2 getPos() const { return {body.x, body.y}; }
  void draw();
  void update(const std::vector<Rectangle> &solids);
};
