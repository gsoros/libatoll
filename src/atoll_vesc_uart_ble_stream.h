#ifndef __atoll_vesc_uart_ble_stream_h
#define __atoll_vesc_uart_ble_stream_h

#include <NimBLEDevice.h>
#include <CircularBuffer.h>

#include "atoll_peer.h"
#include "VescUart.h"

namespace Atoll {

class Vesc;

class VescUartBleStream : public Stream {
   public:
    VescUartBleStream();
    virtual int available() override;
    virtual int read() override;
    virtual int peek() override;
    virtual size_t write(uint8_t) override;
    virtual size_t write(const uint8_t* buffer, size_t size) override;

    Vesc* vesc = nullptr;

    CircularBuffer<uint8_t, 512> rxBuf;
};

}  // namespace Atoll

#endif