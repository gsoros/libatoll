#ifndef __atoll_vesc_h
#define __atoll_vesc_h

#include "atoll_peer.h"
#include "VescUart.h"

namespace Atoll {

class Vesc : public Peer {
   public:
    Vesc(Saved saved,
         PeerCharacteristicVescRX* customVescRX = nullptr,
         PeerCharacteristicVescTX* customVescTX = nullptr);

    VescUartBleStream bleStream;
    VescUart vescUart;
};

class VescUartBleStream : public Stream {
   public:
    virtual int available() override;
    virtual int read() override;
    virtual int peek() override;
    virtual size_t write(uint8_t) override;
    virtual size_t write(const uint8_t* buffer, size_t size) override;

    Vesc* vesc = nullptr;
};

}  // namespace Atoll

#endif