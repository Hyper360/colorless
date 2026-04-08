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
  wasGrounded = false;
  landSquashTimer = 0.0f;
  for (int i = 0; i < 5; i++) trailPos[i] = spawnPos;
  trailIdx = 0;
}

void Entity::draw() {
  // Draw motion trail (faded afterimages)
  for (int i = 0; i < 5; i++) {
    int idx = (trailIdx + i) % 5; // oldest first
    float alpha = (i + 1) * 0.08f; // 0.08 .. 0.40
    float scale = 0.5f + (i + 1) * 0.08f; // shrinking
    float w = body.width * scale;
    float h = body.height * scale;
    float tx = trailPos[idx].x + (body.width - w) * 0.5f;
    float ty = trailPos[idx].y + (body.height - h) * 0.5f;
    DrawRectangle((int)tx, (int)ty, (int)w, (int)h,
                  {color.r, color.g, color.b, (unsigned char)(alpha * 255)});
  }

  // Squash/stretch visual adjustment
  Rectangle drawRect = body;
  if (landSquashTimer > 0.0f) {
    // Landing squash: wider & shorter
    float t = landSquashTimer / 0.12f; // 0..1
    float sx = 1.0f + 0.2f * t;
    float sy = 1.0f - 0.15f * t;
    float cxOld = drawRect.x + drawRect.width * 0.5f;
    float cyOld = drawRect.y + drawRect.height;
    drawRect.width  *= sx;
    drawRect.height *= sy;
    drawRect.x = cxOld - drawRect.width * 0.5f;
    drawRect.y = cyOld - drawRect.height;
  } else if (velocity.y < -100.0f) {
    // Jumping stretch: taller & narrower
    float t = std::min(1.0f, std::abs(velocity.y) / 300.0f);
    float sx = 1.0f - 0.1f * t;
    float sy = 1.0f + 0.2f * t;
    float cxOld = drawRect.x + drawRect.width * 0.5f;
    float cyOld = drawRect.y + drawRect.height;
    drawRect.width  *= sx;
    drawRect.height *= sy;
    drawRect.x = cxOld - drawRect.width * 0.5f;
    drawRect.y = cyOld - drawRect.height;
  }

  DrawRectangleRec(drawRect, color);
}

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

  // Update motion trail
  trailPos[trailIdx] = {body.x, body.y};
  trailIdx = (trailIdx + 1) % 5;

  // Track landing for squash effect
  wasGrounded = grounded;

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

  // Trigger squash on landing transition
  if (grounded && !wasGrounded)
    landSquashTimer = 0.12f;
  if (landSquashTimer > 0.0f)
    landSquashTimer -= GetFrameTime();
}
