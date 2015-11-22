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

struct NonCopyable {
  NonCopyable() = default;
  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator =(const NonCopyable&) = delete;
};

template<class T>
class Holder : public NonCopyable {
 protected:
  using Base = Holder<T>;
  using DtorPtr = void(*)(T*);
  T* impl_;

 private:
  DtorPtr dtor_;

 public:
  Holder(T* op, DtorPtr dtor) :
    impl_(op), dtor_(dtor) {}

  Holder(Holder&& op) {
    impl_ = op.impl_;
    op.impl_ = nullptr;
  }

  ~Holder() {
    if (impl_ && dtor_)
      dtor_(impl_);
  }
};

template<class T, int N>
constexpr int Length(const T(&)[N]) {
  return N;
}

std::string ReadFile(const std::string& path);
void PrintException(const std::exception& ex);
void ThrowSystemError(const std::string& what);

}  // namespace util

#endif  // LAPS2_UTIL_H_
