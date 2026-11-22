#include <print>

#include "window.h"

auto main() -> int {
  std::println("Hello World");

  game::Window window{800u, 600u};

  while (window.running()) {
  }

  return 0;
}
