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
    virtual void onDisconnect();

    virtual bool requestDeviceInfo();
    virtual bool requestCellInfo();

    struct DeviceInfo {
        char vendorId[16] = "";
        char hardwareVersion[8] = "";
        char softwareVersion[8] = "";
        int32_t uptime = 0;  // seconds
        int32_t powerOnCount = 0;
        char deviceName[16] = "";
        char devicePasscode[16] = "";
        char manufacturingDate[8] = "";
        char serialNumber[11] = "";
        char passcode[5] = "";
        char userData[16] = "";
        char setupPasscode[16] = "";
    } deviceInfo;

    struct Settings {
        float cellUVP = 0.0f;   // undervoltage protection
        float cellUVPR = 0.0f;  // undervoltage protection recovery
        float cellOVP = 0.0f;   // overvoltage protection
        float cellOVPR = 0.0f;  // overvoltage protection recovery
        float balanceTriggerVoltage = 0.0f;
        float powerOffVoltage = 0.0f;
        float maxChargeCurrent = 0.0f;
        int32_t chargeOCPDelay = 0;   // charge overcurrent protection time in seconds
        int32_t chargeOCPRDelay = 0;  // charge overcurrent protection recovery time in seconds
        float maxDischargeCurrent = 0.0f;
        int32_t dischargeOCPDelay = 0;   // discharge overcurrent protection delay in seconds
        int32_t dischargeOCPRDelay = 0;  // // discharge overcurrent protection recovery delay in seconds
        int32_t scpRecoveryTime = 0;     // shortcircuit protection recovery time in seconds
        float maxBalanceCurrent = 0.0f;
        float chargeOTP = 0.0f;      // charge overtemperature protection in C
        float chargeOTPR = 0.0f;     // charge overtemperature protection recovery in C
        float dischargeOTP = 0.0f;   // discharge overtemperature protection in C
        float dischargeOTPR = 0.0f;  // discharge overtemperature protection recovery in C
        float chargeUTP = 0.0f;      // charge undertemperature protection in C
        float chargeUTPR = 0.0f;     // charge undertemperature protection recovery in C
        float mosOTP = 0.0f;         // mos overtemperature protection in C
        float mosOTPR = 0.0f;        // mos overtemperature protection recovery in  C
        uint32_t cellCount = 0;
        bool chargingSwitch = false;
        bool dischargingSwitch = false;
        bool balancerSwitch = false;
        float nominalCapacity = 0.0f;
        float balanceStartingVoltage = 0.0f;
        float conWireResistance[24] = {
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f};
        bool disableTemperatureSensors = false;
        bool displayAlwaysOn = false;
    } settings;

    struct CellInfo {
        ulong lastUpdate = 0;

        struct Cell {
            float voltage = 0.0f;
            float resistance = 0.0f;
        } cells[32];

        float lowestCellVoltage = 0.0f;
        float highestCellVoltage = 0.0f;
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

    void printDeviceInfo();
    void printSettings();
    void printCellInfo();

    typedef std::function<void(PeerCharacteristicJkBms*)> Callback;
    Callback onDeviceInfoUpdate = nullptr;
    Callback onSettingsUpdate = nullptr;
    Callback onCellInfoUpdate = nullptr;

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
    void decodeDeviceInfo(const std::vector<uint8_t>& data);
    void decodeJk02Settings(const std::vector<uint8_t>& data);
    void decodeJk04Settings(const std::vector<uint8_t>& data);
    void decodeJk02CellInfo(const std::vector<uint8_t>& data);
    void decodeJk04CellInfo(const std::vector<uint8_t>& data);

    std::string errorToString(const uint16_t mask);
    std::string modeToString(const uint16_t mask);
    uint8_t crc(const uint8_t data[], const uint16_t len);
    void printHex(const uint8_t* s, size_t size, const char* pre = "");
};

}  // namespace Atoll

#endif