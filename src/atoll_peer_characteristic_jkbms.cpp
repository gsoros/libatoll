#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED) && defined(FEATURE_BLE_CLIENT)

#include "atoll_peer_characteristic_jkbms.h"

using namespace Atoll;

PeerCharacteristicJkBms::PeerCharacteristicJkBms(const char* label,
                                                 BLEUUID serviceUuid,
                                                 BLEUUID charUuid) {
    snprintf(this->label, sizeof(this->label), "%s", label);
    this->serviceUuid = serviceUuid;
    this->charUuid = charUuid;
}

String PeerCharacteristicJkBms::decode(const uint8_t* data, const size_t length) {
    log_e("not implemented");
    return lastValue;
}

bool PeerCharacteristicJkBms::encode(const String value, uint8_t* data, size_t length) {
    log_e("not implemented");
    return true;
}

void PeerCharacteristicJkBms::onNotify(BLERemoteCharacteristic* c, uint8_t* data, size_t length, bool isNotify) {
    // log_s("JkBms onNotify %d", length);
    assemble(data, length);
    // printHex(data, length, "frame: ");
}

void PeerCharacteristicJkBms::notify() {
}

bool PeerCharacteristicJkBms::subscribeOnConnect() {
    return true;
}

bool PeerCharacteristicJkBms::readOnSubscribe() {
    return false;
}

void PeerCharacteristicJkBms::onSubscribe(BLEClient* client) {
    requestDeviceInfo();
    delay(1000);
    requestCellInfo();
}

bool PeerCharacteristicJkBms::requestDeviceInfo() {
    log_d("requesting...");
    writeRegister(COMMAND_DEVICE_INFO, 0x00000000, 0x00);
    return true;
}

bool PeerCharacteristicJkBms::requestCellInfo() {
    log_d("requesting...");
    writeRegister(COMMAND_CELL_INFO, 0x00000000, 0x00);
    return true;
}

void PeerCharacteristicJkBms::assemble(const uint8_t* data, uint16_t length) {
    if (frameBuffer.size() > MAX_RESPONSE_SIZE) {
        log_w("Frame dropped because of invalid length");
        frameBuffer.clear();
    }

    // Flush buffer on every preamble
    if (data[0] == 0x55 && data[1] == 0xAA && data[2] == 0xEB && data[3] == 0x90) {
        frameBuffer.clear();
    }

    frameBuffer.insert(frameBuffer.end(), data, data + length);

    if (frameBuffer.size() >= MIN_RESPONSE_SIZE) {
        const uint8_t* raw = &frameBuffer[0];
        // Even if the frame is 320 bytes long the CRC is at position 300 in front of 0xAA 0x55 0x90 0xEB
        const uint16_t frameSize = 300;  // frameBuffer.size();

        uint8_t computed_crc = crc(raw, frameSize - 1);
        uint8_t remote_crc = raw[frameSize - 1];
        if (computed_crc != remote_crc) {
            log_w("CRC check failed! 0x%02X != 0x%02X", computed_crc, remote_crc);
            frameBuffer.clear();
            return;
        }

        std::vector<uint8_t> data(frameBuffer.begin(), frameBuffer.end());

        decode(data);
        frameBuffer.clear();
    }
}

void PeerCharacteristicJkBms::decode(const std::vector<uint8_t>& data) {
    uint8_t frameType = data[4];
    switch (frameType) {
        case 0x01:
            if (protocolVersion == PROTOCOL_VERSION_JK04) {
                decodeJk04Settings(data);
            } else {
                decodeJk02Settings(data);
            }
            printSettings();
            break;
        case 0x02:
            if (protocolVersion == PROTOCOL_VERSION_JK04) {
                decodeJk04CellInfo(data);
            } else {
                decodeJk02CellInfo(data);
            }
            break;
        case 0x03:
            decodeDeviceInfo(data);
            printDeviceInfo();
            break;
        default:
            log_w("Unsupported message type (0x%02X)", data[4]);
    }
}

void PeerCharacteristicJkBms::decodeDeviceInfo(const std::vector<uint8_t>& data) {
    auto get16 = [&](size_t i) -> uint16_t { return (uint16_t(data[i + 1]) << 8) | (uint16_t(data[i + 0]) << 0); };
    auto get32 = [&](size_t i) -> uint32_t {
        return (uint32_t(get16(i + 2)) << 16) | (uint32_t(get16(i + 0)) << 0);
    };

    // log_i("Device info frame (%d bytes) received", data.size());
    // log_d("  %s", format_hex_pretty(&data.front(), 160).c_str());
    // log_d("  %s", format_hex_pretty(&data.front() + 160, data.size() - 160).c_str());

    // JK04 (JK-B2A16S v3) response example:
    //
    // 0x55 0xAA 0xEB 0x90 0x03 0xE7 0x4A 0x4B 0x2D 0x42 0x32 0x41 0x31 0x36 0x53 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x33
    // 0x2E 0x30 0x00 0x00 0x00 0x00 0x00 0x33 0x2E 0x33 0x2E 0x30 0x00 0x00 0x00 0x10 0x8E 0x32 0x02 0x13 0x00 0x00 0x00
    // 0x42 0x4D 0x53 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x31 0x32 0x33 0x34 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0xA9
    //
    // Device info frame (300 bytes):
    //   Vendor ID: JK-B2A16S
    //   Hardware version: 3.0
    //   Software version: 3.3.0
    //   Uptime: 36867600 s
    //   Power on count: 19
    //   Device name: BMS
    //   Device passcode: 1234
    //   Manufacturing date:
    //   Serial number:
    //   Passcode:
    //   User data:
    //   Setup passcode:

    // JK02 response example:
    //
    // 0x55 0xAA 0xEB 0x90 0x03 0x9F 0x4A 0x4B 0x2D 0x42 0x32 0x41 0x32 0x34 0x53 0x31 0x35 0x50 0x00 0x00 0x00 0x00 0x31
    // 0x30 0x2E 0x58 0x57 0x00 0x00 0x00 0x31 0x30 0x2E 0x30 0x37 0x00 0x00 0x00 0x40 0xAF 0x01 0x00 0x06 0x00 0x00 0x00
    // 0x4A 0x4B 0x2D 0x42 0x32 0x41 0x32 0x34 0x53 0x31 0x35 0x50 0x00 0x00 0x00 0x00 0x31 0x32 0x33 0x34 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x32 0x32 0x30 0x34 0x30 0x37 0x00 0x00 0x32 0x30 0x32 0x31 0x36 0x30
    // 0x32 0x30 0x39 0x36 0x00 0x30 0x30 0x30 0x30 0x00 0x49 0x6E 0x70 0x75 0x74 0x20 0x55 0x73 0x65 0x72 0x64 0x61 0x74
    // 0x61 0x00 0x00 0x31 0x32 0x33 0x34 0x35 0x36 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x65

    snprintf(deviceInfo.vendorId, sizeof(deviceInfo.vendorId),
             std::string(data.begin() + 6, data.begin() + 6 + 16).c_str());

    snprintf(deviceInfo.hardwareVersion, sizeof(deviceInfo.hardwareVersion),
             std::string(data.begin() + 22, data.begin() + 22 + 8).c_str());

    snprintf(deviceInfo.softwareVersion, sizeof(deviceInfo.softwareVersion),
             std::string(data.begin() + 30, data.begin() + 30 + 8).c_str());

    deviceInfo.uptime = get32(38);

    deviceInfo.powerOnCount = get32(42);

    snprintf(deviceInfo.deviceName, sizeof(deviceInfo.deviceName),
             std::string(data.begin() + 46, data.begin() + 46 + 16).c_str());

    snprintf(deviceInfo.devicePasscode, sizeof(deviceInfo.devicePasscode),
             std::string(data.begin() + 62, data.begin() + 62 + 16).c_str());

    snprintf(deviceInfo.manufacturingDate, sizeof(deviceInfo.manufacturingDate),
             std::string(data.begin() + 78, data.begin() + 78 + 8).c_str());

    snprintf(deviceInfo.serialNumber, sizeof(deviceInfo.serialNumber),
             std::string(data.begin() + 86, data.begin() + 86 + 11).c_str());

    snprintf(deviceInfo.passcode, sizeof(deviceInfo.passcode),
             std::string(data.begin() + 97, data.begin() + 97 + 5).c_str());

    snprintf(deviceInfo.userData, sizeof(deviceInfo.userData),
             std::string(data.begin() + 102, data.begin() + 102 + 16).c_str());

    snprintf(deviceInfo.setupPasscode, sizeof(deviceInfo.setupPasscode),
             std::string(data.begin() + 118, data.begin() + 118 + 16).c_str());
}

void PeerCharacteristicJkBms::decodeJk02Settings(const std::vector<uint8_t>& data) {
    auto get16 = [&](size_t i) -> uint16_t { return (uint16_t(data[i + 1]) << 8) | (uint16_t(data[i + 0]) << 0); };
    auto get32 = [&](size_t i) -> uint32_t {
        return (uint32_t(get16(i + 2)) << 16) | (uint32_t(get16(i + 0)) << 0);
    };

    // log_i("Settings frame (%d bytes) received", data.size());
    // log_d("  %s", format_hex_pretty(&data.front(), 160).c_str());
    // log_d("  %s", format_hex_pretty(&data.front() + 160, data.size() - 160).c_str());

    // JK02 response example:
    //
    // 0x55 0xAA 0xEB 0x90 0x01 0x4F 0x58 0x02 0x00 0x00 0x54 0x0B 0x00 0x00 0x80 0x0C 0x00 0x00 0xCC 0x10 0x00 0x00 0x68
    // 0x10 0x00 0x00 0x0A 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0xF0 0x0A 0x00 0x00 0xA8 0x61 0x00 0x00 0x1E 0x00 0x00 0x00 0x3C 0x00 0x00 0x00 0xF0 0x49 0x02 0x00 0x2C 0x01 0x00
    // 0x00 0x3C 0x00 0x00 0x00 0x3C 0x00 0x00 0x00 0xD0 0x07 0x00 0x00 0xBC 0x02 0x00 0x00 0x58 0x02 0x00 0x00 0xBC 0x02
    // 0x00 0x00 0x58 0x02 0x00 0x00 0x38 0xFF 0xFF 0xFF 0x9C 0xFF 0xFF 0xFF 0x84 0x03 0x00 0x00 0xBC 0x02 0x00 0x00 0x0D
    // 0x00 0x00 0x00 0x01 0x00 0x00 0x00 0x01 0x00 0x00 0x00 0x01 0x00 0x00 0x00 0x88 0x13 0x00 0x00 0xDC 0x05 0x00 0x00
    // 0xE4 0x0C 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x40

    // Byte Len  Payload                Content              Coeff.      Unit        Example value
    // 0     4   0x55 0xAA 0xEB 0x90    Header
    // 4     1   0x01                   Frame type
    // 5     1   0x4F                   Frame counter
    // 6     4   0x58 0x02 0x00 0x00    Unknown6
    // log_d("  Unknown6: %f", (float)get32(6) * 0.001f);
    // 10    4   0x54 0x0B 0x00 0x00    Cell UVP
    settings.cellUVP = (float)get32(10) * 0.001f;

    // 14    4   0x80 0x0C 0x00 0x00    Cell UVP Recovery
    settings.cellUVPR = (float)get32(14) * 0.001f;

    // 18    4   0xCC 0x10 0x00 0x00    Cell OVP
    settings.cellOVP = (float)get32(18) * 0.001f;

    // 22    4   0x68 0x10 0x00 0x00    Cell OVP Recovery
    settings.cellOVPR = (float)get32(22) * 0.001f;

    // 26    4   0x0A 0x00 0x00 0x00    Balance trigger voltage
    settings.balanceTriggerVoltage = (float)get32(26) * 0.001f;

    // 30    4   0x00 0x00 0x00 0x00    Unknown30
    // 34    4   0x00 0x00 0x00 0x00    Unknown34
    // 38    4   0x00 0x00 0x00 0x00    Unknown38
    // 42    4   0x00 0x00 0x00 0x00    Unknown42
    // 46    4   0xF0 0x0A 0x00 0x00    Power off voltage
    settings.powerOffVoltage = (float)get32(46) * 0.001f;

    // 50    4   0xA8 0x61 0x00 0x00    Max. charge current
    settings.maxChargeCurrent = (float)get32(50) * 0.001f;

    // 54    4   0x1E 0x00 0x00 0x00    Charge OCP delay
    settings.chargeOCPDelay = get32(54);
    // 58    4   0x3C 0x00 0x00 0x00    Charge OCP recovery delay
    settings.chargeOCPRDelay = get32(58);
    // 62    4   0xF0 0x49 0x02 0x00    Max. discharge current
    settings.maxDischargeCurrent = (float)get32(62) * 0.001f;

    // 66    4   0x2C 0x01 0x00 0x00    Discharge OCP delay
    settings.dischargeOCPDelay = get32(66);
    // 70    4   0x3C 0x00 0x00 0x00    Discharge OCP recovery delay
    settings.dischargeOCPRDelay = get32(70);
    // 74    4   0x3C 0x00 0x00 0x00    SCPR time
    settings.scpRecoveryTime = get32(74);
    // 78    4   0xD0 0x07 0x00 0x00    Max balance current
    settings.maxBalanceCurrent = (float)get32(78) * 0.001f;

    // 82    4   0xBC 0x02 0x00 0x00    Charge OTP
    settings.chargeOTP = (float)get32(82) * 0.1f;
    // 86    4   0x58 0x02 0x00 0x00    Charge OTP Recovery
    settings.chargeOTPR = (float)get32(86) * 0.1f;
    // 90    4   0xBC 0x02 0x00 0x00    Discharge OTP
    settings.dischargeOTP = (float)get32(90) * 0.1f;
    // 94    4   0x58 0x02 0x00 0x00    Discharge OTP Recovery
    settings.dischargeOTPR = (float)get32(94) * 0.1f;
    // 98    4   0x38 0xFF 0xFF 0xFF    Charge UTP
    settings.chargeUTP = (float)get32(98) * 0.1f;
    // 102   4   0x9C 0xFF 0xFF 0xFF    Charge UTP Recovery
    settings.chargeUTPR = (float)get32(102) * 0.1f;
    // 106   4   0x84 0x03 0x00 0x00    MOS OTP
    settings.mosOTP = (float)get32(106) * 0.1f;
    // 110   4   0xBC 0x02 0x00 0x00    MOS OTP Recovery
    settings.mosOTPR = (float)get32(110) * 0.1f;
    // 114   4   0x0D 0x00 0x00 0x00    Cell count
    settings.cellCount = get32(114);

    // 118   4   0x01 0x00 0x00 0x00    Charge switch
    settings.chargingSwitch = (bool)get32(118);

    // 122   4   0x01 0x00 0x00 0x00    Discharge switch
    settings.dischargingSwitch = (bool)get32(122);

    // 126   4   0x01 0x00 0x00 0x00    Balancer switch
    settings.balancerSwitch = (bool)get32(126);

    // 130   4   0x88 0x13 0x00 0x00    Nominal battery capacity
    settings.nominalCapacity = (float)get32(130) * 0.001f;

    // 134   4   0xDC 0x05 0x00 0x00    Unknown134
    // log_d("  Unknown134: %f", (float)get32(134) * 0.001f);
    // 138   4   0xE4 0x0C 0x00 0x00    Start balance voltage
    settings.balanceStartingVoltage = (float)get32(138) * 0.001f;

    // 142   4   0x00 0x00 0x00 0x00
    // 146   4   0x00 0x00 0x00 0x00
    // 150   4   0x00 0x00 0x00 0x00
    // 154   4   0x00 0x00 0x00 0x00
    // 158   4   0x00 0x00 0x00 0x00    Con. wire resistance 1
    // 162   4   0x00 0x00 0x00 0x00    Con. wire resistance 2
    // 166   4   0x00 0x00 0x00 0x00    Con. wire resistance 3
    // 170   4   0x00 0x00 0x00 0x00    Con. wire resistance 4
    // 174   4   0x00 0x00 0x00 0x00    Con. wire resistance 5
    // 178   4   0x00 0x00 0x00 0x00    Con. wire resistance 6
    // 182   4   0x00 0x00 0x00 0x00    Con. wire resistance 7
    // 186   4   0x00 0x00 0x00 0x00    Con. wire resistance 8
    // 190   4   0x00 0x00 0x00 0x00    Con. wire resistance 9
    // 194   4   0x00 0x00 0x00 0x00    Con. wire resistance 10
    // 198   4   0x00 0x00 0x00 0x00    Con. wire resistance 11
    // 202   4   0x00 0x00 0x00 0x00    Con. wire resistance 12
    // 206   4   0x00 0x00 0x00 0x00    Con. wire resistance 13
    // 210   4   0x00 0x00 0x00 0x00    Con. wire resistance 14
    // 214   4   0x00 0x00 0x00 0x00    Con. wire resistance 15
    // 218   4   0x00 0x00 0x00 0x00    Con. wire resistance 16
    // 222   4   0x00 0x00 0x00 0x00    Con. wire resistance 17
    // 226   4   0x00 0x00 0x00 0x00    Con. wire resistance 18
    // 230   4   0x00 0x00 0x00 0x00    Con. wire resistance 19
    // 234   4   0x00 0x00 0x00 0x00    Con. wire resistance 20
    // 238   4   0x00 0x00 0x00 0x00    Con. wire resistance 21
    // 242   4   0x00 0x00 0x00 0x00    Con. wire resistance 22
    // 246   4   0x00 0x00 0x00 0x00    Con. wire resistance 23
    // 250   4   0x00 0x00 0x00 0x00    Con. wire resistance 24
    for (uint8_t i = 0; i < 24; i++) {
        settings.conWireResistance[i] = (float)get32(i * 4 + 158) * 0.001f;
    }

    // 254   4   0x00 0x00 0x00 0x00
    // 258   4   0x00 0x00 0x00 0x00
    // 262   4   0x00 0x00 0x00 0x00
    // 266   4   0x00 0x00 0x00 0x00
    // 270   4   0x00 0x00 0x00 0x00
    // 274   4   0x00 0x00 0x00 0x00
    // 278   4   0x00 0x00 0x00 0x00
    // 282   1   0x00                   New controls bitmask
    settings.disableTemperatureSensors = (bool)((data[282] & 2) == 2);
    settings.displayAlwaysOn = (bool)((data[282] & 16) == 16);

    // 283   3   0x00 0x00 0x00
    // 286   4   0x00 0x00 0x00 0x00
    // 290   4   0x00 0x00 0x00 0x00
    // 294   4   0x00 0x00 0x00 0x00
    // 298   1   0x00
    // 299   1   0x40                   CRC
}

void PeerCharacteristicJkBms::decodeJk04Settings(const std::vector<uint8_t>& data) {
    log_e("not impl");
    /*
    auto get16 = [&](size_t i) -> uint16_t { return (uint16_t(data[i + 1]) << 8) | (uint16_t(data[i + 0]) << 0); };
    auto get32 = [&](size_t i) -> uint32_t {
        return (uint32_t(get16(i + 2)) << 16) | (uint32_t(get16(i + 0)) << 0);
    };

    log_i("Settings frame (%d bytes) received", data.size());
    log_d("  %s", format_hex_pretty(&data.front(), 160).c_str());
    log_d("  %s", format_hex_pretty(&data.front() + 160, data.size() - 160).c_str());

    // JK04 (JK-B2A16S v3) response example:
    //
    // 0x55 0xAA 0xEB 0x90 0x01 0x50 0x00 0x00 0x80 0x3F 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x10 0x00 0x00 0x00 0x00 0x00 0x40 0x40 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0xA3 0xFD 0x40 0x40 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x88 0x40 0x9A 0x99 0x59 0x40 0x0A 0xD7 0xA3 0x3B 0x00 0x00 0x00 0x40 0x01
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0xCE

    // Byte Len  Payload                Content              Coeff.      Unit        Example value
    // 0     4   0x55 0xAA 0xEB 0x90    Header
    // 4     1   0x01                   Frame type
    // 5     1   0x50                   Frame counter
    // 6     4   0x00 0x00 0x80 0x3F
    log_d("  Unknown6: 0x%02X 0x%02X 0x%02X 0x%02X (%f)", data[6], data[7], data[8], data[9],
          (float)ieee_float_(get32(6)));

    // 10    4   0x00 0x00 0x00 0x00
    // 14    4   0x00 0x00 0x00 0x00
    // 18    4   0x00 0x00 0x00 0x00
    // 22    4   0x00 0x00 0x00 0x00
    // 26    4   0x00 0x00 0x00 0x00
    // 30    4   0x00 0x00 0x00 0x00
    // 34    4   0x10 0x00 0x00 0x00    Cell count
    log_i("  Cell count: %d", data[34]);

    // 38    4   0x00 0x00 0x40 0x40    Power off voltage
    log_i("  Power off voltage: %f V", (float)ieee_float_(get32(38)));

    // 42    4   0x00 0x00 0x00 0x00
    // 46    4   0x00 0x00 0x00 0x00
    // 50    4   0x00 0x00 0x00 0x00
    // 54    4   0x00 0x00 0x00 0x00
    // 58    4   0x00 0x00 0x00 0x00
    // 62    4   0x00 0x00 0x00 0x00
    // 66    4   0x00 0x00 0x00 0x00
    // 70    4   0x00 0x00 0x00 0x00
    // 74    4   0xA3 0xFD 0x40 0x40
    log_d("  Unknown74: %f", (float)ieee_float_(get32(74)));

    // 78    4   0x00 0x00 0x00 0x00
    // 82    4   0x00 0x00 0x00 0x00
    // 86    4   0x00 0x00 0x00 0x00
    // 90    4   0x00 0x00 0x00 0x00
    // 94    4   0x00 0x00 0x00 0x00
    // 98    4   0x00 0x00 0x88 0x40    Start balance voltage
    log_i("  Start balance voltage: %f V", (float)ieee_float_(get32(98)));

    // 102   4   0x9A 0x99 0x59 0x40
    log_d("  Unknown102: %f", (float)ieee_float_(get32(102)));

    // 106   4   0x0A 0xD7 0xA3 0x3B    Trigger delta voltage
    log_i("  Trigger Delta Voltage: %f V", (float)ieee_float_(get32(106)));

    // 110   4   0x00 0x00 0x00 0x40    Max. balance current
    log_i("  Max. balance current: %f A", (float)ieee_float_(get32(110)));
    // 114   4   0x01 0x00 0x00 0x00    Balancer switch
    publish_state_(balancer_switch_, (bool)(data[114]));

    log_i("  ADC Vref: unknown V");  // 53.67 V?

    // 118  20   0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 138  20   0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 158  20   0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 178  20   0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 198  20   0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 218  20   0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 238  20   0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 258  20   0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 278  20   0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 298   2   0x00 0xCE
    */
}

void PeerCharacteristicJkBms::decodeJk02CellInfo(const std::vector<uint8_t>& data) {
    auto get16 = [&](size_t i) -> uint16_t { return (uint16_t(data[i + 1]) << 8) | (uint16_t(data[i + 0]) << 0); };

    auto get32 = [&](size_t i) -> uint32_t {
        return (uint32_t(get16(i + 2)) << 16) | (uint32_t(get16(i + 0)) << 0);
    };

    const uint32_t now = millis();
    if (now - cellInfo.lastUpdate < cellInfoMinDelay) {
        // log_d("what's the rush?");
        return;
    }
    cellInfo.lastUpdate = now;

    uint8_t offset = 0;
    uint8_t frameVersion = FRAME_VERSION_JK02;
    if (protocolVersion == PROTOCOL_VERSION_JK02) {
        // Weak assumption: The value of data[189] (JK02) or data[189+32] (JK02_32S) is 0x01, 0x02 or 0x03
        if (data[189] == 0x00 && data[189 + 32] > 0) {
            frameVersion = FRAME_VERSION_JK02_32S;
            offset = 16;
            // log_w("You hit the unstable auto detection of the protocol version. This feature will be removed in future! Please update your configuration to protocol version JK02_32S if you are using a JK-B2A8S20P v11+");
        }
    }

    // Override unstable auto detection
    if (protocolVersion == PROTOCOL_VERSION_JK02_32S) {
        frameVersion = FRAME_VERSION_JK02_32S;
        offset = 16;
    }

    // log_i("Cell info frame (version %d, %d bytes) received", frameVersion, data.size());
    //  printHex(&data.front(), 150);
    //  printHex(&data.front() + 150, data.size() - 150);

    // 6 example responses (128+128+44 = 300 bytes per frame)
    //
    //
    // 55.AA.EB.90.02.8C.FF.0C.01.0D.01.0D.FF.0C.01.0D.01.0D.FF.0C.01.0D.01.0D.01.0D.01.0D.FF.0C.01.0D.01.0D.01.0D.01.0D.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.FF.FF.00.00.00.0D.00.00.00.00.9D.01.96.01.8C.01.87.01.84.01.84.01.83.01.84.01.85.01.81.01.83.01.86.01.82.01.82.01.83.01.85.01.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.03.D0.00.00.00.00.00.00.00.00
    // 00.00.BE.00.BF.00.D2.00.00.00.00.00.00.54.8E.0B.01.00.68.3C.01.00.00.00.00.00.3D.04.00.00.64.00.79.04.CA.03.10.00.01.01.AA.06.00.00.00.00.00.00.00.00.00.00.00.00.07.00.01.00.00.00.D5.02.00.00.00.00.AE.D6.3B.40.00.00.00.00.58.AA.FD.FF.00.00.00.01.00.02.00.00.EC.E6.4F.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00
    // 00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.CD
    //
    // 55.AA.EB.90.02.8D.FF.0C.01.0D.01.0D.01.0D.01.0D.01.0D.FF.0C.01.0D.01.0D.01.0D.FF.0C.FF.0C.01.0D.01.0D.01.0D.01.0D.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.FF.FF.00.00.00.0D.00.00.00.00.9D.01.96.01.8C.01.87.01.84.01.84.01.83.01.84.01.85.01.81.01.83.01.86.01.82.01.82.01.83.01.85.01.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.04.D0.00.00.00.00.00.00.00.00
    // 00.00.BE.00.BF.00.D2.00.00.00.00.00.00.54.8E.0B.01.00.68.3C.01.00.00.00.00.00.3D.04.00.00.64.00.79.04.CA.03.10.00.01.01.AA.06.00.00.00.00.00.00.00.00.00.00.00.00.07.00.01.00.00.00.D5.02.00.00.00.00.AE.D6.3B.40.00.00.00.00.58.AA.FD.FF.00.00.00.01.00.02.00.00.F0.E6.4F.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00
    // 00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.D3
    //
    // 55.AA.EB.90.02.8E.FF.0C.01.0D.01.0D.FF.0C.01.0D.01.0D.FF.0C.01.0D.01.0D.01.0D.FF.0C.FF.0C.01.0D.01.0D.01.0D.01.0D.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.FF.FF.00.00.00.0D.00.00.00.00.9D.01.96.01.8C.01.87.01.84.01.84.01.83.01.84.01.85.01.81.01.83.01.86.01.82.01.82.01.83.01.85.01.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.04.D0.00.00.00.00.00.00.00.00
    // 00.00.BE.00.BF.00.D2.00.00.00.00.00.00.54.8E.0B.01.00.68.3C.01.00.00.00.00.00.3D.04.00.00.64.00.79.04.CA.03.10.00.01.01.AA.06.00.00.00.00.00.00.00.00.00.00.00.00.07.00.01.00.00.00.D5.02.00.00.00.00.AE.D6.3B.40.00.00.00.00.58.AA.FD.FF.00.00.00.01.00.02.00.00.F5.E6.4F.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00
    // 00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.D6
    //
    // 55.AA.EB.90.02.91.FF.0C.FF.0C.01.0D.FF.0C.01.0D.01.0D.FF.0C.01.0D.01.0D.01.0D.01.0D.FF.0C.01.0D.01.0D.01.0D.01.0D.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.FF.FF.00.00.00.0D.00.00.00.00.9D.01.96.01.8C.01.87.01.84.01.84.01.83.01.84.01.85.01.81.01.83.01.86.01.82.01.82.01.83.01.85.01.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.01.D0.00.00.00.00.00.00.00.00
    // 00.00.BF.00.C0.00.D2.00.00.00.00.00.00.54.8E.0B.01.00.68.3C.01.00.00.00.00.00.3D.04.00.00.64.00.79.04.CC.03.10.00.01.01.AA.06.00.00.00.00.00.00.00.00.00.00.00.00.07.00.01.00.00.00.D5.02.00.00.00.00.AE.D6.3B.40.00.00.00.00.58.AA.FD.FF.00.00.00.01.00.02.00.00.01.E7.4F.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00
    // 00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.E7
    //
    // 55.AA.EB.90.02.92.01.0D.01.0D.01.0D.01.0D.01.0D.01.0D.FF.0C.01.0D.01.0D.01.0D.01.0D.FF.0C.01.0D.01.0D.01.0D.01.0D.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.FF.FF.00.00.00.0D.00.00.00.00.9D.01.96.01.8C.01.87.01.84.01.84.01.83.01.84.01.85.01.81.01.83.01.86.01.82.01.82.01.83.01.85.01.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.03.D0.00.00.00.00.00.00.00.00
    // 00.00.BF.00.C0.00.D2.00.00.00.00.00.00.54.8E.0B.01.00.68.3C.01.00.00.00.00.00.3D.04.00.00.64.00.79.04.CC.03.10.00.01.01.AA.06.00.00.00.00.00.00.00.00.00.00.00.00.07.00.01.00.00.00.D5.02.00.00.00.00.AE.D6.3B.40.00.00.00.00.58.AA.FD.FF.00.00.00.01.00.02.00.00.06.E7.4F.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00
    // 00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.F8
    //
    // 55.AA.EB.90.02.93.FF.0C.01.0D.01.0D.01.0D.01.0D.01.0D.FF.0C.01.0D.01.0D.01.0D.FF.0C.FF.0C.01.0D.01.0D.01.0D.01.0D.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.FF.FF.00.00.00.0D.00.00.00.00.9D.01.96.01.8C.01.87.01.84.01.84.01.83.01.84.01.85.01.81.01.83.01.86.01.82.01.82.01.83.01.85.01.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.04.D0.00.00.00.00.00.00.00.00
    // 00.00.BE.00.C0.00.D2.00.00.00.00.00.00.54.8E.0B.01.00.68.3C.01.00.00.00.00.00.3D.04.00.00.64.00.79.04.CD.03.10.00.01.01.AA.06.00.00.00.00.00.00.00.00.00.00.00.00.07.00.01.00.00.00.D5.02.00.00.00.00.AE.D6.3B.40.00.00.00.00.58.AA.FD.FF.00.00.00.01.00.02.00.00.0A.E7.4F.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00
    // 00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.F8
    //
    // Byte Len  Payload                Content              Coeff.      Unit        Example value
    // 0     2   0x55 0xAA 0xEB 0x90    Header
    // 4     1   0x02                   Record type
    // 5     1   0x8C                   Frame counter
    // 6     2   0xFF 0x0C              Voltage cell 01       0.001        V
    // 8     2   0x01 0x0D              Voltage cell 02       0.001        V
    // 10    2   0x01 0x0D              Voltage cell 03       0.001        V
    // ...
    uint8_t cells = 24 + (offset / 2);
    float cellVoltageMin = 100.0f;
    float cellVoltageMax = -100.0f;
    for (uint8_t i = 0; i < cells; i++) {
        float cellVoltage = (float)get16(i * 2 + 6) * 0.001f;
        float cell_resistance = (float)get16(i * 2 + 64 + offset) * 0.001f;
        if (cellVoltage > 0 && cellVoltage < cellVoltageMin) {
            cellVoltageMin = cellVoltage;
        }
        if (cellVoltage > cellVoltageMax) {
            cellVoltageMax = cellVoltage;
        }
        cellInfo.cells[i].voltage = cellVoltage;
        cellInfo.cells[i].resistance = cell_resistance;
    }
    cellInfo.cellVoltageMin = cellVoltageMin;
    cellInfo.cellVoltageMax = cellVoltageMax;

    // 54    4   0xFF 0xFF 0x00 0x00    Enabled cells bitmask
    //           0x0F 0x00 0x00 0x00    4 cells enabled
    //           0xFF 0x00 0x00 0x00    8 cells enabled
    //           0xFF 0x0F 0x00 0x00    12 cells enabled
    //           0xFF 0x1F 0x00 0x00    13 cells enabled
    //           0xFF 0xFF 0x00 0x00    16 cells enabled
    //           0xFF 0xFF 0xFF 0x00    24 cells enabled
    //           0xFF 0xFF 0xFF 0xFF    32 cells enabled
    // log_d("Enabled cells bitmask: 0x%02X 0x%02X 0x%02X 0x%02X", data[54 + offset], data[55 + offset], data[56 + offset], data[57 + offset]);

    // 58    2   0x00 0x0D              Average Cell Voltage  0.001        V
    cellInfo.cellVoltageAvg = (float)get16(58 + offset) * 0.001f;
    // log_d("cellVoltageAvg: %.3f V", cellVoltageAvg);

    // 60    2   0x00 0x00              Delta Cell Voltage    0.001        V
    cellInfo.cellVoltageDelta = (float)get16(60 + offset) * 0.001f;
    // log_d("cellVoltageDelta: %.3f V", cellVoltageDelta);

    // 62    1   0x00                   Max voltage cell      1
    cellInfo.cellVoltageMaxId = data[62 + offset] + 1;

    // 63    1   0x00                   Min voltage cell      1
    cellInfo.cellVoltageMinId = data[63 + offset] + 1;

    // 64    2   0x9D 0x01              Resistance Cell 01    0.001        Ohm
    // 66    2   0x96 0x01              Resistance Cell 02    0.001        Ohm
    // 68    2   0x8C 0x01              Resistance Cell 03    0.001        Ohm
    // ...
    // 110   2   0x00 0x00              Resistance Cell 24    0.001        Ohm

    offset = offset * 2;

    // 112   2   0x00 0x00              Unknown112
    if (frameVersion == FRAME_VERSION_JK02_32S) {
        cellInfo.temp0 = (float)((int16_t)get16(112 + offset)) * 0.1f;
        // log_d("temp0: %.1f C", temp0);

    } else {
        // log_d("Unknown112: 0x%02X 0x%02X", data[112 + offset], data[113 + offset]);
    }

    // 114   4   0x00 0x00 0x00 0x00    Wire resistance warning bitmask (each bit indicates a warning per cell / wire)
    // log_d("Wire resistance warning bitmask: 0x%02X 0x%02X 0x%02X 0x%02X", data[114 + offset], data[115 + offset], data[116 + offset], data[117 + offset]);

    // 118   4   0x03 0xD0 0x00 0x00    Battery voltage       0.001        V
    cellInfo.voltage = (float)get32(118 + offset) * 0.001f;
    // log_d("voltage: %.2f V", voltage);

    // 122   4   0x00 0x00 0x00 0x00    Battery power         0.001        W
    cellInfo.power = (float)((int32_t)get32(122 + offset)) * 0.001f;
    // log_d("power: %.2f W", power);

    // 126   4   0x00 0x00 0x00 0x00    Charge current        0.001        A
    cellInfo.chargeCurrent = (float)((int32_t)get32(126 + offset)) * 0.001f;
    // log_d("chargeCurrent: %.2f A", chargeCurrent);
    if (cellInfo.chargeCurrent < 0.0f && 0.0f < cellInfo.power) cellInfo.power *= -1;
    // 130   2   0xBE 0x00              Temperature Sensor 1  0.1          °C
    cellInfo.temp1 = (float)((int16_t)get16(130 + offset)) * 0.1f;
    // log_d("temp1: %.1f C", temp1);

    // 132   2   0xBF 0x00              Temperature Sensor 2  0.1          °C
    cellInfo.temp2 = (float)((int16_t)get16(132 + offset)) * 0.1f;
    // log_d("temp2: %.1f C", temp2);

    // 134   2   0xD2 0x00              MOS Temperature       0.1          °C
    if (frameVersion == FRAME_VERSION_JK02_32S) {
        /*
        uint16_t raw_errors_bitmask = (uint16_t(data[134 + offset]) << 8) | (uint16_t(data[134 + 1 + offset]) << 0);
        publish_state_(errors_bitmask_sensor_, (float)raw_errors_bitmask);
        publish_state_(errors_text_sensor_, errorToString(raw_errors_bitmask));
        */
    } else {
        cellInfo.temp0 = (float)((int16_t)get16(134 + offset)) * 0.1f;
        // log_d("temp0: %.1f C", temp0);
    }

    // 136   2   0x00 0x00              System alarms
    //           0x00 0x01                Charge overtemperature               0000 0000 0000 0001
    //           0x00 0x02                Charge undertemperature              0000 0000 0000 0010
    //           0x00 0x04                                                     0000 0000 0000 0100
    //           0x00 0x08                Cell Undervoltage                    0000 0000 0000 1000
    //           0x00 0x10                                                     0000 0000 0001 0000
    //           0x00 0x20                                                     0000 0000 0010 0000
    //           0x00 0x40                                                     0000 0000 0100 0000
    //           0x00 0x80                                                     0000 0000 1000 0000
    //           0x01 0x00                                                     0000 0001 0000 0000
    //           0x02 0x00                                                     0000 0010 0000 0000
    //           0x04 0x00                Cell count is not equal to settings  0000 0100 0000 0000
    //           0x08 0x00                Current sensor anomaly               0000 1000 0000 0000
    //           0x10 0x00                Cell Over Voltage                    0001 0000 0000 0000
    //           0x20 0x00                                                     0010 0000 0000 0000
    //           0x40 0x00                                                     0100 0000 0000 0000
    //           0x80 0x00                                                     1000 0000 0000 0000
    //
    //           0x14 0x00                Cell Over Voltage +                  0001 0100 0000 0000
    //                                    Cell count is not equal to settings
    //           0x04 0x08                Cell Undervoltage +                  0000 0100 0000 1000
    //                                    Cell count is not equal to settings
    if (frameVersion != FRAME_VERSION_JK02_32S) {
        uint16_t raw_errors_bitmask = (uint16_t(data[136 + offset]) << 8) |
                                      (uint16_t(data[136 + 1 + offset]) << 0);
        strncpy(cellInfo.errors, errorToString(raw_errors_bitmask).c_str(), sizeof(cellInfo.errors));
        // log_d("errors: %s", errors);
    }

    // 138   2   0x00 0x00              Balance current      0.001         A
    float bc = (float)((int16_t)get16(138 + offset)) * 0.001f;
    // 140   1   0x00                   Balancing action                   0x00: Off
    //                                                                     0x01: Charging balancer
    //                                                                     0x02: Discharging balancer
    if (0x02 == data[140 + offset]) bc *= -1;
    cellInfo.balanceCurrent = bc;
    // log_d("balanceCurrent: %.3f A", balanceCurrent);

    // 141   1   0x54                   State of charge in   1.0           %
    cellInfo.soc = data[141 + offset];
    // log_d("soc: %d%", soc);

    // 142   4   0x8E 0x0B 0x01 0x00    Capacity_Remain      0.001         Ah
    cellInfo.capacityRemaining = (float)get32(142 + offset) * 0.001f;

    // 146   4   0x68 0x3C 0x01 0x00    Nominal_Capacity     0.001         Ah
    cellInfo.capacityNominal = (float)get32(146 + offset) * 0.001f;
    // log_d("capacity: %.3f Ah / %.3f Ah", capacityRemaining, capacityNominal);

    // 150   4   0x00 0x00 0x00 0x00    Cycle_Count          1.0
    cellInfo.cycleCount = get32(150 + offset);

    // 154   4   0x3D 0x04 0x00 0x00    Cycle_Capacity       0.001         Ah
    cellInfo.capacityCycle = (float)get32(154 + offset) * 0.001f;
    // log_d("cycles: %d (%.3f Ah)", cycleCount, capacityCycle);

    // 158   2   0x64 0x00              Unknown158
    // log_d("Unknown158: 0x%02X 0x%02X (always 0x64 0x00?)", data[158 + offset], data[159 + offset]);

    // 160   2   0x79 0x04              Unknown160 (Cycle capacity?)
    // log_d("Unknown160: 0x%02X 0x%02X (always 0xC5 0x09?)", data[160 + offset], data[161 + offset]);

    // 162   4   0xCA 0x03 0x10 0x00    Total runtime in seconds           s
    cellInfo.totalRuntime = get32(162 + offset);

    // 166   1   0x01                   Charging mosfet enabled                      0x00: off, 0x01: on
    cellInfo.chargingEnabled = (bool)data[166 + offset];

    // 167   1   0x01                   Discharging mosfet enabled                   0x00: off, 0x01: on
    cellInfo.dischargingEnabled = (bool)data[167 + offset];
    // log_d("%s   %s", chargingEnabled ? "charging" : "", dischargingEnabled ? "discharging" : "");

    // log_d("Unknown168: %s",   format_hex_pretty(&data.front() + 168 + offset, data.size() - (168 + offset) - 4 - 81 - 1).c_str());

    // 168   1   0xAA                   Unknown168
    // 169   2   0x06 0x00              Unknown169
    // 171   2   0x00 0x00              Unknown171
    // 173   2   0x00 0x00              Unknown173
    // 175   2   0x00 0x00              Unknown175
    // 177   2   0x00 0x00              Unknown177
    // 179   2   0x00 0x00              Unknown179
    // 181   2   0x00 0x07              Unknown181
    // 183   2   0x00 0x01              Unknown183
    // 185   2   0x00 0x00              Unknown185
    // 187   2   0x00 0xD5              Unknown187
    // 189   2   0x02 0x00              Unknown189
    // log_d("Unknown189: 0x%02X 0x%02X", data[189], data[190]);
    // 190   1   0x00                   Unknown190
    // 191   1   0x00                   Balancer status (working: 0x01, idle: 0x00)
    // 192   1   0x00                   Unknown192
    // log_d("Unknown192: 0x%02X", data[192 + offset]);
    // 193   2   0x00 0xAE              Unknown193
    // log_d("Unknown193: 0x%02X 0x%02X (0x00 0x8D)", data[193 + offset], data[194 + offset]);
    // 195   2   0xD6 0x3B              Unknown195
    // log_d("Unknown195: 0x%02X 0x%02X (0x21 0x40)", data[195 + offset], data[196 + offset]);
    // 197   10  0x40 0x00 0x00 0x00 0x00 0x58 0xAA 0xFD 0xFF 0x00
    // 207   7   0x00 0x00 0x01 0x00 0x02 0x00 0x00
    // 214   4   0xEC 0xE6 0x4F 0x00    Uptime 100ms
    //
    // 218   81  0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    //           0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    //           0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    //           0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    //           0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    //           0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    //           0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    //           0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    //           0x00
    // 299   1   0xCD                   CRC
}

void PeerCharacteristicJkBms::decodeJk04CellInfo(const std::vector<uint8_t>& data) {
    log_e("not impl");
    /*
    auto get16 = [&](size_t i) -> uint16_t { return (uint16_t(data[i + 1]) << 8) | (uint16_t(data[i + 0]) << 0); };
    auto get32 = [&](size_t i) -> uint32_t {
        return (uint32_t(get16(i + 2)) << 16) | (uint32_t(get16(i + 0)) << 0);
    };

    const uint32_t now = millis();
    if (now - lastCellInfo < cellInfoMinDelay) {
        return;
    }
    lastCellInfo = now;

    log_i("Cell info frame (%d bytes) received", data.size());
    log_d("  %s", format_hex_pretty(&data.front(), 150).c_str());
    log_d("  %s", format_hex_pretty(&data.front() + 150, data.size() - 150).c_str());

    // 0x55 0xAA 0xEB 0x90 0x02 0x4B 0xC0 0x61 0x56 0x40 0x1F 0xAA 0x56 0x40 0xFF 0x91 0x56 0x40 0xFF 0x91 0x56 0x40 0x1F
    // 0xAA 0x56 0x40 0xFF 0x91 0x56 0x40 0xFF 0x91 0x56 0x40 0xFF 0x91 0x56 0x40 0x1F 0xAA 0x56 0x40 0xFF 0x91 0x56 0x40
    // 0xFF 0x91 0x56 0x40 0xFF 0x91 0x56 0x40 0xFF 0x91 0x56 0x40 0x1F 0xAA 0x56 0x40 0xE0 0x79 0x56 0x40 0xE0 0x79 0x56
    // 0x40 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x7C 0x1D 0x23 0x3E 0x1B 0xEB 0x08 0x3E 0x56 0xCE 0x14 0x3E 0x4D
    // 0x9B 0x15 0x3E 0xE0 0xDB 0xCD 0x3D 0x72 0x33 0xCD 0x3D 0x94 0x88 0x01 0x3E 0x5E 0x1E 0xEA 0x3D 0xE5 0x17 0xCD 0x3D
    // 0xE3 0xBB 0xD7 0x3D 0xF5 0x44 0xD2 0x3D 0xBE 0x7C 0x01 0x3E 0x27 0xB6 0x00 0x3E 0xDA 0xB5 0xFC 0x3D 0x6B 0x51 0xF8
    // 0x3D 0xA2 0x93 0xF3 0x3D 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x03 0x95 0x56 0x40 0x00
    // 0xBE 0x90 0x3B 0x00 0x00 0x00 0x00 0xFF 0xFF 0x00 0x00 0x01 0x00 0x00 0x01 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x66 0xA0 0xD2 0x4A 0x40 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x01 0x01 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    // 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x53 0x96 0x1C 0x00 0x00 0x00 0x00 0x00 0x00 0x48 0x22 0x40 0x00
    // 0x13
    //
    // Byte Len  Payload                Content              Coeff.      Unit        Example value
    // 0     4   0x55 0xAA 0xEB 0x90    Header
    // 4     1   0x02                   Frame type
    // 5     1   0x4B                   Frame counter
    // 6     4   0xC0 0x61 0x56 0x40    Cell voltage 1                     V
    // 10    4   0x1F 0xAA 0x56 0x40    Cell voltage 2                     V
    // 14    4   0xFF 0x91 0x56 0x40    Cell voltage 3                     V
    // 18    4   0xFF 0x91 0x56 0x40    Cell voltage 4                     V
    // 22    4   0x1F 0xAA 0x56 0x40    Cell voltage 5                     V
    // 26    4   0xFF 0x91 0x56 0x40    Cell voltage 6                     V
    // 30    4   0xFF 0x91 0x56 0x40    Cell voltage 7                     V
    // 34    4   0xFF 0x91 0x56 0x40    Cell voltage 8                     V
    // 38    4   0x1F 0xAA 0x56 0x40    Cell voltage 9                     V
    // 42    4   0xFF 0x91 0x56 0x40    Cell voltage 10                    V
    // 46    4   0xFF 0x91 0x56 0x40    Cell voltage 11                    V
    // 50    4   0xFF 0x91 0x56 0x40    Cell voltage 12                    V
    // 54    4   0xFF 0x91 0x56 0x40    Cell voltage 13                    V
    // 58    4   0x1F 0xAA 0x56 0x40    Cell voltage 14                    V
    // 62    4   0xE0 0x79 0x56 0x40    Cell voltage 15                    V
    // 66    4   0xE0 0x79 0x56 0x40    Cell voltage 16                    V
    // 70    4   0x00 0x00 0x00 0x00    Cell voltage 17                    V
    // 74    4   0x00 0x00 0x00 0x00    Cell voltage 18                    V
    // 78    4   0x00 0x00 0x00 0x00    Cell voltage 19                    V
    // 82    4   0x00 0x00 0x00 0x00    Cell voltage 20                    V
    // 86    4   0x00 0x00 0x00 0x00    Cell voltage 21                    V
    // 90    4   0x00 0x00 0x00 0x00    Cell voltage 22                    V
    // 94    4   0x00 0x00 0x00 0x00    Cell voltage 23                    V
    // 98    4   0x00 0x00 0x00 0x00    Cell voltage 24                    V
    // 102   4   0x7C 0x1D 0x23 0x3E    Cell resistance 1                  Ohm
    // 106   4   0x1B 0xEB 0x08 0x3E    Cell resistance 2                  Ohm
    // 110   4   0x56 0xCE 0x14 0x3E    Cell resistance 3                  Ohm
    // 114   4   0x4D 0x9B 0x15 0x3E    Cell resistance 4                  Ohm
    // 118   4   0xE0 0xDB 0xCD 0x3D    Cell resistance 5                  Ohm
    // 122   4   0x72 0x33 0xCD 0x3D    Cell resistance 6                  Ohm
    // 126   4   0x94 0x88 0x01 0x3E    Cell resistance 7                  Ohm
    // 130   4   0x5E 0x1E 0xEA 0x3D    Cell resistance 8                  Ohm
    // 134   4   0xE5 0x17 0xCD 0x3D    Cell resistance 9                  Ohm
    // 138   4   0xE3 0xBB 0xD7 0x3D    Cell resistance 10                 Ohm
    // 142   4   0xF5 0x44 0xD2 0x3D    Cell resistance 11                 Ohm
    // 146   4   0xBE 0x7C 0x01 0x3E    Cell resistance 12                 Ohm
    // 150   4   0x27 0xB6 0x00 0x3E    Cell resistance 13                 Ohm
    // 154   4   0xDA 0xB5 0xFC 0x3D    Cell resistance 14                 Ohm
    // 158   4   0x6B 0x51 0xF8 0x3D    Cell resistance 15                 Ohm
    // 162   4   0xA2 0x93 0xF3 0x3D    Cell resistance 16                 Ohm
    // 166   4   0x00 0x00 0x00 0x00    Cell resistance 17                 Ohm
    // 170   4   0x00 0x00 0x00 0x00    Cell resistance 18                 Ohm
    // 174   4   0x00 0x00 0x00 0x00    Cell resistance 19                 Ohm
    // 178   4   0x00 0x00 0x00 0x00    Cell resistance 20                 Ohm
    // 182   4   0x00 0x00 0x00 0x00    Cell resistance 21                 Ohm
    // 186   4   0x00 0x00 0x00 0x00    Cell resistance 22                 Ohm
    // 190   4   0x00 0x00 0x00 0x00    Cell resistance 23                 Ohm
    // 194   4   0x00 0x00 0x00 0x00    Cell resistance 24                 Ohm
    // 198   4   0x00 0x00 0x00 0x00    Cell resistance 25                 Ohm
    //                                  https://github.com/jblance/mpp-solar/issues/98#issuecomment-823701486
    uint8_t cells = 24;
    float cellVoltageMin = 100.0f;
    float cellVoltageMax = -100.0f;
    float total_voltage = 0.0f;
    uint8_t min_voltage_cell = 0;
    uint8_t max_voltage_cell = 0;
    for (uint8_t i = 0; i < cells; i++) {
        float cellVoltage = (float)ieee_float_(get32(i * 4 + 6));
        float cell_resistance = (float)ieee_float_(get32(i * 4 + 102));
        total_voltage = total_voltage + cellVoltage;
        if (cellVoltage > 0 && cellVoltage < cellVoltageMin) {
            cellVoltageMin = cellVoltage;
            min_voltage_cell = i + 1;
        }
        if (cellVoltage > cellVoltageMax) {
            cellVoltageMax = cellVoltage;
            max_voltage_cell = i + 1;
        }
        publish_state_(cells_[i].cellVoltage_sensor_, cellVoltage);
        publish_state_(cells_[i].cell_resistance_sensor_, cell_resistance);
    }

    publish_state_(cellVoltageMin_sensor_, cellVoltageMin);
    publish_state_(cellVoltageMax_sensor_, cellVoltageMax);
    publish_state_(max_voltage_cell_sensor_, (float)max_voltage_cell);
    publish_state_(min_voltage_cell_sensor_, (float)min_voltage_cell);
    publish_state_(total_voltage_sensor_, total_voltage);

    // 202   4   0x03 0x95 0x56 0x40    Average Cell Voltage               V
    publish_state_(average_cellVoltage_sensor_, (float)ieee_float_(get32(202)));

    // 206   4   0x00 0xBE 0x90 0x3B    Delta Cell Voltage                 V
    publish_state_(delta_cellVoltage_sensor_, (float)ieee_float_(get32(206)));

    // 210   4   0x00 0x00 0x00 0x00    Unknown210
    log_d("Unknown210: 0x%02X 0x%02X 0x%02X 0x%02X (always 0x00 0x00 0x00 0x00)", data[210], data[211], data[212],
          data[213]);

    // 214   4   0xFF 0xFF 0x00 0x00    Unknown214
    log_d("Unknown214: 0x%02X 0x%02X 0x%02X 0x%02X (0xFF 0xFF: 24 cells?)", data[214], data[215], data[216],
          data[217]);

    // 218   1   0x01                   Unknown218
    log_d("Unknown218: 0x%02X", data[218]);

    // 219   1   0x00                   Unknown219
    log_d("Unknown219: 0x%02X", data[219]);

    // 220   1   0x00                  Blink cells (0x00: Off, 0x01: Charging balancer, 0x02: Discharging balancer)
    bool balancing = (data[220] != 0x00);
    publish_state_(balancing_binary_sensor_, balancing);
    publish_state_(operation_status_text_sensor_, (balancing) ? "Balancing" : "Idle");

    // 221   1   0x01                  Unknown221
    log_d("Unknown221: 0x%02X", data[221]);

    // 222   4   0x00 0x00 0x00 0x00    Balancing current
    publish_state_(balancing_current_sensor_, (float)ieee_float_(get32(222)));

    // 226   7   0x00 0x00 0x00 0x00 0x00 0x00 0x00    Unknown226
    log_d("Unknown226: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X (always 0x00...0x00?)", data[226],
          data[227], data[228], data[229], data[230], data[231], data[232]);

    // 233   4   0x66 0xA0 0xD2 0x4A    Unknown233
    log_d("Unknown233: 0x%02X 0x%02X 0x%02X 0x%02X (%f)", data[233], data[234], data[235], data[236],
          ieee_float_(get32(233)));

    // 237   4   0x40 0x00 0x00 0x00    Unknown237
    log_d("Unknown237: 0x%02X 0x%02X 0x%02X 0x%02X (always 0x40 0x00 0x00 0x00?)", data[237], data[238],
          data[239], data[240]);

    // 241   45  0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    //           0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    //           0x00 0x00 0x00 0x01 0x01 0x00 0x00 0x00
    //           0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    //           0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    //           0x00 0x00 0x00 0x00 0x00
    // 286   3   0x53 0x96 0x1C 0x00        Uptime
    publish_state_(total_runtime_sensor_, (float)get32(286));
    publish_state_(total_runtime_formatted_text_sensor_, format_total_runtime_(get32(286)));

    // 290   4   0x00 0x00 0x00 0x00    Unknown290
    log_d("Unknown290: 0x%02X 0x%02X 0x%02X 0x%02X (always 0x00 0x00 0x00 0x00?)", data[290], data[291],
          data[292], data[293]);

    // 294   4   0x00 0x48 0x22 0x40    Unknown294
    log_d("Unknown294: 0x%02X 0x%02X 0x%02X 0x%02X", data[294], data[295], data[296], data[297]);

    // 298   1   0x00                   Unknown298
    log_d("Unknown298: 0x%02X", data[298]);

    // 299   1   0x13                   Checksm

    status_notification_received_ = true;
    */
}

void PeerCharacteristicJkBms::printDeviceInfo() {
    log_i("");
    log_s("  Vendor ID:          %s", deviceInfo.vendorId);
    log_s("  Hardware version:   %s", deviceInfo.hardwareVersion);
    log_s("  Software version:   %s", deviceInfo.softwareVersion);
    log_s("  Uptime:             %ds", deviceInfo.uptime);
    log_s("  Power on count:     %d", deviceInfo.powerOnCount);
    log_s("  Device name:        %s", deviceInfo.deviceName);
    log_s("  Device passcode:    %s", deviceInfo.devicePasscode);
    log_s("  Manufacturing date: %s", deviceInfo.manufacturingDate);
    log_s("  Serial number:      %s", deviceInfo.serialNumber);
    log_s("  Passcode:           %s", deviceInfo.passcode);
    log_s("  User data:          %s", deviceInfo.userData);
    log_s("  Setup passcode:     %s", deviceInfo.setupPasscode);
}

void PeerCharacteristicJkBms::printSettings() {
    log_i("");
    log_s("  Cell UVP:        %10.3fV   UVP recovery:      %10.3fV",
          settings.cellUVP, settings.cellUVPR);
    log_s("  Cell OVP:        %10.3fV   OVP recovery:      %10.3fV",
          settings.cellOVP, settings.cellOVPR);
    log_s("  Power off:       %10.3fV",
          settings.powerOffVoltage);
    log_s("  Balance above:   %10.3fV   Trigger delta:     %10.3fV",
          settings.balanceStartingVoltage, settings.balanceTriggerVoltage);
    log_s("  Balance current: %10.1fA",
          settings.maxBalanceCurrent);
    log_s("  Max charge:      %10.0fA   Max discharge:     %10.0fA",
          settings.maxChargeCurrent, settings.maxDischargeCurrent);
    log_s("  Charge OCP:      %10ds   COCP recovery:     %10ds",
          settings.chargeOCPDelay, settings.chargeOCPRDelay);
    log_s("  Discharge OCP:   %10ds   DOCP recovery:     %10ds",
          settings.dischargeOCPDelay, settings.dischargeOCPRDelay);
    log_s("  Short circuit protection recovery: %ds",
          settings.scpRecoveryTime);
    log_s("  Charge OTP:      %10.1fC   COTP recovery:     %10.1fC",
          settings.chargeOTP, settings.chargeOTPR);
    log_s("  Discharge OTP:   %10.1fC   DOTP recovery:     %10.1fC",
          settings.dischargeOTP, settings.dischargeOTPR);
    log_s("  Charge UTP:      %10.1fC   CUTP recovery:     %10.1fC",
          settings.chargeUTP, settings.chargeUTPR);
    log_s("  MOS OTP:         %10.1fC   MOTP recovery:     %10.1fC",
          settings.mosOTP, settings.mosOTPR);
    log_s("  Cell count:      %10d    Nominal capacity: %10.0fAh",
          settings.cellCount, settings.nominalCapacity);
    log_s("  Charging: %sabled    Disharging: %sabled    Balancer: %sabled",
          settings.chargingSwitch ? "en" : "dis",
          settings.dischargingSwitch ? "en" : "dis",
          settings.balancerSwitch ? "en" : "dis");

    // float conWireResistance[24]
    // bool disableTemperatureSensors
    // bool displayAlwaysOn
}

void PeerCharacteristicJkBms::printCellInfo() {
    char bal[] = "     ";
    if (0.0001f < cellInfo.balanceCurrent)
        snprintf(bal, sizeof(bal), "B%.1fA", cellInfo.balanceCurrent);

    log_s(
        "%d%%  "
        "%.2f"
        "/%.0fAh  "
        "%.3fV  "
        "%.1fA  "
        "%.0fW  "
        "C%d "
        "%.3fV "
        "C%d "
        "%.3fV  "
        "%s  "
        "%.1f "
        "%.1f "
        "%.1fC  "
        "%d:"
        "%.0fAh"
        "%s"
        "%s"
        "%s",
        cellInfo.soc,
        cellInfo.capacityRemaining,
        cellInfo.capacityNominal,
        cellInfo.voltage,
        cellInfo.chargeCurrent,
        cellInfo.power,
        cellInfo.cellVoltageMaxId,
        cellInfo.cellVoltageMax,
        cellInfo.cellVoltageMinId,
        cellInfo.cellVoltageMin,
        bal,
        cellInfo.temp0,
        cellInfo.temp1,
        cellInfo.temp2,
        cellInfo.cycleCount,
        cellInfo.capacityCycle,
        cellInfo.chargingEnabled ? "" : " [charging disabled] ",
        cellInfo.dischargingEnabled ? "" : " [discharging disabled] ",
        cellInfo.errors);
}

std::string PeerCharacteristicJkBms::errorToString(const uint16_t mask) {
    static const uint8_t numItems = 16;
    static const char items[numItems][40] = {
        "Charge Overtemperature",               // 0000 0000 0000 0001
        "Charge Undertemperature",              // 0000 0000 0000 0010
        "Error 0x00 0x04",                      // 0000 0000 0000 0100
        "Cell Undervoltage",                    // 0000 0000 0000 1000
        "Error 0x00 0x10",                      // 0000 0000 0001 0000
        "Error 0x00 0x20",                      // 0000 0000 0010 0000
        "Error 0x00 0x40",                      // 0000 0000 0100 0000
        "Error 0x00 0x80",                      // 0000 0000 1000 0000
        "Error 0x01 0x00",                      // 0000 0001 0000 0000
        "Error 0x02 0x00",                      // 0000 0010 0000 0000
        "Cell count is not equal to settings",  // 0000 0100 0000 0000
        "Current sensor anomaly",               // 0000 1000 0000 0000
        "Cell Overvoltage",                     // 0001 0000 0000 0000
        "Error 0x20 0x00",                      // 0010 0000 0000 0000
        "Charge overcurrent protection",        // 0100 0000 0000 0000
        "Error 0x80 0x00",                      // 1000 0000 0000 0000
    };
    std::string ret = "";
    if (!mask) return ret;
    for (int i = 0; i < numItems; i++) {
        if (mask & (1 << i)) {
            if (0 != i) ret.append(", ");
            ret.append(items[i]);
        }
    }
    return ret;
}

std::string PeerCharacteristicJkBms::modeToString(const uint16_t mask) {
    static const uint8_t numItems = 4;
    static const char items[numItems][32] = {
        "Charging enabled",     // 0x00
        "Discharging enabled",  // 0x01
        "Balancer enabled",     // 0x02
        "Battery dropped"       // 0x03
    };
    std::string ret = "";
    if (!mask) return ret;
    for (int i = 0; i < numItems; i++) {
        if (mask & (1 << i)) {
            if (0 != i) ret.append(", ");
            ret.append(items[i]);
        }
    }
    return ret;
}

uint8_t PeerCharacteristicJkBms::crc(const uint8_t data[], const uint16_t len) {
    uint8_t crc = 0;
    for (uint16_t i = 0; i < len; i++) {
        crc = crc + data[i];
    }
    return crc;
}

void PeerCharacteristicJkBms::printHex(const uint8_t* s, size_t size, const char* pre) {
    printf(pre);
    unsigned char* p = (unsigned char*)s;
    for (int i = 0; i < size; ++i) {
        // if (!(i % 16) && i)
        //    log_s("\n");
        printf("0x%02x ", p[i]);
    }
    printf("\n");
}

bool PeerCharacteristicJkBms::writeRegister(uint8_t address, uint32_t value, uint8_t length) {
    uint8_t frame[20];
    frame[0] = 0xAA;     // start sequence
    frame[1] = 0x55;     // start sequence
    frame[2] = 0x90;     // start sequence
    frame[3] = 0xEB;     // start sequence
    frame[4] = address;  // holding register
    frame[5] = length;   // size of the value in byte
    frame[6] = value >> 0;
    frame[7] = value >> 8;
    frame[8] = value >> 16;
    frame[9] = value >> 24;
    frame[10] = 0x00;
    frame[11] = 0x00;
    frame[12] = 0x00;
    frame[13] = 0x00;
    frame[14] = 0x00;
    frame[15] = 0x00;
    frame[16] = 0x00;
    frame[17] = 0x00;
    frame[18] = 0x00;
    frame[19] = crc(frame, sizeof(frame) - 1);

    log_d("sending frame...");
    printHex(frame, sizeof(frame), "frame: ");

    return write(String((char*)&frame, sizeof(frame)), sizeof(frame));
}

#endif