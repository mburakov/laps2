#ifndef LAPS2_NM_GLUE_H_
#define LAPS2_NM_GLUE_H_

#include "org.freedesktop.NetworkManager.AccessPoint-proxy.h"
#include "org.freedesktop.NetworkManager.Connection.Active-proxy.h"
#include "org.freedesktop.NetworkManager-proxy.h"
#include <functional>

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

using Props = std::map<std::string, ::DBus::Variant>;
using PropsCb = std::function<void(const Props&)>;

}  // namespace nm

namespace org {
namespace freedesktop {
namespace NetworkManager {

class AccessPointImpl : public AccessPoint_proxy, public DBus::ObjectProxy {
 private:
  const nm::PropsCb props_cb_;

 public:
  AccessPointImpl(DBus::Connection& conn, const std::string& path, const std::string& name, const nm::PropsCb& props_cb) :
    ObjectProxy(conn, path, name.c_str()), props_cb_(props_cb) {}

  void PropertiesChanged(const nm::Props& props) override {
    props_cb_(props);
  }
};

}  // namespace NetworkManager
}  // namespace freedesktop
}  // namespace org

namespace org {
namespace freedesktop {
namespace NetworkManager {
namespace Connection {

class ActiveImpl : public Active_proxy, public DBus::ObjectProxy {
 private:
  const nm::PropsCb props_cb_;

 public:
  ActiveImpl(DBus::Connection& conn, const std::string& path, const std::string& name, const nm::PropsCb& props_cb) :
    ObjectProxy(conn, path, name.c_str()), props_cb_(props_cb) {}

  void PropertiesChanged(const nm::Props& props) override {
    props_cb_(props);
  }
};

}  // namespace Connection
}  // namespace NetworkManager
}  // namespace freedesktop
}  // namespace org

namespace org {
namespace freedesktop {

class NetworkManagerImpl : public NetworkManager_proxy, public DBus::ObjectProxy {
 private:
  const nm::PropsCb props_cb_;

 public:
  NetworkManagerImpl(DBus::Connection& conn, const std::string& path, const std::string& name, const nm::PropsCb& props_cb) :
    ObjectProxy(conn, path, name.c_str()), props_cb_(props_cb) {}

  void DeviceRemoved(const ::DBus::Path &argin0) override {}
  void DeviceAdded(const ::DBus::Path &argin0) override {}
  void StateChanged(const uint32_t &argin0) override {}
  void CheckPermissions() override {}

  void PropertiesChanged(const nm::Props& props) override {
    props_cb_(props);
  }
};

}  // namespace freedesktop
}  // namespace org

#endif  // LAPS2_NM_GLUE_H_
