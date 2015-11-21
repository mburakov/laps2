#include "nm_glue.h"
#include "resources.h"
#include "widget.h"
#include <dbus/dbus.h>
#include <algorithm>
#include <cstring>
#include <memory>

namespace dbus {

class Error : public std::exception, public util::NonCopyable {
 private:
  DBusError impl_;

 public:
  Error() {
    dbus_error_init(&impl_);
  }

  Error(Error&& op) {
    dbus_move_error(op, &impl_);
  }

  ~Error() {
    if (dbus_error_is_set(&impl_))
      dbus_error_free(&impl_);
  }

  bool IsSet() const {
    return dbus_error_is_set(&impl_);
  }

  operator DBusError*() {
    return &impl_;
  }

  const char* what() const noexcept override {
    return dbus_error_is_set(&impl_) ?
        impl_.message : "Empty dbus error thrown";
  }
};

class Connection : public util::NonCopyable {
 private:
  std::unique_ptr<DBusConnection, decltype(&dbus_connection_unref)> impl_;

  static DBusConnection* CtorHelper(DBusBusType type) {
    dbus::Error error;
    DBusConnection* result = dbus_bus_get(type, error);
    if (error.IsSet()) throw error;
    return result;
  }

 public:
  Connection(DBusBusType type) :
      impl_(CtorHelper(type), dbus_connection_unref) {}

  Connection(Connection&& op) :
      impl_(std::move(op.impl_)) {}

  operator DBusConnection*() {
    return impl_.get();
  }
};

class Watch : public util::NonCopyable {
 private:
  DBusWatch* impl_;

 public:
  Watch(DBusWatch* op) :
    impl_(op) {}

  Watch(Watch&& op) {
    impl_ = op.impl_;
    op.impl_ = nullptr;
  }

  bool IsEnabled() const {
    return dbus_watch_get_enabled(impl_);
  }

  int GetFd() const {
    return dbus_watch_get_unix_fd(impl_);
  }

  bool operator==(const Watch& op) const {
    return impl_ == op.impl_;
  }

  bool operator==(const pollfd& op) const {
    return GetFd() == op.fd;
  }

  operator DBusWatch*() {
    return impl_;
  }
};

}  // namespace dbus

namespace {

const char kDBusFilter[] = "type='signal',interface='org.freedesktop.NetworkManager.AccessPoint'";

const uint8_t* kSignalLevel[] = {
  wifi_00, wifi_01, wifi_02, wifi_03, wifi_04
};

struct NmWidget : public Widget {
  std::list<dbus::Watch> watches_;
  const uint8_t* icon_;

  std::unique_ptr<dbus::Connection> connection_;

  void Init(int argc, char** argv) override {
    // TODO(Micha): Parse commandline arguments
    connection_ = std::make_unique<decltype(connection_)::element_type>(DBUS_BUS_SYSTEM);
    auto watch_rc = dbus_connection_set_watch_functions(*connection_,
        NmWidget::OnAddWatch, NmWidget::OnRemoveWatch, nullptr, this, nullptr);
    if (!watch_rc) throw std::runtime_error("Failed to setup dbus watch functions");
    dbus::Error error;
    dbus_bus_add_match(*connection_, kDBusFilter, error);
    if (error.IsSet()) throw error;
    auto filter_rc = dbus_connection_add_filter(*connection_, &NmWidget::OnSignal, this, nullptr);
    if (!filter_rc) throw std::runtime_error("Failed to setup dbus filter function");
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

  void Handle(const pollfd& fd) override {
    auto it = std::find(watches_.begin(), watches_.end(), fd);
    if (it == watches_.end()) throw std::runtime_error("Invalid descriptor");
    if (!dbus_watch_handle(*it, fd.revents))
      throw std::runtime_error("Not enough memory for the dbus operation");
    while (dbus_connection_dispatch(*connection_) != DBUS_DISPATCH_COMPLETE);
    // TODO(Micha): Why this is needed???
    dbus_connection_read_write_dispatch(*connection_, -1);
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

  static DBusHandlerResult OnSignal(DBusConnection* connection, DBusMessage* message, void* data) {
    // TODO(Micha): Rework this function
    auto self = reinterpret_cast<NmWidget*>(data);
    auto msg = message;
    if (!dbus_message_is_signal(msg, "org.freedesktop.NetworkManager.AccessPoint", "PropertiesChanged"))
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    DBusMessageIter args;
    if (!dbus_message_iter_init(msg, &args))
      throw std::runtime_error("Invalid signal received from nm");

    char* signature = dbus_message_iter_get_signature(&args);
    int signature_valid = !std::strcmp(signature, "a{sv}");
    dbus_free(signature);

    if (!signature_valid)
      throw std::runtime_error("Wrong signal signature");

    DBusMessageIter map;
    dbus_message_iter_recurse(&args, &map);
    for (; dbus_message_iter_get_arg_type(&map) != DBUS_TYPE_INVALID; dbus_message_iter_next(&map))
    {
      DBusMessageIter entry;
      dbus_message_iter_recurse(&map, &entry);

      char* name;
      dbus_message_iter_get_basic(&entry, &name);
      if (std::strcmp(name, "Strength"))
        continue;

      DBusMessageIter variant;
      dbus_message_iter_next(&entry);
      dbus_message_iter_recurse(&entry, &variant);
      uint8_t signal_strength;
      dbus_message_iter_get_basic(&variant, &signal_strength);
      self->icon_ = kSignalLevel[signal_strength * util::Length(kSignalLevel) / 100];
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
} __impl__;

}  // namespace
