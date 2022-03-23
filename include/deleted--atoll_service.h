#ifndef __atoll_service_h
#define __atoll_service_h

#include "atoll_ble_server.h"
#include "atoll_characteristic.h"

#ifndef SETTINGS_STR_LENGTH
#define SETTINGS_STR_LENGTH 32
#endif

namespace Atoll {

class Service : public BLEService {
   public:
    char label[SETTINGS_STR_LENGTH];
    // 0: not advertised; 1: advertised; 2: advertised in scan response;
    uint8_t advertised = 1;
    Service(const char *label,
            const char *uuid,
            uint16_t numHandles,
            BLEServer *pServer)
        : BLEService(
              uuid,
              numHandles,
              pServer) {
        strncpy(this->label, label, sizeof(this->label));
    }
    Service(const char *label,
            const BLEUUID &uuid,
            uint16_t numHandles,
            NimBLEServer *pServer)
        : BLEService(
              uuid,
              numHandles,
              pServer) {
        strncpy(this->label, label, sizeof(this->label));
    }

    Characteristic *createChar(
        const char *label,
        const char *uuid,
        const uint32_t properties = NIMBLE_PROPERTY::READ |
                                    NIMBLE_PROPERTY::WRITE,
        const uint8_t *initialValue = nullptr,
        const size_t initialValueSize = 0) {
        // log_i("label: %s", label);
        // Serial.flush();
        // log_i("uuid: %s", uuid);
        // Serial.flush();
        // log_i("props: %d", properties);
        // Serial.flush();
        // log_i("initValSize: %d", initialValueSize);
        // Serial.flush();

        // char str[initialValueSize + 1];
        // memcpy(str, initialValue, initialValueSize);
        // memcpy(str + initialValueSize, "\n", 1);
        // log_i("initVal: %s", str);
        // Serial.flush();

        log_i("before");
        Serial.flush();
        BLECharacteristic *x = ((BLEService *)this)->createCharacteristic("DEAD");
        log_i("after");
        Serial.flush();
        if (nullptr == x) return nullptr;
        log_i("UUID: %s", x->getUUID().toString().c_str());
        Serial.flush();
        return nullptr;

        Characteristic *c = (Characteristic *)createCharacteristic(uuid, properties);
        if (nullptr == c) return c;
        strncpy(c->label, label, sizeof(c->label));
        // if (nullptr != initialValue && 0 != initialValueSize)
        //     c->setValue(initialValue, initialValueSize);
        c->setCallbacks(c);
        return c;
    }

    std::vector<Characteristic *> getChars() {
        std::vector<BLECharacteristic *> BLEChars = getCharacteristics();
        std::vector<Characteristic *> chars;
        chars.reserve(chars.size());
        for (const auto &c : chars)
            chars.emplace_back((Characteristic *)c);
        return chars;
    }

    Characteristic *getChar(const char *label) {
        for (const auto &c : getChars())
            if (0 == strstr(c->label, label)) return c;
        return nullptr;
    }
};
}  // namespace Atoll

#endif