#ifndef LAPS2_UTIL_H_
#define LAPS2_UTIL_H_

#include <functional>
#include <stdexcept>

namespace util {

template<class T>
struct Singleton {
  static T& Get() {
    static T instance;
    return instance;
  }
};

template<class T, int N>
constexpr int Length(const T(&)[N]) {
  return N;
}

void UnwindNested(const std::exception& ex, const std::function<void(const std::exception&)>& handler);
void ThrowSystemError(const std::string& what);

std::string ReadFile(const std::string& path);

}  // namespace util

#endif  // LAPS2_UTIL_H_
