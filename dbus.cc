#include "dbus.h"
#include <iostream>
#include <map>

namespace {

void ParseImpl(dbus::Tree& self, DBusMessageIter& iter);

template<class T>
inline T GetBasic(DBusMessageIter& iter) {
  T result;
  dbus_message_iter_get_basic(&iter, &result);
  return result;
}

void ParseBasic(dbus::Tree& self, DBusMessageIter& iter) {
  auto type = dbus_message_iter_get_arg_type(&iter);
  switch (dbus_message_iter_get_arg_type(&iter)) {
    case DBUS_TYPE_OBJECT_PATH:
    case DBUS_TYPE_STRING:
      self.emplace_back(std::string(), dbus::Tree(GetBasic<char*>(iter)));
      break;
    case DBUS_TYPE_BYTE:
      self.emplace_back(std::string(), dbus::Tree(std::to_string(GetBasic<uint8_t>(iter))));
      break;
    default:
      const std::string message("Type not implemented: ");
      throw std::runtime_error(message + static_cast<char>(type));
  }
}

void ParseVariant(dbus::Tree& self, DBusMessageIter& iter) {
  DBusMessageIter sub;
  dbus_message_iter_recurse(&iter, &sub);
  ParseImpl(self, sub);
}

void ParseArray(dbus::Tree& self, DBusMessageIter& iter) {
  DBusMessageIter sub;
  dbus_message_iter_recurse(&iter, &sub);
  while (dbus_message_iter_get_arg_type(&sub) != DBUS_TYPE_INVALID) {
    dbus::Tree subtree;
    ParseImpl(subtree, sub);
    self.emplace_back(std::string(), subtree);
    dbus_message_iter_next(&sub);
  }
}

void ParseDictEntry(dbus::Tree& self, DBusMessageIter& iter) {
  DBusMessageIter sub;
  dbus_message_iter_recurse(&iter, &sub);
  {
    dbus::Tree internal;
    ParseImpl(internal, sub);
    if (internal.empty())
      throw std::runtime_error("Empty structures are not allowed");
    const auto& key = internal.front().second.data;
    dbus::Tree value(std::next(internal.begin()), internal.end());
    self.emplace_back(key, value.size() == 1 ? dbus::Tree(value.front().second.data) : value);
  }
}

void ParseImpl(dbus::Tree& self, DBusMessageIter& iter) {
  static const std::map<int, void(*)(dbus::Tree&, DBusMessageIter&)> kHandlers = {
    {DBUS_TYPE_VARIANT, ParseVariant},
    {DBUS_TYPE_ARRAY, ParseArray},
    {DBUS_TYPE_DICT_ENTRY, ParseDictEntry}
  };
  for (;;) {
    auto type = dbus_message_iter_get_arg_type(&iter);
    if (type == DBUS_TYPE_INVALID) return;
    auto it = kHandlers.find(type);
    (it == kHandlers.end() ? ParseBasic : it->second)(self, iter);
    dbus_message_iter_next(&iter);
  }
}

}  // namespace

namespace dbus {

Error::Error() {
  dbus_error_init(&impl_);
}

Error::Error(Error&& op) {
  dbus_move_error(op, &impl_);
}

Error::~Error() {
  if (dbus_error_is_set(&impl_))
    dbus_error_free(&impl_);
}

bool Error::IsSet() const {
  return dbus_error_is_set(&impl_);
}

Error::operator DBusError*() {
  return &impl_;
}

const char* Error::what() const noexcept {
  return dbus_error_is_set(&impl_) ?
      impl_.message : "Empty dbus error thrown";
}

DBusConnection* Connection::CtorHelper(DBusBusType type) {
  Error error;
  DBusConnection* result = dbus_bus_get(type, error);
  if (error.IsSet()) throw error;
  return result;
}

Connection::Connection(DBusBusType type) :
    impl_(CtorHelper(type), dbus_connection_unref) {}

Connection::Connection(Connection&& op) :
    impl_(std::move(op.impl_)) {}

Connection::operator DBusConnection*() {
  return impl_.get();
}

Watch::Watch(DBusWatch* op) :
    Base(op, nullptr) {}

bool Watch::IsEnabled() const {
  return dbus_watch_get_enabled(impl_);
}

int Watch::GetFd() const {
  return dbus_watch_get_unix_fd(impl_);
}

bool Watch::operator==(const Watch& op) const {
  return impl_ == op.impl_;
}

bool Watch::operator==(const pollfd& op) const {
  return GetFd() == op.fd;
}

Watch::operator DBusWatch*() {
  return impl_;
}

Message::Message(DBusMessage* op, DtorPtr dtor) :
    Base(op, dtor) {}

Message::operator DBusMessage*() {
  return impl_;
}

Tree::Tree(const std::string& op) :
    data(op) {}

Tree::Tree(const_iterator a, const_iterator b) :
    std::vector<std::pair<std::string, Tree>>(a, b) {}

Message Request(Connection& conn, Message& req) {
  Error error;
  auto reply = dbus_connection_send_with_reply_and_block(conn, req, DBUS_TIMEOUT_USE_DEFAULT, error);
  if (error.IsSet()) throw error;
  return reply;
}

Tree Parse(Message&& op) {
  Tree result;
  DBusMessageIter iter;
  dbus_message_iter_init(op, &iter);
  ParseImpl(result, iter);
  return result;
}

void DumpTree(const Tree& tree, int depth) {
  for (const auto& it : tree) {
    std::cout << std::string(depth * 2, ' ') << it.first << ": " << it.second.data << std::endl;
    DumpTree(it.second, depth + 1);
  }
}

}  // namespace dbus
