#include "nm_glue.h"
#include "resources.h"
#include "widget.h"
#include <ctime>
#include <iostream>
#include <memory>

namespace {

const char kServiceName[] = "org.freedesktop.NetworkManager";
const char kObjectPath[] = "/org/freedesktop/NetworkManager";

const uint8_t* kSignalLevel[] = {
  wifi_00, wifi_01, wifi_02, wifi_03, wifi_04
};

struct NmWidget : public Widget, public DBus::BusDispatcher {
  // TODO(Micha): Replace with RaiiFd
  std::list<int> fd_;
  const uint8_t* icon_;

  std::unique_ptr<DBus::Connection> connection_;
  std::unique_ptr<org::freedesktop::NetworkManager::AccessPointImpl> access_point_;
  std::unique_ptr<org::freedesktop::NetworkManager::Connection::ActiveImpl> active_;
  std::unique_ptr<org::freedesktop::NetworkManagerImpl> network_manager_;

  void OnNmProp(const nm::Props&) {
    // TODO(Micha): Optimize this
    const auto& primary = network_manager_->PrimaryConnection();
    active_ = std::make_unique<decltype(active_)::element_type>(*connection_,
        primary, kServiceName, std::bind(&NmWidget::OnNmProp, this, std::placeholders::_1));
    if (active_->Type() == nm::kEthType) {
      access_point_.reset();
      icon_ = ethernet;
      return;
    }
    const auto& object = active_->SpecificObject();
    access_point_ = std::make_unique<decltype(access_point_)::element_type>(*connection_,
        object, kServiceName, std::bind(&NmWidget::OnNmProp, this, std::placeholders::_1));
    const auto& strength = access_point_->Strength();
    std::cout << "Strength: " << static_cast<uint32_t>(strength) << std::endl;
    icon_ = kSignalLevel[strength * util::Length(kSignalLevel) / 100];
  }

  void Init(int argc, char** argv) override {
    // TODO(Micha): Parse commandline arguments
    DBus::default_dispatcher = this;
    connection_ = std::make_unique<decltype(connection_)::element_type>(DBus::Connection::SystemBus());
    network_manager_ = std::make_unique<decltype(network_manager_)::element_type>(*connection_,
        kObjectPath, kServiceName, std::bind(&NmWidget::OnNmProp, this, std::placeholders::_1));
    OnNmProp({});
  }

  const uint8_t* GetState() override {
    // TODO(Micha): Check status, select appropriate icon, watch for reconnections
    return icon_;
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
