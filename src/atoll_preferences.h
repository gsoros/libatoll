#ifndef __atoll_preferences_h
#define __atoll_preferences_h

#include <Arduino.h>
#include <Preferences.h>

#include "atoll_log.h"

namespace Atoll {

class Preferences {
   public:
    ::Preferences *preferences;
    const char *preferencesNS;

    void preferencesSetup(::Preferences *p, const char *ns) {
        if (strlen(ns) < 1) log_e("empty namespace");
        preferences = p;
        preferencesNS = ns;
    }

    bool preferencesStartLoad() {
        if (!preferences->begin(preferencesNS, true))         // try ro mode
            return _preferencesBeginRW();                     // open in rw mode to create ns
        return true;
    };

    bool preferencesStartSave() {
        return _preferencesBeginRW();
    };

    void preferencesEnd() {
        preferences->end();
    }

    protected:
     bool _preferencesBeginRW() {
         if (!preferences->begin(preferencesNS, false)) {
             log_e("failed for '%s'.", preferencesNS);
             return false;
         }
         return true;
     };
};

}  // namespace Atoll

#endif