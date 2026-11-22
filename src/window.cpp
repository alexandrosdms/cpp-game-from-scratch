#include "window.h"
#include <cstdint>
#include <stdexcept>

#include <Windows.h>

namespace game {
Window::Window(std::uint32_t width, std::uint32_t heihgt)
    : window_({}), wc_({}) {

  wc_ = {
      .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
      .lpfnWndProc = ::DefWindowProcA,
      .hInstance = ::GetModuleHandleA(nullptr),
      .lpszClassName = "window class",
  };

  if (::RegisterClassA(&wc_) == 0) {
    throw std::runtime_error("failed to register class");
  }

  ::RECT rect{.left = {},
              .top = {},
              .right = static_cast<int>(width),
              .bottom = static_cast<int>(heihgt)};

  if (::AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false) == 0) {
    throw std::runtime_error("could not resize window");
  }
  window_ = {::CreateWindowExA(0, wc_.lpszClassName, "game window",
                               WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                               CW_USEDEFAULT, rect.right - rect.left,
                               rect.bottom - rect.top, nullptr, nullptr,
                               wc_.hInstance, nullptr),
             ::DestroyWindow};

  ::ShowWindow(window_, SW_SHOW);
  ::UpdateWindow(window_);
}

} // namespace game
