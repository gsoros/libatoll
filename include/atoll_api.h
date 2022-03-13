#ifndef __atoll_api_h
#define __atoll_api_h

#include <Arduino.h>

#ifndef ATOLL_API_COMMAND_STR_LENGTH
#define ATOLL_API_COMMAND_STR_LENGTH 256
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
#define ATOLL_API_REPLY_LENGTH 256
#endif
#ifndef ATOLL_API_VALUE_LENGTH
#define ATOLL_API_VALUE_LENGTH 32
#endif
#ifndef ATOLL_API_MAX_COMMANDS
#define ATOLL_API_MAX_COMMANDS 32
#endif
#ifndef ATOLL_API_MAX_RESULTS
#define ATOLL_API_MAX_RESULTS 16
#endif

namespace Atoll {

class ApiResult {
   public:
    uint8_t code;
    char name[ATOLL_API_RESULT_NAME_LENGTH];

    ApiResult(uint8_t code = 0, const char *name = "") {
        this->code = code;
        strncpy(this->name, name, sizeof(this->name));
    }
};

typedef ApiResult *(*ApiProcessor)(const char *str, char *reply, char *value);

class ApiCommand {
   public:
    uint8_t code;
    char name[ATOLL_API_COMMAND_NAME_LENGTH];
    ApiProcessor processor;

    ApiCommand(uint8_t code = 0,
               const char *name = "",
               ApiProcessor processor = nullptr) {
        this->code = code;
        strncpy(this->name, name, sizeof(this->name));
        this->processor = processor;
    }

    ApiResult *call(const char *arg, char *reply, char *value) {
        ApiResult *result = processor(arg, reply, value);
        log_i("command(%d:%s)=arg(%s) ===> result(%d:%s) reply(%s) value(%s)",
              code, name, arg, result->code, result->name, reply, value);
        return result;
    };
};

class Api {
   public:
    static void setup();
    static bool addCommand(ApiCommand command);
    static bool addResult(ApiResult result);
    static ApiResult *process(const char *commandWithArg, char *reply, char *value);

    static ApiResult *result(uint8_t code, bool logOnError = true);
    static ApiResult *result(const char *name, bool logOnError = true);
    static ApiResult *success();
    static ApiResult *error();

    static const uint16_t replyLength = ATOLL_API_REPLY_LENGTH;
    static const uint16_t valueLength = ATOLL_API_VALUE_LENGTH;

   protected:
    static ApiCommand commands[ATOLL_API_MAX_COMMANDS];
    static uint8_t numCommands;
    static ApiResult results[ATOLL_API_MAX_RESULTS];
    static uint8_t numResults;

    static ApiCommand *command(uint8_t code, bool logOnError = true);
    static ApiCommand *command(const char *name, bool logOnError = true);

    static ApiResult *hostname(const char *arg, char *reply, char *value);
    static ApiResult *build(const char *arg, char *reply, char *value);
};

}  // namespace Atoll

#endif