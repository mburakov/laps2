#ifndef LAPS2_NM_GLUE_H_
#define LAPS2_NM_GLUE_H_

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

#endif  // LAPS2_NM_GLUE_H_
