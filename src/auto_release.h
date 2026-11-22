#pragma once

#include <concepts>
#include <functional>
#include <ranges>
#include <utility>

namespace game {

template <class T, T Invalid = {}> class AutoRelease {
public:
  AutoRelease() : AutoRelease({}, nullptr) {}

  AutoRelease(T obj, std::function<void(T)> deleter)
      : obj_(obj), deleter_(deleter) {}

  ~AutoRelease() {
    if ((obj_ != Invalid) && deleter_) {
      deleter_(obj_);
    }
  }

  AutoRelease(const AutoRelease &) = delete;
  auto operator=(const AutoRelease &) -> AutoRelease & = delete;

  AutoRelease(AutoRelease &&other) : AutoRelease() { swap(other); }

  auto operator=(AutoRelease &&other) -> AutoRelease & {
    AutoRelease newObj{std::move(other)};
    swap(newObj);

    return *this;
  }

  auto swap(AutoRelease &other) noexcept -> void {
    std::ranges::swap(obj_, other.obj_);
    std::ranges::swap(deleter_, other.deleter_);
  }

  T get() const { return obj_; }

  operator T() const { return obj_; }

private:
  T obj_;
  std::function<void(T)> deleter_;
};

} // namespace game
