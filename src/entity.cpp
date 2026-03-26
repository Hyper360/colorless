#include "../include/entity.hpp"
#include <cmath>
#include <stdio.h>

Entity::Entity(Vector2 pos, Controls ctrl, Color col, ElementType elem)
    : controls(ctrl), color(col), element(elem), spawnPos(pos) {
  body.x = pos.x;
  body.y = pos.y;
  body.width = Config::TILESIZE;
  body.height = Config::TILESIZE;
}

void Entity::nudge(Vector2 d)       { body.x += d.x; body.y += d.y; }
void Entity::applyConveyorX(float v) { conveyorVX = v; }
void Entity::stopX()                 { velocity.x = 0; }
void Entity::stopY()                 { velocity.y = 0; grounded = true; }
bool Entity::isInteracting()  const  { return IsKeyPressed(controls.interact); }
bool Entity::isPushingRight() const  { return IsKeyDown(controls.right); }
bool Entity::isPushingLeft()  const  { return IsKeyDown(controls.left); }

void Entity::respawn() {
  body.x = spawnPos.x;
  body.y = spawnPos.y;
  velocity = Vector2Zero();
  grounded = false;
  coyoteTimer = 0.0f;
  jumpBufferTimer = 0.0f;
}

void Entity::draw() { DrawRectangleRec(body, color); }

int getDir(float val) { return (val != 0) ? val / std::abs(val) : 0; }

void Entity::update(const std::vector<Rectangle> &level) {
  Vector2 acceleration = Vector2Zero();

  bool inputX = IsKeyDown(controls.left) || IsKeyDown(controls.right);

  // Horizontal: full acceleration on ground, reduced in air
  float accelMult = grounded ? 1.0f : Config::AIR_CONTROL;
  if (IsKeyDown(controls.left))
    acceleration.x = DIR::LEFT * Config::ACCELERATION * accelMult;
  if (IsKeyDown(controls.right))
    acceleration.x = DIR::RIGHT * Config::ACCELERATION * accelMult;

  // Coyote time: stay jumpable briefly after walking off a ledge
  if (grounded)
    coyoteTimer = Config::COYOTE_TIME;
  else
    coyoteTimer -= GetFrameTime();

  // Jump buffer: remember a jump press for a short window before landing
  if (IsKeyPressed(controls.jump))
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
  if (IsKeyReleased(controls.jump) && velocity.y < 0.0f)
    velocity.y *= Config::JUMP_RELEASE_MULT;

  // Higher gravity when falling so the arc feels intentional, not floaty
  float gravMult = (velocity.y > 0.0f) ? Config::FALL_GRAVITY_MULT : 1.0f;
  acceleration.y += Config::GRAVITY * gravMult;

  // Integrate velocity
  velocity.x += acceleration.x * GetFrameTime();
  // Conveyor belt override: applied after friction so it isn't cancelled
  velocity.x += conveyorVX;
  conveyorVX = 0.0f;
  velocity.x = Clamp(velocity.x, -Config::MAXSPEED, Config::MAXSPEED);

  // Friction: directly move velocity toward zero (moveToward style).
  // Only when grounded and not pressing a direction — doesn't fight input,
  // never overshoots zero, stopping time = MAXSPEED / FRICTION.
  if (grounded && !inputX) {
    float decel = Config::FRICTION * GetFrameTime();
    velocity.x = (velocity.x > 0) ? std::max(0.0f, velocity.x - decel)
                                   : std::min(0.0f, velocity.x + decel);
  }
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

};
