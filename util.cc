#include "util.h"
#include <fstream>
#include <system_error>

namespace util {

void UnwindNested(const std::exception& ex, const std::function<void(const std::exception&)>& handler) {
  handler(ex);
  try {
    std::rethrow_if_nested(ex);
  } catch (const std::exception& ex) {
    UnwindNested(ex, handler);
  }
}

void ThrowSystemError(const std::string& what) {
  try {
    throw std::system_error(errno, std::system_category());
  } catch (const std::exception&) {
    std::throw_with_nested(std::runtime_error(what));
  }
}

std::string ReadFile(const std::string& path) {
  std::ifstream source(path);
  return std::string(std::istreambuf_iterator<char>(source), {});
}

}  // namespace util
