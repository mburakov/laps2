#include "widget.h"
#include "xcb.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <sys/poll.h>

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

  static xcb_window_t CreateWindow(xcb_connection_t* conn, int width, int height) {
    auto result = xcb_generate_id(conn);
    auto screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t values[] = {screen->white_pixel, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_RESIZE_REDIRECT};
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, result, screen->root, 0, 0, width, height, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, XCB_COPY_FROM_PARENT, mask, values);
    return result;
  }

  static xcb_gcontext_t CreateContext(xcb_connection_t* conn) {
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

  WidgetView(xcb_connection_t* conn, int screen_number) :
      width_(-1), height_(-1),
      state_(nullptr),
      window_(CreateWindow(conn, width_, height_)),
      context_(CreateContext(conn)) {
    auto tray = xcb::FindTray(conn, screen_number);
    xcb::Embed(conn, tray, window_);
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

  bool operator==(xcb_window_t op) const {
    return window_ == op;
  }
};

using WidgetsBinding = std::list<std::pair<Widget*, WidgetView>>;

void HandleXcbEvent(xcb_connection_t* conn, xcb_generic_event_t* evt, WidgetsBinding& widgets) {
  static const auto find_widget = [](auto& widgets, auto window) {
    return std::find_if(widgets.begin(), widgets.end(), [window](const auto& op){return op.second == window;});
  };

  switch (evt->response_type & ~0x80) {
    case XCB_EXPOSE: {
      auto req = reinterpret_cast<xcb_expose_event_t*>(evt);
      auto it = find_widget(widgets, req->window);
      if (it != widgets.end()) it->second.Update(conn);
      break;
    }
    case XCB_RESIZE_REQUEST: {
      auto req = reinterpret_cast<xcb_resize_request_event_t*>(evt);
      auto it = find_widget(widgets, req->window);
      if (it != widgets.end()) {
        it->second.Resize(req->width, req->height);
        it->second.Update(conn);
      }
      break;
    }
    default:
      break;
  }
}

void HandleWidgetEvent(xcb_connection_t* conn, int fd, WidgetsBinding& widgets) {
  static const auto find_widget = [](auto& widgets, auto fd) {
    return std::find_if(widgets.begin(), widgets.end(), [fd](const auto& op) {
      const auto& fds = op.first->GetPollFd();
      return std::find(fds.begin(), fds.end(), fd) != fds.end();
    });
  };

  auto it = find_widget(widgets, fd);
  if (it != widgets.end()) {
    it->first->Handle();
    it->second.SetState(it->first->GetState());
    it->second.Update(conn);
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    int screen_number;
    xcb::Connection conn(xcb_connect(nullptr, &screen_number), &xcb_disconnect);
    WidgetsBinding widgets;
    for (auto it : WidgetList::Get()) try {
      it->Init(argc, argv);
      WidgetView view(conn.get(), screen_number);
      view.SetState(it->GetState());
      widgets.emplace_back(it, std::move(view));
    } catch (const std::exception& ex) {
      PrintException(ex);
    }
    xcb_flush(conn.get());
    for (;;) {
      std::vector<pollfd> fds = {{xcb_get_file_descriptor(conn.get()), POLLIN}};
      for (const auto& it : widgets) {
        for (int fd : it.first->GetPollFd())
          fds.push_back({fd, POLLIN});
      }
      if (poll(fds.data(), fds.size(), -1) < 0)
        util::ThrowSystemError("Polling failed");
      if (fds[0].revents & POLLIN) {
        xcb::Event evt(xcb_poll_for_event(conn.get()), &free);
        if (!evt) break;
        HandleXcbEvent(conn.get(), evt.get(), widgets);
      }
      for (const auto& it : fds) {
        if (it.revents & POLLIN)
          HandleWidgetEvent(conn.get(), it.fd, widgets);
      }
      xcb_flush(conn.get());
    }
    return 0;
  } catch (const std::exception& ex) {
    PrintException(ex);
    return 1;
  }
}
