#ifndef __atoll_api_rx_callbacks_h
#define __atoll_api_rx_callbacks_h

#include "atoll_ble_server.h"
#include "atoll_api.h"

namespace Atoll {
class ApiRxCallbacks : public BLECharacteristicCallbacks {
   public:
    void onWrite(BLECharacteristic* c, BLEConnInfo& connInfo) override;
};

}  // namespace Atoll

#endif