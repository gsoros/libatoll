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

    void setup(
        Stream *stream0,
        Stream *stream1,
        bool stream0_enabled,
        bool stream1_enabled);

    int available();
    int read();
    size_t write(uint8_t c);
    size_t write(const uint8_t *buffer, size_t size);
    int peek(void);
    void flush(void);
    operator bool() const;
};

}  // namespace Atoll

#if !defined(NO_GLOBAL_SPLITSTREAM) && !defined(NO_GLOBAL_INSTANCES)
extern Atoll::SplitStream Serial;
#endif

#endif