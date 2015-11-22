#include "util.h"
#include <fstream>
#include <iostream>
#include <system_error>

namespace {

void UnwindNested(const std::exception& ex, const std::function<void(const std::exception&)>& handler) {
  handler(ex);
  try {
    std::rethrow_if_nested(ex);
  } catch (const std::exception& ex) {
    UnwindNested(ex, handler);
  }
}

}  // namespace

namespace util {

std::string ReadFile(const std::string& path) {
  std::ifstream source(path);
  if (!source.is_open()) ThrowSystemError("Can not read file");
  return std::string(std::istreambuf_iterator<char>(source), {});
}

void PrintException(const std::exception& ex) {
  int level = 0;
  UnwindNested(ex, [&level](const auto& ex) {
    std::cerr << "!" << std::string(level++ * 2 + 1, ' ') << ex.what() << std::endl;
  });
}

void ThrowSystemError(const std::string& what) {
  try {
    throw std::system_error(errno, std::system_category());
  } catch (const std::exception&) {
    std::throw_with_nested(std::runtime_error(what));
  }
}

}  // namespace util
