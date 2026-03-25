#include "../include/entity.hpp"
#include <cmath>
#include <stdio.h>

Entity::Entity(Vector2 pos) {
  body.x = pos.x;
  body.y = pos.y;
  body.width = Config::TILESIZE;
  body.height = Config::TILESIZE;
}

void Entity::changePos(Vector2 pos) {
  body.x = pos.x;
  body.y = pos.y;
}

void Entity::moveX(int direction) {
  acceleration.x = direction * Config::ACCELERATION;
}

void Entity::moveY(int direction) {
  acceleration.y = direction * Config::ACCELERATION;
}

void Entity::draw() { DrawRectangleRec(body, RED); }

int getDir(float val) { return (val != 0) ? val / std::abs(val) : 0; }

void Entity::update(const std::vector<Rectangle> &level) {
  acceleration = Vector2Zero();

  // Horizontal: full acceleration on ground, reduced in air
  float accelMult = grounded ? 1.0f : Config::AIR_CONTROL;
  if (IsKeyDown(KEY_A))
    acceleration.x = DIR::LEFT * Config::ACCELERATION * accelMult;
  if (IsKeyDown(KEY_D))
    acceleration.x = DIR::RIGHT * Config::ACCELERATION * accelMult;

  // Friction only on the ground — in air momentum carries freely
  if (grounded)
    acceleration.x += -Config::FRICTION * getDir(velocity.x);

  // Coyote time: stay jumpable briefly after walking off a ledge
  if (grounded)
    coyoteTimer = Config::COYOTE_TIME;
  else
    coyoteTimer -= GetFrameTime();

  // Jump buffer: remember a jump press for a short window before landing
  if (IsKeyPressed(KEY_W))
    jumpBufferTimer = Config::JUMP_BUFFER_TIME;
  else
    jumpBufferTimer -= GetFrameTime();

  // Fire jump if buffer active and coyote window open
  if (jumpBufferTimer > 0.0f && coyoteTimer > 0.0f) {
    velocity.y = Config::JUMPIMPULSE;
    jumpBufferTimer = 0.0f;
    coyoteTimer = 0.0f;
  }

  // Variable jump height: cut upward velocity when jump is released early
  if (IsKeyReleased(KEY_W) && velocity.y < 0.0f)
    velocity.y *= Config::JUMP_RELEASE_MULT;

  // Higher gravity when falling so the arc feels intentional, not floaty
  float gravMult = (velocity.y > 0.0f) ? Config::FALL_GRAVITY_MULT : 1.0f;
  acceleration.y += Config::GRAVITY * gravMult;

  // Integrate velocity
  velocity.x += acceleration.x * GetFrameTime();
  velocity.x = Clamp(velocity.x, -Config::MAXSPEED, Config::MAXSPEED);
  velocity.y += acceleration.y * GetFrameTime();

  // Reset grounded — set again below if we land this frame
  grounded = false;

  // Resolve X then Y separately so each axis can be corrected independently
  body.x += velocity.x * GetFrameTime();
  for (auto &r : level) {
    if (CheckCollisionRecs(body, r)) {
      if (velocity.x > 0)
        body.x = r.x - body.width;
      else if (velocity.x < 0)
        body.x = r.x + r.width;
      velocity.x = 0;
    }
  }

  body.y += velocity.y * GetFrameTime();
  for (auto &r : level) {
    if (CheckCollisionRecs(body, r)) {
      if (velocity.y > 0) {
        body.y = r.y - body.height;
        grounded = true;
      } else if (velocity.y < 0) {
        body.y = r.y + r.height;
      }
      velocity.y = 0;
    }
  }

  // Screen floor fallback
  if (body.y >= GetScreenHeight() - body.height) {
    body.y = GetScreenHeight() - body.height;
    velocity.y = 0;
    grounded = true;
  }
};
