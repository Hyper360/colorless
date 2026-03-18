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

void Entity::update() {
  acceleration = Vector2Zero();

  // Horizontal: full acceleration on ground, reduced in air
  float accelMult = grounded ? 1.0f : Config::AIR_CONTROL;
  if (IsKeyDown(KEY_A))
    acceleration.x = DIR::LEFT * Config::ACCELERATION * accelMult;
  if (IsKeyDown(KEY_D))
    acceleration.x = DIR::RIGHT * Config::ACCELERATION * accelMult;

  // Jump only when grounded
  if (IsKeyPressed(KEY_W) && grounded)
    velocity.y = Config::JUMPIMPULSE;

  // Friction only on the ground — in air momentum carries freely
  if (grounded)
    acceleration.x += -Config::FRICTION * getDir(velocity.x);

  acceleration.y += Config::GRAVITY;

  // Integrate velocity
  velocity.x += acceleration.x * GetFrameTime();
  velocity.x = Clamp(velocity.x, -Config::MAXSPEED, Config::MAXSPEED);
  velocity.y += acceleration.y * GetFrameTime();

  body.x += velocity.x * GetFrameTime();
  body.y += velocity.y * GetFrameTime();

  // Reset grounded — set again below if we land this frame
  grounded = false;

  if (body.y >= GetScreenHeight() - body.height) {
    body.y = GetScreenHeight() - body.height;
    velocity.y = 0;
    grounded = true;
  }
};
