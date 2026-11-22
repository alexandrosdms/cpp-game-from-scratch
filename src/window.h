#pragma once

#include "auto_release.h"
#include <cstdint>

#include <Windows.h>

namespace game {

class Window {
public:
  Window(std::uint32_t width, std::uint32_t height);
  ~Window() = default;

  Window(const Window &) = delete;
  auto operator=(const Window &) -> Window & = delete;

  Window(Window &&) = default;
  auto operator=(Window &&) -> Window & = default;

private:
  AutoRelease<::HWND, nullptr> window_;
  WNDCLASSA wc_;
};

} // namespace game
