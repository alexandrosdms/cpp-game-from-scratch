#include "exception.h"

#include <stacktrace>
#include <string>

namespace game {

Exception::Exception(const std::string &what)
    : std::runtime_error(what), trace_(std::stacktrace::current()) {}

auto Exception::stacktrace() const -> std::string {
  return std::to_string(trace_);
}

} // namespace game
