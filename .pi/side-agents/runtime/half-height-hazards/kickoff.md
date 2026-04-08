In /Users/liamwest/src/github.com/Hyper360/colorless, make fire, water, and spike tiles render as HALF-HEIGHT tiles that sit IN the floor rather than on top of it. The player should appear submerged (half in the hazard) but NOT fall through — the hazard tile acts as a solid surface.

Changes needed in src/game.cpp:

1. In `loadLevel()` (around line 128-135), when creating tile rects for FIRE, WATER, and SPIKE types, make them half-height and offset down:
   Instead of: `tiles.push_back({type, {x, y, ts, ts}});`
   For fire/water/spike: `tiles.push_back({type, {x, y + ts*0.5f, ts, ts*0.5f}});`
   This makes the hazard rect occupy the BOTTOM half of the tile cell — sitting flush with the floor.

2. In `buildSolids()` (around line 564), add fire, water, and spike tiles as solids too — they should be walkable surfaces (the player stands on them, half-submerged). Add after the SOLID check:
   ```
   if (t.type == TileType::FIRE || t.type == TileType::WATER || t.type == TileType::SPIKE)
       s.push_back(t.rect);
   ```

3. In the death check in `runLevel()` (around lines 433-437), the collision detection already uses `t.rect` which will now be the half-height rect. The player's body overlaps the top half of the hazard when standing on it — this will trigger death. We need to adjust: only kill if the player's CENTER (not just any overlap) is inside the hazard. Change the fire/water/spike death checks to use a smaller collision rect — check if the player's bottom quarter overlaps:
   ```
   // For hazard checks, use player's bottom portion only
   Rectangle p1bot = p.getBody();
   p1bot.y += p1bot.height * 0.5f;
   p1bot.height *= 0.5f;
   Rectangle p2bot = p2.getBody();
   p2bot.y += p2bot.height * 0.5f;
   p2bot.height *= 0.5f;
   ```
   Then use p1bot/p2bot for the fire/water/spike collision checks instead of p.getBody()/p2.getBody().

   BUT: fire still only kills water player (p2), water still only kills fire player (p1), spikes kill both. Keep that logic.

4. The `drawTile()` function already draws `t.rect` so the half-height rect will be drawn correctly.

Read the current game.cpp first, then make precise edits. Build with:
g++ -std=c++20 -Wall -Wextra -O3 -Iinclude/ -c -o src/game.o src/game.cpp && g++ -o Colorless src/entity.o src/game.o src/leveleditor.o src/levelselector.o src/main.o src/pausemenu.o -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

Verify build succeeds with no errors.

Parent Pi session: /Users/liamwest/.pi/agent/sessions/--Users-liamwest-src-github.com-Hyper360-colorless--/2026-04-01T01-24-17-924Z_6c1a4774-8222-4abc-a92f-c229af4446bb.jsonl
