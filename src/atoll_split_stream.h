#if !defined(__atoll_split_stream_h) && defined(FEATURE_SERIAL)
#define __atoll_split_stream_h

#include <Arduino.h>
#include <Stream.h>

namespace Atoll {

// Splits one stream into two
class SplitStream : public Stream {
   public:
    Stream *s0;
    bool s0_enabled;
    Stream *s1;
    bool s1_enabled;

    virtual void setup(
        Stream *stream0,
        Stream *stream1 = nullptr,
        bool stream0_enabled = true,
        bool stream1_enabled = true);

    virtual int available();
    virtual int read();
    virtual size_t write(uint8_t c);
    virtual size_t write(const uint8_t *buffer, size_t size);
    virtual int peek(void);
    virtual void flush(void);
    virtual operator bool() const;
};

}  // namespace Atoll

#if defined(GLOBAL_SPLITSTREAM_SERIAL) && !defined(NO_GLOBAL_INSTANCES)
extern Atoll::SplitStream Serial;
#endif

#endif