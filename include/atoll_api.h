#ifndef __atoll_api_h
#define __atoll_api_h

#include <Arduino.h>
#include "atoll_preferences.h"
#include "atoll_ble_server.h"
#include "atoll_api_rx_callbacks.h"

#ifndef ATOLL_API_COMMAND_STR_LENGTH
#define ATOLL_API_COMMAND_STR_LENGTH 512
#endif
#ifndef ATOLL_API_COMMAND_NAME_LENGTH
#define ATOLL_API_COMMAND_NAME_LENGTH 16
#endif
#ifndef ATOLL_API_ARG_LENGTH
#define ATOLL_API_ARG_LENGTH 239
#endif
#ifndef ATOLL_API_RESULT_NAME_LENGTH
#define ATOLL_API_RESULT_NAME_LENGTH 16
#endif
#ifndef ATOLL_API_REPLY_LENGTH
#define ATOLL_API_REPLY_LENGTH 512
#endif
#ifndef ATOLL_API_VALUE_LENGTH
#define ATOLL_API_VALUE_LENGTH 512
#endif
#ifndef ATOLL_API_MAX_COMMANDS
#define ATOLL_API_MAX_COMMANDS 32
#endif
#ifndef ATOLL_API_MAX_RESULTS
#define ATOLL_API_MAX_RESULTS 16
#endif

#ifndef API_SERVICE_UUID
#ifdef ESPCC_API_SERVICE_UUID
#define API_SERVICE_UUID ESPCC_API_SERVICE_UUID
#else
#error API_SERVICE_UUID is undefined
#endif
#endif

namespace Atoll {

class ApiResult {
   public:
    uint8_t code;
    char name[ATOLL_API_RESULT_NAME_LENGTH];

    ApiResult(
        const char *name = "",
        uint8_t code = 0) {
        this->code = code;
        strncpy(this->name, name, sizeof(this->name));
    }
};

class ApiReply {
   public:
    uint8_t commandCode;
    char arg[ATOLL_API_ARG_LENGTH] = "";
    ApiResult *result;
    char value[ATOLL_API_VALUE_LENGTH] = "";
    bool log = true;  // set false to suppress logging when processing commands
};

typedef ApiResult *(*ApiProcessor)(ApiReply *reply);

class ApiCommand {
   public:
    uint8_t code;
    char name[ATOLL_API_COMMAND_NAME_LENGTH];
    ApiProcessor processor;

    ApiCommand(
        const char *name = "",
        ApiProcessor processor = nullptr,
        uint8_t code = 0) {
        this->code = code;
        strncpy(this->name, name, sizeof(this->name));
        this->processor = processor;
    }

    ApiResult *call(ApiReply *reply) {
        if (nullptr == processor) {
            log_e("Command %d:%s has no processor", code, name);
            return nullptr;
        }
        reply->result = processor(reply);
        log_i("command(%d:%s)=arg(%s) ===> result(%d:%s) value(%s)",
              code, name, reply->arg, reply->result->code, reply->result->name, reply->value);
        return reply->result;
    };
};

class Api : public Preferences {
   public:
    static Api *instance;
    static BleServer *bleServer;
    static bool secureBle;    // whether to use LESC for BLE API service
    static uint32_t passkey;  // passkey for BLE API service, max 6 digits

    static void setup(Api *instance,
                      ::Preferences *p,
                      const char *preferencesNS,
                      BleServer *bleServer = nullptr,
                      const char *serviceUuid = nullptr);
    static bool addBleService(BleServer *bleServer, const char *serviceUuid);
    static bool addCommand(ApiCommand command);
    static bool addResult(ApiResult result);
    static ApiReply process(const char *commandWithArg, bool log = true);

    static ApiResult *result(uint8_t code, bool logOnError = true);
    static ApiResult *result(const char *name, bool logOnError = true);
    static ApiResult *success();
    static ApiResult *error();

    static ApiCommand *command(uint8_t code, bool logOnError = true);
    static ApiCommand *command(const char *name, bool logOnError = true);

    static uint8_t nextAvailableCommandCode();
    static uint8_t nextAvailableResultCode();

    static const uint16_t replyLength = ATOLL_API_REPLY_LENGTH;
    static const uint16_t replyValueLength = ATOLL_API_VALUE_LENGTH;

    static void loadSettings();
    static void saveSettings();
    static void printSettings();

   protected:
    static ApiCommand commands[ATOLL_API_MAX_COMMANDS];
    static uint8_t numCommands;
    static ApiResult results[ATOLL_API_MAX_RESULTS];
    static uint8_t numResults;

    static ApiResult *initProcessor(ApiReply *reply);
    static ApiResult *hostnameProcessor(ApiReply *reply);
    static ApiResult *buildProcessor(ApiReply *reply);

   private:
};

}  // namespace Atoll

#endif