#if !defined(__atoll_null_serial_h) && !defined(FEATURE_SERIAL)
#define __atoll_null_serial_h

#include <Arduino.h>
#include <Stream.h>

namespace Atoll {

// Dummy Serial
class NullSerial : public Stream {
   public:
    void setup(void);
    int available(void);
    int read(void);
    size_t write(uint8_t c);
    size_t write(const uint8_t *buffer, size_t size);
    int peek(void);
    void flush(void);
    operator bool() const;
};

}  // namespace Atoll

#if !defined(NO_GLOBAL_NULLSERIAL) && !defined(NO_GLOBAL_INSTANCES)
extern Atoll::NullSerial Serial;
#endif

#endif