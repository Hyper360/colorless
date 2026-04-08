In /Users/liamwest/src/github.com/Hyper360/colorless, rescale levels 1-8 from the old 25x19 tile grid (800x640) to the new 40x25 tile grid (1280x800). 

The approach: write a Python script that reads each level JSON, scales all tile x/y coordinates and object x/y coordinates to fill the new grid, and writes back. The scaling should:

1. Map the old grid (0-24 x, 0-18 y) to new grid (0-39 x, 0-24 y)
2. Keep the basic structure — walls should still be walls, floors still floors
3. The floor was at y=18, now should be at y=24
4. Left wall was x=0, stays x=0. Right wall was x=24, now x=39
5. All interior features should be proportionally repositioned

The simplest correct approach: offset-and-scale.
- x_new = round(x_old * 39/24) for boundary tiles, or shift interior content to use more space
- y_new = round(y_old * 24/18) for boundary tiles

Actually, the best approach is:
- Walls: left wall stays x=0, right wall moves to x=39 (was x=24), top stays y=0, floor moves to y=24 (was y=18)
- Interior content: scale proportionally: x_new = round(x_old * 39.0/24.0), y_new = round(y_old * 24.0/18.0)
- For objects with ex/ey endpoints (moving platforms, crushers), scale those too
- Keep all tile types and object types the same
- Keep colorblind flags

Write the script as /tmp/rescale_levels.py and run it. After running, verify each level by printing a grid visualization.

IMPORTANT: For each level, also fill in the walls properly:
- Full bottom row at y=24 (solid)  
- Full left column x=0 (solid)
- Full right column x=39 (solid)
- Don't duplicate tiles at same position

After rescaling, verify by printing ASCII maps of all 8 levels.

The level JSON format is: {"tiles": [{"x":N, "y":N, "type":N}, ...], "objects": [{"type":N, "x":N, "y":N, ...}, ...], "colorblind": N}

Build test after: cd /Users/liamwest/src/github.com/Hyper360/colorless && g++ -std=c++20 -Wall -Wextra -O3 -Iinclude/ -c -o src/game.o src/game.cpp 2>&1 | grep error; g++ -o Colorless src/entity.o src/game.o src/leveleditor.o src/levelselector.o src/main.o src/pausemenu.o -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo 2>&1

Parent Pi session: /Users/liamwest/.pi/agent/sessions/--Users-liamwest-src-github.com-Hyper360-colorless--/2026-04-01T01-24-17-924Z_6c1a4774-8222-4abc-a92f-c229af4446bb.jsonl
