#include "../include/config.hpp"
#include "../include/game.hpp"
#include "../include/raylib/raylib.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>

void webWrapper(void *game_p) {
  Game *game = static_cast<Game *>(game_p);
  game->stateManager();
}
#endif

int main() {
  InitWindow(Config::WIDTH, Config::HEIGHT, Config::TITLE);

  Game game = Game();

#if defined(PLATFORM_WEB)

  emscripten_set_main_loop_arg(webWrapper, &game, 0, 1);

#else

  SetTargetFPS(Config::FPS);
  while (!WindowShouldClose()) {
    game.stateManager();
  }

#endif

  game.exit();
  CloseWindow();
}
