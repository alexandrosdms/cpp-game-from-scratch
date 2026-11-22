#pragma once

#include <stacktrace>
#include <stdexcept>
#include <string>

namespace game {

class Exception : public std::runtime_error {
public:
  Exception(const std::string &what);

  auto stacktrace() const -> std::string;

private:
  std::stacktrace trace_;
};

} // namespace game
