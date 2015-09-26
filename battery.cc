#include "resources.h"
#include "widget.h"
#include <cstring>
#include <memory>
#include <libudev.h>

namespace libudev {

using Context = std::unique_ptr<udev, decltype(&udev_unref)>;
using Monitor = std::unique_ptr<udev_monitor, decltype(&udev_monitor_unref)>;
using Device = std::unique_ptr<udev_device, decltype(&udev_device_unref)>;

}  // namespace udev

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

const char kDeviceRoot[] = "/sys/class/power_supply";

struct : public Widget {
  const char* device_name_{"BAT1"};

  libudev::Context udev_{nullptr, udev_unref};
  libudev::Monitor monitor_{nullptr, udev_monitor_unref};
  int fd_;

  bool charging_;
  int current_;
  int total_;

  void Init(int argc, char** argv) {
    // TODO(Micha): Handle arguments
    const auto& base_path = std::string(kDeviceRoot) + "/" + device_name_ + "/";
    charging_ = util::ReadFile(base_path + "status") == "Charging\n";
    current_ = std::atoi(util::ReadFile(base_path + "charge_now").c_str());
    total_ = std::atoi(util::ReadFile(base_path + "charge_full").c_str());
    udev_.reset(udev_new());

    if (!udev_) throw std::runtime_error("Failed to create udev context");
    monitor_.reset(udev_monitor_new_from_netlink(udev_.get(), "udev"));
    if (!monitor_) throw std::runtime_error("Failed to create udev monitor");
    if (udev_monitor_filter_add_match_subsystem_devtype(monitor_.get(), "power_supply", nullptr) < 0)
      throw std::runtime_error("Failed to add udev devtype filter");
    fd_ = udev_monitor_get_fd(monitor_.get());
    udev_monitor_enable_receiving(monitor_.get());
  }

  const uint8_t* GetState() {
    auto source = charging_ ? kChargeLevel : kDrainLevel;
    return source[kNumStates * current_ / total_];
  }

  std::list<int> GetPollFd() {
    return {fd_};
  }

  void Activate() {
  }

  void Handle() {
    libudev::Device device(udev_monitor_receive_device(monitor_.get()), udev_device_unref);
    if (std::strcmp(udev_device_get_sysname(device.get()), device_name_)) return;
    charging_ = !std::strcmp(udev_device_get_property_value(device.get(), "POWER_SUPPLY_STATUS"), "Charging");
    current_ = std::atoi(udev_device_get_property_value(device.get(), "POWER_SUPPLY_CHARGE_NOW"));
    total_ = std::atoi(udev_device_get_property_value(device.get(), "POWER_SUPPLY_CHARGE_FULL"));
  }
} __impl__;

}  // namespace
