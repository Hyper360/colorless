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

void Entity::update() {
  acceleration = Vector2Zero();

  if (IsKeyDown(KEY_A)) {
    moveX(DIR::LEFT);
  }
  if (IsKeyDown(KEY_D)) {
    moveX(DIR::RIGHT);
  }
  if (IsKeyPressed(KEY_W)) {
    velocity.y = Config::JUMPIMPULSE;
  }

  acceleration.y += Config::GRAVITY;

  acceleration.x += (-Config::FRICTION * getDir(velocity.x));

  velocity.x += acceleration.x * GetFrameTime();
  velocity.x = Clamp(velocity.x, -Config::MAXSPEED, Config::MAXSPEED);
  velocity.y += acceleration.y * GetFrameTime();

  body.x += velocity.x * GetFrameTime();
  body.y += velocity.y * GetFrameTime();

  if (body.y >= GetScreenHeight() - 50) {
    body.y = GetScreenHeight() - 50;
    velocity.y = 0;
  }
};
