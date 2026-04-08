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
  int interact;
};

enum class ElementType { FIRE, WATER };

class Entity {
  Rectangle body;
  Vector2 velocity = Vector2Zero();
  bool grounded = false;
  float coyoteTimer = 0.0f;
  float jumpBufferTimer = 0.0f;
  float conveyorVX = 0.0f; // applied after friction, before clamp (set each frame by Game)
  Controls controls;
  Color color;
  ElementType element;
  Vector2 spawnPos;

public:
  Entity(Vector2 pos, Controls ctrl, Color col, ElementType elem);
  void respawn();
  Rectangle getBody()     const { return body; }
  ElementType getElement() const { return element; }
  Vector2 getPos()        const { return {body.x, body.y}; }
  bool isGrounded()       const { return grounded; }

  // Called by Game for interactive objects
  void nudge(Vector2 delta);          // direct position shift (moving platforms)
  void applyConveyorX(float vx);      // sets conveyorVX for this frame
  void stopX();                       // zero velocity.x (pushable block wall)
  void stopY();                       // zero velocity.y + set grounded (standing on block)
  bool isInteracting()  const;        // interact key pressed this frame
  bool isPushingRight() const;        // right key held
  bool isPushingLeft()  const;        // left key held

  void draw();
  void update(const std::vector<Rectangle> &solids);
};
