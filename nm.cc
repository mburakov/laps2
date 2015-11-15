#include "nm_glue.h"
#include "resources.h"
#include "widget.h"
#include <iostream>
#include <memory>

namespace {

const char kServiceName[] = "org.freedesktop.NetworkManager";
const char kObjectPath[] = "/org/freedesktop/NetworkManager";

struct NmWidget : public Widget, public DBus::BusDispatcher {
  // TODO(Micha): Replace with RaiiFd
  std::list<int> fd_;

  std::unique_ptr<DBus::Connection> connection_;
  std::unique_ptr<org::freedesktop::NetworkManager::AccessPointImpl> access_point_;
  std::unique_ptr<org::freedesktop::NetworkManager::Connection::ActiveImpl> active_;
  std::unique_ptr<org::freedesktop::NetworkManagerImpl> network_manager_;

  void OnActiveProp(const nm::Props& props) {
    std::cout << "Prop changed" << std::endl;
  }

  void Init(int argc, char** argv) override {
    // TODO(Micha): Parse commandline arguments
    DBus::default_dispatcher = this;
    connection_ = std::make_unique<decltype(connection_)::element_type>(DBus::Connection::SystemBus());
    network_manager_ = std::make_unique<decltype(network_manager_)::element_type>(*connection_,
        kObjectPath, kServiceName, std::bind(&NmWidget::OnActiveProp, this, std::placeholders::_1));
    std::cout << network_manager_->PrimaryConnection() << std::endl;
  }

  const uint8_t* GetState() override {
    // TODO(Micha): Check status, select appropriate icon, watch for reconnections
    return ethernet;
  }

  std::list<int> GetPollFd() override {
    return fd_;
  }

  void Activate() override {
  }

  void Handle() override {
    dispatch_pending();
    dispatch();
  }

  DBus::Watch* add_watch(DBus::Watch::Internal* op) override {
    // TODO(Micha): Cleanup watches
    auto result = DBus::BusDispatcher::add_watch(op);
    fd_.push_back(result->descriptor());
    return result;
  }
} __impl__;

}  // namespace
