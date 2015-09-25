#include "../widget.h"
#include "../xcb.h"
#include "../util.h"
#include <iostream>
#include <vector>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <unistd.h>

namespace {

void PrintException(const std::exception& ex) {
  int level = 0;
  util::UnwindNested(ex, [&level](const auto& ex) {
    std::cout << "!" << std::string(level++ * 2 + 1, ' ') << ex.what() << std::endl;
  });
}

class WidgetView {
 private:
  int width_;
  int height_;
  const uint8_t* state_;
  xcb_window_t window_;
  xcb_gcontext_t context_;

  xcb_window_t CreateWindow(xcb_connection_t* conn, int width, int height) {
    auto result = xcb_generate_id(conn);
    auto screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t values[] = {screen->white_pixel, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_RESIZE_REDIRECT};
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, result, screen->root, 0, 0, width, height, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, XCB_COPY_FROM_PARENT, mask, values);
    return result;
  }

  xcb_gcontext_t CreateContext(xcb_connection_t* conn) {
    auto result = xcb_generate_id(conn);
    auto screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    uint32_t values[] = {screen->black_pixel, 0};
    xcb_create_gc(conn, result, screen->root, mask, values);
    return result;
  }

 public:
  WidgetView(const WidgetView&) = delete;
  WidgetView(WidgetView&&) = default;

  WidgetView(xcb_connection_t* conn, int size) :
      width_(size), height_(size),
      state_(nullptr),
      window_(CreateWindow(conn, width_, height_)),
      context_(CreateContext(conn)) {
    xcb_map_window(conn, window_);
  }

  void Update(xcb_connection_t* conn) {
    if (width_ < 0 || height_ < 0 || !state_) return;
    xcb_clear_area(conn, false, window_, 0, 0, width_, height_);
    xcb_flush(conn);
    for (auto ptr = state_; ptr[0] || ptr[1]; ptr += 2) {
      std::vector<xcb_point_t> poly;
      for (; ptr[0] || ptr[1]; ptr += 2)
        poly.push_back({static_cast<int16_t>(width_ * ptr[0] >> 8), static_cast<int16_t>(height_ * ptr[1] >> 8)});
      xcb_fill_poly(conn, window_, context_, XCB_POLY_SHAPE_NONCONVEX, XCB_COORD_MODE_ORIGIN, poly.size(), poly.data());
    }
  }

  void Resize(int width, int height) {
    width_ = width;
    height_ = height;
  }

  void SetState(const uint8_t* state) {
    state_ = state;
  }
};

void HandleXcbEvent(xcb_connection_t* conn, xcb_generic_event_t* evt, WidgetView& view) {
  switch (evt->response_type & ~0x80) {
    case XCB_EXPOSE: {
      view.Update(conn);
      break;
    }
    case XCB_RESIZE_REQUEST: {
      auto req = reinterpret_cast<xcb_resize_request_event_t*>(evt);
      view.Resize(req->width, req->height);
      view.Update(conn);
      break;
    }
    default:
      break;
  }
}

void HandleWidgetEvent(xcb_connection_t* conn, const std::string& fname, WidgetView& view) {
  static std::string buffer;
  buffer = util::ReadFile(fname);
  view.SetState(reinterpret_cast<const uint8_t*>(buffer.data()));
  view.Update(conn);
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc < 3) throw std::runtime_error("Invalid arguments");
    xcb::Connection conn(xcb_connect(nullptr, nullptr), &xcb_disconnect);
    WidgetView view(conn.get(), std::atoi(argv[2]));
    HandleWidgetEvent(conn.get(), argv[1], view);
    int inotify_fd = inotify_init();
    if (inotify_fd < 0) throw std::runtime_error("Inotify initialization failed");
    if (inotify_add_watch(inotify_fd, argv[1], IN_MODIFY) < 0)
      throw std::runtime_error("Failed to configure inotify");
    for (;;) {
      std::vector<pollfd> fds = {
        {xcb_get_file_descriptor(conn.get()), POLLIN},
        {inotify_fd, POLLIN}
      };
      if (poll(fds.data(), fds.size(), -1) < 0)
        util::ThrowSystemError("Polling failed");
      if (fds[0].revents & POLLIN) {
        xcb::Event evt(xcb_poll_for_event(conn.get()), &free);
        if (!evt) break;
        HandleXcbEvent(conn.get(), evt.get(), view);
      }
      if (fds[1].revents & POLLIN) {
        char buffer[4096];
        read(fds[1].fd, buffer, sizeof(buffer));
        HandleWidgetEvent(conn.get(), argv[1], view);
      }
      xcb_flush(conn.get());
    }
    return 0;
  } catch (const std::exception& ex) {
    PrintException(ex);
    return 1;
  }
}
