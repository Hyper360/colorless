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
inline constexpr float FRICTION{6.0f * TILESIZE};
inline constexpr float MAXSPEED{TILESIZE * 10.0f};
inline constexpr float AIR_CONTROL{0.3f};    // fraction of ground accel available in air
inline constexpr float FALL_GRAVITY_MULT{2.2f}; // extra gravity when falling
inline constexpr float JUMP_RELEASE_MULT{0.45f}; // velocity cut when jump released early
inline constexpr float COYOTE_TIME{0.1f};        // seconds you can still jump after leaving ground
inline constexpr float JUMP_BUFFER_TIME{0.12f};  // seconds a pre-press jump is remembered
} // namespace Config
