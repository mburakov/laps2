#include "dbus.h"
#include "nm_glue.h"
#include "resources.h"
#include "widget.h"
#include <algorithm>

namespace {

const char kDBusApFilter[] = "type='signal',interface='org.freedesktop.NetworkManager.AccessPoint'";
const char kDBusApIface[] = "org.freedesktop.NetworkManager.AccessPoint";
const char kDBusConIface[] = "org.freedesktop.NetworkManager.Connection.Active";
const char kDBusDest[] = "org.freedesktop.NetworkManager";
const char kDBusNmIface[] = "org.freedesktop.NetworkManager";
const char kDBusNmPath[] = "/org/freedesktop/NetworkManager";

const uint8_t* kSignalLevel[] = {
  wifi_00, wifi_01, wifi_02, wifi_03, wifi_04
};

struct NmWidget : public Widget {
  std::list<dbus::Watch> watches_;
  const uint8_t* icon_;

  std::unique_ptr<dbus::Connection> connection_;

  std::string GetSingleProperty(const std::string& path, const std::string& iface, const std::string& prop) {
    try {
      const char kPropCommand[] = "Get";
      const char kPropIface[] = "org.freedesktop.DBus.Properties";
      const char* iface_name = iface.c_str();
      const char* prop_name = prop.c_str();
      dbus::Message get_prop(dbus_message_new_method_call(kDBusDest, path.c_str(), kPropIface, kPropCommand));
      dbus_message_append_args(get_prop,
          DBUS_TYPE_STRING, &iface_name,
          DBUS_TYPE_STRING, &prop_name,
          DBUS_TYPE_INVALID);
      const auto& reply = dbus::Parse(dbus::Request(*connection_, get_prop));
      if (reply.empty()) throw std::runtime_error("Invalid reply format");
      return reply.front().second.data;
    } catch (const std::exception&) {
      std::throw_with_nested(std::runtime_error("Failed to get property"));
    }
  }

  void Init(int argc, char** argv) override {
    // TODO(Micha): Parse commandline arguments
    connection_ = std::make_unique<decltype(connection_)::element_type>(DBUS_BUS_SYSTEM);
    auto watch_rc = dbus_connection_set_watch_functions(*connection_,
        NmWidget::OnAddWatch, NmWidget::OnRemoveWatch, nullptr, this, nullptr);
    if (!watch_rc) throw std::runtime_error("Failed to setup dbus watch functions");
    dbus::Error error;
    dbus_bus_add_match(*connection_, kDBusApFilter, error);
    if (error.IsSet()) throw error;
    icon_ = kSignalLevel[0];
  }

  const uint8_t* GetState() override {
    return icon_;
  }

  std::vector<pollfd> GetPollFds() override {
    decltype(GetPollFds()) result;
    for (auto& it : watches_) {
      if (!it.IsEnabled()) continue;
      short flags = dbus_watch_get_flags(it);
      result.push_back({it.GetFd(), flags});
    }
    return result;
  }

  void Activate() override {
  }

  void Handle(const pollfd&) override {
    dbus_connection_read_write_dispatch(*connection_, -1);
    const auto& connection = GetSingleProperty(kDBusNmPath, kDBusNmIface, "PrimaryConnection");
    const auto& type = GetSingleProperty(connection, kDBusConIface, "Type");
    if (type == nm::kEthType) {
      icon_ = ethernet;
      return;
    }
    const auto& access_point = GetSingleProperty(connection, kDBusConIface, "SpecificObject");
    auto strength = std::stoi(GetSingleProperty(access_point, kDBusApIface, "Strength").c_str());
    icon_ = kSignalLevel[strength * util::Length(kSignalLevel) / 100];
  }

  static dbus_bool_t OnAddWatch(DBusWatch* watch, void* data) {
    auto self = reinterpret_cast<NmWidget*>(data);
    self->watches_.push_back(watch);
    return true;
  }

  static void OnRemoveWatch(DBusWatch* watch, void* data) {
    auto self = reinterpret_cast<NmWidget*>(data);
    self->watches_.remove(watch);
  }
} __impl__;

}  // namespace
