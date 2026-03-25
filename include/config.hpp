#pragma once
namespace Config {
inline constexpr const char *TITLE{"Colorless"};
inline constexpr int WIDTH{800};
inline constexpr int HEIGHT{640};
inline constexpr int FPS{60};
inline constexpr float TILESIZE{32};
inline constexpr float GRAVITY{9.8f * TILESIZE};
inline constexpr float ACCELERATION{50.0f * TILESIZE};
inline constexpr float JUMPIMPULSE{-10.0f * TILESIZE};
inline constexpr float FRICTION{47.0f * TILESIZE}; // velocity/sec removed when no input (moveToward style)
inline constexpr float MAXSPEED{TILESIZE * 10.0f};
inline constexpr float AIR_CONTROL{
    0.3f}; // fraction of ground accel available in air
} // namespace Config
