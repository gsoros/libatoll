#if !defined(__atoll_api_h) && defined(FEATURE_API)
#define __atoll_api_h

#include <Arduino.h>
#include <CircularBuffer.h>

#include "atoll_preferences.h"

#ifdef FEATURE_BLE_SERVER
#include "atoll_ble_server.h"
#include "atoll_ble_characteristic_callbacks.h"
#endif

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
#define ATOLL_API_COMMAND_BUF_LENGTH 128
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

#if API_SERVICE == 1
#define API_SERVICE_UUID ESPM_API_SERVICE_UUID
#else
#if API_SERVICE == 2
#define API_SERVICE_UUID ESPCC_API_SERVICE_UUID
#else
#ifndef API_SERVICE_UUID
#define API_SERVICE_UUID ""
#endif
#endif
#endif

namespace Atoll {

class Api : public Preferences, public BleCharacteristicCallbacks {
   public:
    struct Result {
       public:
        uint8_t code;
        char name[ATOLL_API_RESULT_NAME_LENGTH];

        Result(const char *name = "", uint8_t code = 0);
    };

    struct Message {
       public:
        uint8_t commandCode;
        char arg[ATOLL_API_MSG_ARG_LENGTH] = "";
        Result *result;
        char reply[ATOLL_API_MSG_REPLY_LENGTH] = "";
        size_t replyLength = 0;  // the actual length of the reply when it contains binary data
        bool log = true;         // set false to suppress logging when processing messages

        bool argIs(const char *str);
        bool argStartsWith(const char *str);
        bool argHasParam(const char *str);
        size_t argGetParam(const char *str, char *buf, size_t size, char delim = ';');
        size_t replyAppend(const char *str, bool onlyIfNotEmpty = false);
    };

    // typedef Result *(*Processor)(Message *);
    typedef std::function<Result *(Message *)> Processor;

    struct Command {
       public:
        uint8_t code;
        char name[ATOLL_API_COMMAND_NAME_LENGTH];
        Processor processor;

        Command(const char *name = "",
                Processor processor = nullptr,
                uint8_t code = 0);

        Result *call(Message *msg);
    };

    static Api *instance;

#ifdef FEATURE_BLE_SERVER
    static Atoll::BleServer *bleServer;
    static BLEUUID serviceUuid;
    static bool secureBle;    // whether to use LESC for BLE API service
    static uint32_t passkey;  // passkey for BLE API service, max 6 digits
#endif

    static void setup(Api *instance,
                      ::Preferences *p,
                      const char *preferencesNS,

#ifdef FEATURE_BLE_SERVER
                      BleServer *bleServer = nullptr,
                      const char *serviceUuid = nullptr
#endif
    );

#ifdef FEATURE_BLE_SERVER
    static bool addBleService();
    virtual void beforeBleServiceStart(BLEService *service) {}
#endif

    static bool addCommand(Command command);
    static bool addResult(Result result);
    static Message process(const char *commandWithArg, bool log = true);

    static Result *result(uint8_t code, bool logOnError = true);
    static Result *result(const char *name, bool logOnError = true);
    static Result *success();
    static Result *error();
    static Result *internalError();
    static Result *argInvalid();

    static Command *command(uint8_t code, bool logOnError = true);
    static Command *command(const char *name, bool logOnError = true);

    static uint8_t nextAvailableCommandCode();
    static uint8_t nextAvailableResultCode();

    static const uint16_t msgReplyLength = ATOLL_API_MSG_REPLY_LENGTH;

    static void loadSettings();
    static void saveSettings();
    static void printSettings();

    static bool isAlNumStr(const char *str);

    static size_t write(const uint8_t *buffer, size_t size);

#ifdef FEATURE_BLE_SERVER
    // BleCharacteristicCallbacks
    void onWrite(BLECharacteristic *c, BLEConnInfo &connInfo) override;

    void notifyTxChar(const char *str);
#endif

   protected:
    static CircularBuffer<char, ATOLL_API_COMMAND_BUF_LENGTH> _commandBuf;

    static Command commands[ATOLL_API_MAX_COMMANDS];
    static uint8_t numCommands;
    static Result results[ATOLL_API_MAX_RESULTS];
    static uint8_t numResults;

    static Result *initProcessor(Message *msg);
    static Result *systemProcessor(Message *msg);

    static void onLogWrite(const char *buf, size_t size);

   private:
};

}  // namespace Atoll

#endif