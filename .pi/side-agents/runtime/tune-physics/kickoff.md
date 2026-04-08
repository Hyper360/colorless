In /Users/liamwest/src/github.com/Hyper360/colorless, tune the player movement physics in include/config.hpp. The user wants:

1. Jump height = exactly 2 full blocks (64px with TILESIZE=32)
2. Full-speed horizontal jump distance = about 3 blocks sideways (96px)
3. Movement should feel tighter/cleaner — less floaty horizontal when jumping

Current values in config.hpp:
- GRAVITY: 15.0f * TILESIZE (480)
- JUMPIMPULSE: -7.5f * TILESIZE (-240) 
- MAXSPEED: TILESIZE * 10.0f (320)
- AIR_CONTROL: 0.2f
- FALL_GRAVITY_MULT: 2.5f
- ACCELERATION: 50.0f * TILESIZE (1600)
- FRICTION: 47.0f * TILESIZE (1504)

Physics derivation for the new values:
- Jump height h = v²/(2g) = 64px → with g=480: v = sqrt(2*480*64) = sqrt(61440) ≈ 248 → JUMPIMPULSE = -7.75f * TILESIZE
- Time airborne: up = v/g = 248/480 = 0.517s, down = sqrt(2*64/(480*2.5)) = sqrt(0.107) = 0.327s, total ≈ 0.844s
- For 3 blocks (96px) horizontal distance during jump: effective_speed = 96/0.844 ≈ 114 px/s during air time
- The player's air speed should cap around 114. Keep MAXSPEED reasonable for ground (~6*TS = 192 px/s) and use AIR_CONTROL to limit air speed.
- With MAXSPEED=192 and AIR_CONTROL, at full ground speed entering a jump: horizontal vel ≈ 192, but air friction should bring it down. Actually AIR_CONTROL only affects acceleration, not existing velocity. So we need a lower MAXSPEED or add air speed capping.

SIMPLEST APPROACH: 
- JUMPIMPULSE = -7.75f * TILESIZE (for 2 block jump height)
- MAXSPEED = TILESIZE * 4.0f (128 px/s) — this gives 128 * 0.844 ≈ 108px ≈ 3.4 blocks horizontal. Close enough.
- ACCELERATION = 40.0f * TILESIZE (slightly snappier relative to lower max speed)
- FRICTION = 50.0f * TILESIZE (quick stop)
- AIR_CONTROL = 0.12f (tighter air control)
- Keep GRAVITY, FALL_GRAVITY_MULT, JUMP_RELEASE_MULT, COYOTE_TIME, JUMP_BUFFER_TIME the same

Edit include/config.hpp with these values. Then rebuild:
g++ -std=c++20 -Wall -Wextra -O3 -Iinclude/ -c -o src/entity.o src/entity.cpp && g++ -std=c++20 -Wall -Wextra -O3 -Iinclude/ -c -o src/game.o src/game.cpp && g++ -o Colorless src/entity.o src/game.o src/leveleditor.o src/levelselector.o src/main.o src/pausemenu.o -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

Verify build succeeds.

Parent Pi session: /Users/liamwest/.pi/agent/sessions/--Users-liamwest-src-github.com-Hyper360-colorless--/2026-04-01T01-24-17-924Z_6c1a4774-8222-4abc-a92f-c229af4446bb.jsonl
