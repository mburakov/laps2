#include "util.h"
#include "xcb.h"

namespace xcb {

xcb_window_t FindTray(xcb_connection_t* conn, int screen) {
  try {
    const auto& name = "_NET_SYSTEM_TRAY_S" + std::to_string(screen);
    auto atom = xcb::Sync(&xcb_intern_atom_reply, &xcb_intern_atom_reply_t::atom,
                          &xcb_intern_atom, conn, 1, name.size(), name.data());
    return xcb::Sync(&xcb_get_selection_owner_reply, &xcb_get_selection_owner_reply_t::owner,
                     &xcb_get_selection_owner, conn, atom);
  } catch (const std::exception&) {
    std::throw_with_nested(std::runtime_error("Can not find tray"));
  }
}

void Embed(xcb_connection_t* conn, xcb_window_t tray, xcb_window_t win) {
  static const char atom_name[] = "_NET_SYSTEM_TRAY_OPCODE";
  xcb_client_message_event_t evt = {0};
  evt.response_type = XCB_CLIENT_MESSAGE;
  evt.format = 32;
  evt.window = tray;
  evt.type = xcb::Sync(&xcb_intern_atom_reply, &xcb_intern_atom_reply_t::atom,
                       &xcb_intern_atom, conn, 1, util::Length(atom_name) - 1, atom_name);
  evt.data.data32[0] = XCB_CURRENT_TIME;
  evt.data.data32[1] = 0;
  evt.data.data32[2] = win;
  xcb_send_event(conn, 0, tray, XCB_EVENT_MASK_NO_EVENT, reinterpret_cast<char*>(&evt));
}

}  // namespace xcb
