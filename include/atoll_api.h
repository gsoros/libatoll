#ifndef __atoll_api_h
#define __atoll_api_h

#include <Arduino.h>
#include <CircularBuffer.h>

#include "atoll_preferences.h"
#include "atoll_ble_server.h"
#include "atoll_api_rx_callbacks.h"

#ifndef VERSION
#define VERSION "0.1"
#endif
#ifndef BUILDTAG
#define BUILDTAG "dev"
#endif

#ifndef ATOLL_API_COMMAND_STR_LENGTH
#define ATOLL_API_COMMAND_STR_LENGTH 512
#endif
#ifndef ATOLL_API_COMMAND_BUF_LENGTH
#define ATOLL_API_COMMAND_BUF_LENGTH 64
#endif
#ifndef ATOLL_API_COMMAND_NAME_LENGTH
#define ATOLL_API_COMMAND_NAME_LENGTH 16
#endif
#ifndef ATOLL_API_RESULT_NAME_LENGTH
#define ATOLL_API_RESULT_NAME_LENGTH 16
#endif
#ifndef ATOLL_API_MSG_ARG_LENGTH
#define ATOLL_API_MSG_ARG_LENGTH 239
#endif
#ifndef ATOLL_API_MSG_REPLY_LENGTH
#define ATOLL_API_MSG_REPLY_LENGTH 512
#endif
#ifndef ATOLL_API_MAX_COMMANDS
#define ATOLL_API_MAX_COMMANDS 32
#endif
#ifndef ATOLL_API_MAX_RESULTS
#define ATOLL_API_MAX_RESULTS 16
#endif
#ifndef ATOLL_API_PASSKEY
#define ATOLL_API_PASSKEY 696669
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

class ApiMessage {
   public:
    uint8_t commandCode;
    char arg[ATOLL_API_MSG_ARG_LENGTH] = "";
    ApiResult *result;
    char reply[ATOLL_API_MSG_REPLY_LENGTH] = "";
    size_t replyLength = 0;  // the actual length of the reply when it contains binary data
    bool log = true;         // set false to suppress logging when processing messages

    bool argIs(const char *str) {
        return argStartsWith(str) && strlen(arg) == strlen(str);
    }

    bool argStartsWith(const char *str) {
        int res = strncmp(arg, str, strlen(str));
        // log_i("arg: %s, str: %s, res: %d", arg, str, res);
        return res == 0;
    }

    bool argHasParam(const char *str) {
        char *match;
        match = strstr(arg, str);
        log_i("arg: %s, str: %s, res: %s", arg, str, match ? "true" : "false");
        return match ? true : false;
    }

    size_t argGetParam(const char *str, char *buf, size_t size, char delim = ';') {
        char *cp;
        cp = strstr(arg, str);
        if (!cp) {
            log_i("no match for '%s' in '%s'", str, arg);
            return 0;
        }
        size_t copied = 0;
        cp += strlen(str);
        while (copied < size) {
            if (*cp == '\0' || *cp == delim)
                break;
            buf[copied] = *cp;
            copied++;
            cp++;
        }
        buf[copied] = '\0';
        // log_i("found '%s' for '%s' in '%s'", buf, str, arg);
        return copied;
    }

    size_t replyAppend(const char *str, bool onlyIfNotEmpty = false) {
        size_t sVal = strlen(reply);
        if (onlyIfNotEmpty && !sVal) return 0;
        strncat(reply, str, ATOLL_API_MSG_REPLY_LENGTH - sVal - strlen(str) - 1);
        return strlen(reply) - sVal;
    }
};

typedef ApiResult *(*ApiProcessor)(ApiMessage *msg);

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

    ApiResult *call(ApiMessage *msg) {
        if (nullptr == processor) {
            log_e("Command %d:%s has no processor", code, name);
            return nullptr;
        }
        msg->result = processor(msg);
        // log_i("command(%d:%s)=arg(%s) ===> result(%d:%s) value(%s)",
        //       code, name, msg->arg, msg->result->code, msg->result->name, msg->value);
        return msg->result;
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
    virtual void beforeBleServiceStart(BleServer *server, BLEService *service) {}
    static bool addCommand(ApiCommand command);
    static bool addResult(ApiResult result);
    static ApiMessage process(const char *commandWithArg, bool log = true);

    static ApiResult *result(uint8_t code, bool logOnError = true);
    static ApiResult *result(const char *name, bool logOnError = true);
    static ApiResult *success();
    static ApiResult *error();
    static ApiResult *internalError();
    static ApiResult *argInvalid();

    static ApiCommand *command(uint8_t code, bool logOnError = true);
    static ApiCommand *command(const char *name, bool logOnError = true);

    static uint8_t nextAvailableCommandCode();
    static uint8_t nextAvailableResultCode();

    static const uint16_t msgReplyLength = ATOLL_API_MSG_REPLY_LENGTH;

    static void loadSettings();
    static void saveSettings();
    static void printSettings();

    static bool isAlNumStr(const char *str);

    static size_t write(const uint8_t *buffer, size_t size);

   protected:
    static CircularBuffer<char, ATOLL_API_COMMAND_BUF_LENGTH> _commandBuf;

    static ApiCommand commands[ATOLL_API_MAX_COMMANDS];
    static uint8_t numCommands;
    static ApiResult results[ATOLL_API_MAX_RESULTS];
    static uint8_t numResults;

    static ApiResult *initProcessor(ApiMessage *msg);
    static ApiResult *systemProcessor(ApiMessage *msg);

   private:
};

}  // namespace Atoll

#endif