#ifndef LAPS2_XCB_H_
#define LAPS2_XCB_H_

#include <memory>
#include <xcb/xcb.h>

namespace xcb {

template<class R, class S, class F1, class F2, class... Args>
static R Sync(F2 f2, R S::*ptr, F1 f1, xcb_connection_t* conn, Args&&... args) {
  try {
    xcb_generic_error_t* error;
    auto cookie = f1(conn, std::forward<Args>(args)...);
    auto temp = f2(conn, cookie, &error);
    using PtrType = typename std::remove_pointer<decltype(temp)>::type;
    std::unique_ptr<PtrType, decltype(&free)> result(temp, &free);
    if (error) throw std::runtime_error("Error " + std::to_string(error->error_code));
    if (!result) throw std::runtime_error("Unknown error");
    return result.get()->*ptr;
  } catch (const std::exception&) {
    std::throw_with_nested(std::runtime_error("Sync operation failed"));
  }
}

xcb_window_t FindTray(xcb_connection_t* conn, int screen);
void Embed(xcb_connection_t* conn, xcb_window_t tray, xcb_window_t win);

using Connection = std::unique_ptr<xcb_connection_t, decltype(&xcb_disconnect)>;
using Event = std::unique_ptr<xcb_generic_event_t, decltype(&free)>;

}  // namespace xcb

#endif  // LAPS2_XCB_H_
