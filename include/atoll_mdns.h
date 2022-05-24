#ifndef __atoll_mdns_h
#define __atoll_mdns_h

#include <ESPmDNS.h>

#include "atoll_log.h"

namespace Atoll {

class Mdns {
   public:
    char *hostname = nullptr;
    uint16_t port = 0;

    void setup(char *hostname, uint16_t port) {
        this->hostname = hostname;
        this->port = port;
    }

    void begin() {
        if (nullptr == hostname) {
            log_e("no hostname");
            return;
        }
        if (0 == port) {
            log_e("no port");
            return;
        }
        if (!MDNS.begin(hostname)) {
            log_e("mdns start failed");
            return;
        }
        MDNS.enableArduino(port);
        log_i("mDNS responder listening on %s.local:%d", hostname, port);
    }

    void end() {
        MDNS.end();
        log_i("mDNS stopped");
    }
};

}  // namespace Atoll

#endif