#include "../include/entity.hpp"
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

  if (IsKeyDown(KEY_A))
    moveX(DIR::LEFT);
  if (IsKeyDown(KEY_D))
    moveX(DIR::RIGHT);
  if (IsKeyPressed(KEY_W))
    velocity.y = Config::JUMPIMPULSE;

  acceleration.y += Config::GRAVITY;
  acceleration.x += (-Config::FRICTION * getDir(velocity.x));

  velocity.x += acceleration.x * GetFrameTime();
  velocity.x = Clamp(velocity.x, -Config::MAXSPEED, Config::MAXSPEED);
  velocity.y += acceleration.y * GetFrameTime();

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
      if (velocity.y > 0)
        body.y = r.y - body.height;
      else if (velocity.y < 0)
        body.y = r.y + r.height;
      velocity.y = 0;
    }
  }

  // Screen floor fallback
  if (body.y >= GetScreenHeight() - body.height) {
    body.y = GetScreenHeight() - body.height;
    velocity.y = 0;
  }
};
