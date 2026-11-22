#include <iostream>
#include <print>
#include <stdexcept>

#include "exception.h"
#include "window.h"

auto bar() -> void { throw game::Exception("err"); }

auto foo() -> void { bar(); }

auto main() -> int {
  std::println("Hello World");

  try {
    foo();
  } catch (game::Exception &err) {
    std::println(std::cerr, "exception {} {}", err.what(), err.stacktrace());
  }
  // game::Window window{800u, 600u};

  // while (window.running()) {
  // }

  return 0;
}
