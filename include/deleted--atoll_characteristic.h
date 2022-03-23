#ifndef __atoll_characteristic_h
#define __atoll_characteristic_h

#include "atoll_ble_server.h"

namespace Atoll {

#ifndef SETTINGS_STR_LENGTH
#define SETTINGS_STR_LENGTH 32
#endif

class Characteristic : public BLECharacteristic,
                       public BLECharacteristicCallbacks {
   public:
    char label[SETTINGS_STR_LENGTH];
    Characteristic(const char *label,
                   const char *uuid,
                   uint16_t properties =
                       NIMBLE_PROPERTY::READ |
                       NIMBLE_PROPERTY::WRITE,
                   BLEService *pService = nullptr)
        : BLECharacteristic(
              uuid,
              properties,
              pService) {
        strncpy(this->label, label, sizeof(this->label));
    }
    Characteristic(const char *label,
                   const BLEUUID &uuid,
                   uint16_t properties =
                       NIMBLE_PROPERTY::READ |
                       NIMBLE_PROPERTY::WRITE,
                   BLEService *pService = nullptr)
        : BLECharacteristic(
              uuid,
              properties,
              pService) {
        strncpy(this->label, label, sizeof(this->label));
    }

    BLEDescriptor *createDescriptor(
        const BLEUUID &uuid,
        uint32_t properties = NIMBLE_PROPERTY::READ,
        const uint8_t *initialValue = nullptr,
        const size_t initialValueSize = 0) {
        BLEDescriptor *d = createDescriptor(uuid, properties);
        if (nullptr != initialValue && 0 != initialValueSize)
            d->setValue(initialValue, initialValueSize);
        return d;
    }
}
};
}  // namespace Atoll

#endif