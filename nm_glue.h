#ifndef LAPS2_NM_GLUE_H_
#define LAPS2_NM_GLUE_H_

#include "org.freedesktop.NetworkManager.AccessPoint-proxy.h"
#include "org.freedesktop.NetworkManager.Connection.Active-proxy.h"

namespace nm {

enum ConnectionState {
  kUnknown = 0,
  kActivating,
  kActivated,
  kDeactivating,
  kDeactivated
};

const char kEthType[] = "802-3-ethernet";
const char kWifiType[] = "802-11-wireless";

}  // namespace nm

namespace org {
namespace freedesktop {
namespace NetworkManager {

struct AccessPoint : public AccessPoint_proxy, public DBus::ObjectProxy {
  AccessPoint(DBus::Connection& conn, const std::string& path, const std::string& name) :
    ObjectProxy(conn, path, name.c_str()) {}

  void PropertiesChanged(const std::map<std::string, ::DBus::Variant>& props) override {
  }
};

}  // namespace NetworkManager
}  // namespace freedesktop
}  // namespace org

namespace org {
namespace freedesktop {
namespace NetworkManager {
namespace Connection {

struct Active : public Active_proxy, public DBus::ObjectProxy {
  Active(DBus::Connection& conn, const std::string& path, const std::string& name) :
    ObjectProxy(conn, path, name.c_str()) {}

  void PropertiesChanged(const std::map<std::string, ::DBus::Variant>& props) override {
  }
};

}  // namespace Connection
}  // namespace NetworkManager
}  // namespace freedesktop
}  // namespace org

#endif  // LAPS2_NM_GLUE_H_

