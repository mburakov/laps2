#include "nm_glue.h"
#include "widget.h"
#include <memory>

namespace {

const char kActiveServiceName[] = "org.freedesktop.NetworkManager.Connection.Active";
const char kActiveObjectPath[] = "/org/freedesktop/NetworkManager/ActiveConnection/0";

struct : public Widget {
  // TODO(Micha): Replace with RaiiFd
  int fd_ = -1;

  DBus::BusDispatcher dispatcher_;
  std::unique_ptr<DBus::Connection> connection_;
  std::unique_ptr<org::freedesktop::NetworkManager::Connection::Active> active_;
  std::unique_ptr<org::freedesktop::NetworkManager::AccessPoint> access_point_;

  void Init(int argc, char** argv) {
    // TODO(Micha): Parse commandline arguments
    DBus::default_dispatcher = &dispatcher_;
    connection_ = std::make_unique<decltype(connection_)::element_type>(DBus::Connection::SystemBus());
    active_ = std::make_unique<decltype(active_)::element_type>(*connection_, kActiveObjectPath, kActiveServiceName);
  }

  const uint8_t* GetState() {
    // TODO(Micha): Check status, select appropriate icon, watch for reconnections
    static const uint8_t kDummyState[] = {
      0x10, 0x10, 0x80, 0x40, 0xf0, 0x10, 0xc0, 0x80, 0xf0, 0xf0, 0x80, 0xc0, 0x10, 0xf0, 0x40, 0x80,
      0x00, 0x00, 0x00, 0x00
    };

    return kDummyState;
  }

  std::list<int> GetPollFd() {
    return {fd_};
  }

  void Activate() {
  }

  void Handle() {
  }
} __impl__;

}  // namespace
