#include "resources.h"
#include "widget.h"
#include <fstream>
#include <linux/netlink.h>
#include <sys/socket.h>

namespace {

const uint8_t* kChargeLevel[] = {
  battery_00c, battery_01c, battery_02c, battery_03c, battery_04c,
  battery_05c, battery_06c, battery_07c, battery_08c, battery_09c
};

const uint8_t* kDrainLevel[] = {
  battery_00d, battery_01d, battery_02d, battery_03d, battery_04d,
  battery_05d, battery_06d, battery_07d, battery_08d, battery_09c
};

// TODO(Micha): Why util::Length does not work here?
constexpr auto kNumStates = sizeof(kChargeLevel) / sizeof(*kChargeLevel);
static_assert(kNumStates == sizeof(kDrainLevel) / sizeof(*kDrainLevel),
              "Mismatching number of battery charge levels");

struct : public Widget {
  // TODO(Micha): Replace with RaiiFd
  int sock_;

  void Init(int argc, char** argv) {
    // TODO(Micha): Handle arguments
    sockaddr_nl sa = {AF_NETLINK, 0, 0, ~0u};
    sock_ = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (sock_ < 0) throw std::runtime_error("Failed to create udev socket");
    if (bind(sock_, (struct sockaddr*)&sa, sizeof(struct sockaddr_nl)))
      throw std::runtime_error("Failed to bind udev socket");
  }

  const uint8_t* GetState() {
    static const char kTotal[] = "/sys/class/power_supply/BAT1/charge_full";
    static const char kCurrent[] = "/sys/class/power_supply/BAT1/charge_now";
    static const char kStatus[] = "/sys/class/power_supply/BAT1/status";

    int total = std::atoi(util::ReadFile(kTotal).c_str());
    int current = std::atoi(util::ReadFile(kCurrent).c_str());
    const auto& status = util::ReadFile(kStatus);
    auto source = status == "Charging" ? kChargeLevel : kDrainLevel;
    return source[kNumStates * current / total];
  }

  int GetPollFd() {
    return sock_;
  }

  void Activate() {
  }

  void Handle() {
    // TODO(Micha): Add normal receiver here
    for (char byte, res = 1; res > 0; res = recv(sock_, &byte, 1, MSG_DONTWAIT));
  }
} __impl__;

}  // namespace
