#pragma once
namespace Config {
inline constexpr const char *TITLE{"Colorless"};
inline constexpr int WIDTH{1280};
inline constexpr int HEIGHT{800};
inline constexpr int FPS{60};
inline constexpr float TILESIZE{32};
inline constexpr float GRAVITY{15.0f * TILESIZE};
inline constexpr float ACCELERATION{40.0f * TILESIZE};
inline constexpr float JUMPIMPULSE{-14.0f * TILESIZE};
inline constexpr float FRICTION{
    50.0f * TILESIZE}; // velocity/sec removed when no input (moveToward style)
inline constexpr float MAXSPEED{TILESIZE * 8.0f};
inline constexpr float AIR_CONTROL{
    0.12f}; // fraction of ground accel available in air
inline constexpr float FALL_GRAVITY_MULT{2.5f}; // extra gravity when falling
inline constexpr float JUMP_RELEASE_MULT{
    0.45f}; // velocity cut when jump released early
inline constexpr float COYOTE_TIME{
    0.1f}; // seconds you can still jump after leaving ground
inline constexpr float JUMP_BUFFER_TIME{
    0.12f}; // seconds a pre-press jump is remembered
} // namespace Config
