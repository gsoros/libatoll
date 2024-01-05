#if !defined(__atoll_peer_characteristic_jkbms_h) && defined(FEATURE_BLE_CLIENT)
#define __atoll_peer_characteristic_jkbms_h

#include "atoll_peer_characteristic_template.h"

namespace Atoll {

class PeerCharacteristicJkBms : public PeerCharacteristicTemplate<String> {
   public:
    PeerCharacteristicJkBms(const char* label = "JkBms",
                            BLEUUID serviceUuid = BLEUUID(SERVICE_UUID),
                            BLEUUID charUuid = BLEUUID(CHAR_UUID));
    virtual String decode(const uint8_t* data, const size_t length) override;
    virtual bool encode(const String value, uint8_t* data, size_t length) override;
    virtual void onNotify(BLERemoteCharacteristic* c, uint8_t* data, size_t length, bool isNotify) override;
    virtual void notify() override;
    virtual bool subscribeOnConnect() override;
    virtual bool readOnSubscribe() override;
    virtual void onSubscribe(BLEClient* client) override;

    virtual bool requestDeviceInfo();
    virtual bool requestCellInfo();

    struct CellInfo {
        ulong lastUpdate = 0;

        struct Cell {
            float voltage = 0.0f;
            float resistance = 0.0f;
        } cells[32];

        float cellVoltageMin = 0.0f;
        float cellVoltageMax = 0.0f;
        float cellVoltageAvg = 0.0f;
        float cellVoltageDelta = 0.0f;
        uint8_t cellVoltageMinId = 0;
        uint8_t cellVoltageMaxId = 0;

        float temp0 = 0.0f;
        float temp1 = 0.0f;
        float temp2 = 0.0f;

        float voltage = 0.0f;
        float chargeCurrent = 0.0f;
        float power = 0.0f;
        float powerCharge = 0.0f;
        float powerDischarge = 0.0f;

        float balanceCurrent = 0.0f;

        uint8_t soc = 0;
        float capacityRemaining = 0.0f;
        float capacityNominal = 0.0f;
        uint32_t cycleCount = 0;
        float capacityCycle = 0.0f;
        uint32_t totalRuntime = 0;

        bool chargingEnabled = false;
        bool dischargingEnabled = false;

        char errors[512] = "";
    } cellInfo;

    // https://github.com/syssi/esphome-jk-bms/blob/main/components/jk_bms_ble/jk_bms_ble.cpp

    static const uint16_t SERVICE_UUID = 0xffe0;
    static const uint16_t CHAR_UUID = 0xffe1;

   protected:
    enum ProtocolVersion {
        PROTOCOL_VERSION_JK04,
        PROTOCOL_VERSION_JK02,
        PROTOCOL_VERSION_JK02_32S,
    };
    ProtocolVersion protocolVersion{PROTOCOL_VERSION_JK02};

    static const uint8_t FRAME_VERSION_JK04 = 0x01;
    static const uint8_t FRAME_VERSION_JK02 = 0x02;
    static const uint8_t FRAME_VERSION_JK02_32S = 0x03;

    /*
        static const uint8_t BATTERY_TYPES_SIZE = 3;
        static constexpr char* const BATTERY_TYPES[BATTERY_TYPES_SIZE] = {
            "Lithium Iron Phosphate",  // 0x00
            "Ternary Lithium",         // 0x01
            "Lithium Titanate",        // 0x02
        };
    */

    std::vector<uint8_t> frameBuffer;
    uint32_t cellInfoMinDelay = 1000;

    static const uint8_t COMMAND_CELL_INFO = 0x96;
    static const uint8_t COMMAND_DEVICE_INFO = 0x97;
    static const uint16_t MIN_RESPONSE_SIZE = 300;
    static const uint16_t MAX_RESPONSE_SIZE = 320;

    bool writeRegister(uint8_t address, uint32_t value, uint8_t length);
    void assemble(const uint8_t* data, uint16_t length);
    void decode(const std::vector<uint8_t>& data);
    void decodeJk02CellInfo(const std::vector<uint8_t>& data);

    // void decodeJk04CellInfo(const std::vector<uint8_t>& data)
    // void decodeJk02Settings(const std::vector<uint8_t>& data)
    // void decodeJk04Settings(const std::vector<uint8_t>& data)
    // void decodeDeviceInfo(const std::vector<uint8_t>& data)

    std::string errorToString(const uint16_t mask);
    std::string modeToString(const uint16_t mask);
    uint8_t crc(const uint8_t data[], const uint16_t len);
    void printHex(const uint8_t* s, size_t size, const char* pre = "");
};

}  // namespace Atoll

#endif