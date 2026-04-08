In the Colorless game project at /Users/liamwest/src/github.com/Hyper360/colorless, improve the visual polish of the game. Here's what to do:

**1. Entity (src/entity.cpp, include/entity.hpp):**
- Add a motion trail: store the last 5 positions in a ring buffer, draw faded afterimages in `draw()`. Add a `Vector2 trailPos[5]` and `int trailIdx` member. Update trail in `update()`, draw faded smaller rectangles in `draw()`.
- Add a squash/stretch effect: slightly widen the body rect when landing (grounded transitions from false→true) and elongate when jumping (velocity.y < -100). This is VISUAL ONLY in draw(), don't change the physics body.
- Add velocity getter: `Vector2 getVelocity() const { return velocity; }`

**2. Tile drawing (src/game.cpp, the `drawTile` function):**
- Add a bevel/3D effect to solid tiles: draw a 2px lighter stripe on the top edge and left edge, and a 2px darker stripe on the bottom and right edge. Use ColorBrightness() or manually lighten/darken the baseColor.
- Add a subtle inner gradient to fire tiles (slightly lighter at top) and water tiles (slightly lighter at top) by drawing 2-3 horizontal bands with decreasing alpha overlays.

**3. Background (src/game.cpp, in `runLevel` where BeginTextureMode draws):**
- Before drawing tiles, draw a subtle dot grid pattern on the background (every 64px, small 2px dots in a very light gray like {200,200,200,80}). This gives the empty space some texture.

**4. Menu screen (src/game.cpp, `Game::menu()`):**
- Add animated floating particles in the background: use `GetTime()` to animate ~15 small circles that drift upward slowly using sin/cos patterns. Colors should cycle between dim red and dim blue (the two player colors).
- Add a subtle pulsing glow on the title text by drawing it twice - once offset/larger in a dim color that pulses with sin(GetTime()).

**5. Win screen (src/game.cpp, in `runLevel` where winTimer is drawn):**
- Add expanding rings/circles emanating from center when winning, using winTimer to animate them outward.
- Make the "YOU WIN!" text scale up from small using the winTimer value.

**6. Object drawing (src/game.cpp, `Game::drawObjects()`):**
- Electric bars: when active, add a random crackle effect - draw 3-4 small jagged line segments at random positions within the bar rect using GetRandomValue.
- Moving platforms: add a subtle bobbing shadow beneath them (a dark semi-transparent ellipse drawn 4px below).
- Gates: when transitioning (opening), add a shimmer effect.

IMPORTANT: 
- Don't break any existing accessibility features (high contrast, pattern tiles, shape labels). These overlay ON TOP of the visual enhancements.
- Don't change any physics or gameplay logic.
- The game uses raylib. Include `<cmath>` for sin/cos.
- Build with: `g++ -std=c++20 -Wall -Wextra -O3 -Iinclude/ -c -o src/game.o src/game.cpp && g++ -std=c++20 -Wall -Wextra -O3 -Iinclude/ -c -o src/entity.o src/entity.cpp && g++ -o Colorless src/entity.o src/game.o src/leveleditor.o src/levelselector.o src/main.o src/pausemenu.o -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo`
- Test that it compiles without errors.

Parent Pi session: /Users/liamwest/.pi/agent/sessions/--Users-liamwest-src-github.com-Hyper360-colorless--/2026-04-01T01-24-17-924Z_6c1a4774-8222-4abc-a92f-c229af4446bb.jsonl
