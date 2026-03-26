#pragma once
#include "raylib/raylib.h"

enum class ObjType : int {
    PRESSURE_PLATE   = 0,
    LEVER            = 1,
    GATE             = 2,
    MOVING_PLATFORM  = 3,
    CONVEYOR_LEFT    = 4,
    CONVEYOR_RIGHT   = 5,
    FALLING_PLATFORM = 6,
    PUSHABLE_BLOCK   = 7,
    SPAWN_P1         = 8,  // editor marker only — consumed by loadLevel, not added to objects
    SPAWN_P2         = 9,
    CRUSHER          = 10, // deadly moving block; speed=px/s, origin→endpoint patrol
    ELECTRIC         = 11, // timed deadly bar; speed=period(s), active=electrified
};
constexpr int OBJ_TYPE_COUNT = 12;

struct LevelObject {
    ObjType   type;
    Rectangle rect;       // current world-space bounding box

    // --- Linking (plates, levers, gates) ---
    int  linkId = 0;
    bool active = false;  // plate: pressed; lever: on; gate: open

    // --- Moving platform patrol ---
    Vector2 origin   = {};
    Vector2 endpoint = {};
    float   speed    = 80.0f;
    float   tParam   = 0.0f;
    int     patDir   = 1; // +1/-1

    // --- Falling platform ---
    float fallTimer = -1.0f; // <0 = ready; >0 = countdown; 0 -> falling = true
    bool  falling   = false;

    // --- Pushable block / falling platform physics ---
    float velY     = 0.0f;
    bool  grounded = false;
};

inline const char *objTypeName(ObjType t) {
    switch (t) {
    case ObjType::PRESSURE_PLATE:   return "plate";
    case ObjType::LEVER:            return "lever";
    case ObjType::GATE:             return "gate";
    case ObjType::MOVING_PLATFORM:  return "mplatform";
    case ObjType::CONVEYOR_LEFT:    return "conv<";
    case ObjType::CONVEYOR_RIGHT:   return "conv>";
    case ObjType::FALLING_PLATFORM: return "fplatform";
    case ObjType::PUSHABLE_BLOCK:   return "block";
    case ObjType::SPAWN_P1:        return "spawn-P1";
    case ObjType::SPAWN_P2:        return "spawn-P2";
    case ObjType::CRUSHER:         return "crusher";
    case ObjType::ELECTRIC:        return "electric";
    default:                        return "?";
    }
}

inline Color objColor(ObjType t, bool active = false) {
    switch (t) {
    case ObjType::PRESSURE_PLATE:
        return active ? Color{255,255,60,255} : Color{160,130,20,255};
    case ObjType::LEVER:
        return active ? Color{255,180,50,255} : Color{140,90,30,255};
    case ObjType::GATE:
        return active ? Color{100,160,255,60} : Color{70,110,220,255};
    case ObjType::MOVING_PLATFORM:  return {60,  190, 60,  255};
    case ObjType::CONVEYOR_LEFT:
    case ObjType::CONVEYOR_RIGHT:   return {170, 70,  200, 255};
    case ObjType::FALLING_PLATFORM: return {210, 140, 70,  255};
    case ObjType::PUSHABLE_BLOCK:   return {140, 140, 150, 255};
    case ObjType::SPAWN_P1:        return {255, 80,  80,  200};
    case ObjType::SPAWN_P2:        return {80,  140, 255, 200};
    case ObjType::CRUSHER:         return {190, 40,  40,  255};
    case ObjType::ELECTRIC:        return active ? Color{255,240,40,255} : Color{90,90,30,255};
    default:                        return PINK;
    }
}
