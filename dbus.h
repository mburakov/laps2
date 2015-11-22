#ifndef LAPS2_DBUS_H_
#define LAPS2_DBUS_H_

#include "util.h"
#include <poll.h>
#include <memory>
#include <vector>
#include <dbus/dbus.h>

namespace dbus {

class Error : public std::exception, public util::NonCopyable {
 private:
  DBusError impl_;

 public:
  Error();
  Error(Error&& op);
  ~Error();
  bool IsSet() const;
  operator DBusError*();
  const char* what() const noexcept override;
};

class Connection : public util::NonCopyable {
 private:
  std::unique_ptr<DBusConnection, decltype(&dbus_connection_unref)> impl_;
  static DBusConnection* CtorHelper(DBusBusType type);

 public:
  Connection(DBusBusType type);
  Connection(Connection&& op);
  operator DBusConnection*();
};

struct Watch : public util::Holder<DBusWatch> {
  Watch(DBusWatch* op);
  bool IsEnabled() const;
  int GetFd() const;
  bool operator==(const Watch& op) const;
  bool operator==(const pollfd& op) const;
  operator DBusWatch*();
};

struct Message : public util::Holder<DBusMessage> {
  Message(DBusMessage* op, DtorPtr dtor = dbus_message_unref);
  operator DBusMessage*();
};

struct Tree : public std::vector<std::pair<std::string, Tree>> {
  std::string data;
  Tree(const std::string& op = {});
  Tree(const_iterator a, const_iterator b);
};

Message Request(Connection& conn, Message& req);
Tree Parse(Message&& op);
void DumpTree(const Tree& tree, int depth = 0);

}  // namespace dbus

#endif  // LAPS2_DBUS_H_
