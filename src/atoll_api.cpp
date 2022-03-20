#include "atoll_api.h"

using namespace Atoll;

ApiCommand Api::commands[ATOLL_API_MAX_COMMANDS];
uint8_t Api::numCommands;
ApiResult Api::results[ATOLL_API_MAX_RESULTS];
uint8_t Api::numResults;

void Api::setup() {
    numCommands = 0;
    numResults = 0;

    addResult(ApiResult("success", 1));
    addResult(ApiResult("error"));
    addResult(ApiResult("commandMissing"));
    addResult(ApiResult("unknownCommand"));
    addResult(ApiResult("commandTooLong"));
    addResult(ApiResult("argInvalid"));
    addResult(ApiResult("argTooLong"));
    addResult(ApiResult("internalError"));

    addCommand(ApiCommand("init", Atoll::Api::initProcessor));
    addCommand(ApiCommand("hostname", Atoll::Api::hostnameProcessor));
    addCommand(ApiCommand("build", Atoll::Api::buildProcessor));
}

// call with newCommand.code = 0 to assign the next available code
bool Api::addCommand(ApiCommand newCommand) {
    if (ATOLL_API_MAX_COMMANDS <= numCommands) {
        log_e("no slot left for '%s'", newCommand.name);
        return false;
    }
    if (strlen(newCommand.name) < 1) {
        log_e("cannot add command without name");
        return false;
    }
    ApiCommand *existing = command(newCommand.code, false);
    if (nullptr != existing) {
        log_e("Code %d already exists: %s", existing->code, existing->name);
        return false;
    }
    if (0 == newCommand.code)
        newCommand.code = nextAvailableCommandCode();
    existing = command(newCommand.name, false);
    if (nullptr != existing) {
        for (uint8_t i = 0; i < numCommands; i++)
            if (0 == strcmp(commands[i].name, existing->name)) {
                log_i("Warning: replacing command %d:%s", existing->code, existing->name);
                newCommand.code = existing->code;
                commands[i] = newCommand;
                return true;
            }
        log_e("Error adding %d:%s", newCommand.code, newCommand.name);
        return false;
    }
    log_i("Adding command %d:%s", newCommand.code, newCommand.name);
    commands[numCommands] = newCommand;
    numCommands++;
    return true;
}

// returns 0 if there are no available codes
uint8_t Api::nextAvailableCommandCode() {
    if (ATOLL_API_MAX_COMMANDS <= numCommands) {
        log_e("no more command codes left");
        return 0;
    }
    uint8_t candidate;
    for (uint8_t i = 0; i < numCommands; i++) {
        if (commands[i].code == UINT8_MAX) continue;
        candidate = commands[i].code + 1;
        if (nullptr == command(candidate, false))
            return candidate;
    }
    candidate = 1;
    if (nullptr == command(candidate, false))
        return candidate;
    log_e("could not find an available code");
    return 0;
}

// call with newResult.code = 0 to assign the next available code
bool Api::addResult(ApiResult newResult) {
    if (ATOLL_API_MAX_RESULTS <= numResults) {
        log_e("no slot left for '%s'", newResult.name);
        return false;
    }
    if (strlen(newResult.name) < 1) {
        log_e("cannot add result without a name");
        return false;
    }
    ApiResult *existing = result(newResult.code, false);
    if (nullptr != existing) {
        log_e("Code %d already exists: %s", existing->code, existing->name);
        return false;
    }
    if (0 == newResult.code)
        newResult.code = nextAvailableResultCode();
    existing = result(newResult.name, false);
    if (nullptr != existing) {
        for (uint8_t i = 0; i < numResults; i++)
            if (0 == strcmp(results[i].name, existing->name)) {
                log_i("Warning: replacing result %d:%s", existing->code, existing->name);
                newResult.code = existing->code;
                results[i] = newResult;
                return true;
            }
        log_e("Error adding %d:%s", newResult.code, newResult.name);
        return false;
    }
    log_i("Adding result %d:%s", newResult.code, newResult.name);
    results[numResults] = newResult;
    numResults++;
    return true;
}

// returns 0 if there are no available codes
uint8_t Api::nextAvailableResultCode() {
    if (ATOLL_API_MAX_RESULTS <= numResults) {
        log_e("no more command codes left");
        return 0;
    }
    uint8_t candidate;
    for (uint8_t i = 0; i < numResults; i++) {
        if (results[i].code == UINT8_MAX) continue;
        candidate = results[i].code + 1;
        if (nullptr == result(candidate, false))
            return candidate;
    }
    candidate = 1;
    if (nullptr == result(candidate, false))
        return candidate;
    log_e("could not find an available code");
    return 0;
}

ApiCommand *Api::command(uint8_t code, bool logOnError) {
    if (code < 1) return nullptr;
    for (int i = 0; i < numCommands; i++)
        if (commands[i].code == code)
            return &commands[i];
    if (logOnError) log_e("no command with code %d", code);
    return nullptr;
}

ApiCommand *Api::command(const char *name, bool logOnError) {
    if (strlen(name) < 1) return nullptr;
    for (int i = 0; i < numCommands; i++)
        if (0 == strcmp(commands[i].name, name))
            return &commands[i];
    if (logOnError) log_e("no command with name '%s'", name);
    return nullptr;
}

ApiResult *Api::result(uint8_t code, bool logOnError) {
    if (code < 1) return nullptr;
    for (int i = 0; i < numResults; i++)
        if (results[i].code == code)
            return &results[i];
    if (logOnError) log_e("no result with code %d", code);
    return nullptr;
}

ApiResult *Api::result(const char *name, bool logOnError) {
    if (strlen(name) < 1) return nullptr;
    for (int i = 0; i < numResults; i++)
        if (0 == strcmp(results[i].name, name))
            return &results[i];
    if (logOnError) log_e("no result with name '%s'", name);
    return nullptr;
}

ApiResult *Api::success() {
    return result("success");
}

ApiResult *Api::error() {
    return result("error");
}

// Command format: commandCode|commandStr[=[arg]];
// Reply format: resultCode[:resultName];[commandCode[=value]]
ApiReply Api::process(const char *commandWithArg, bool log) {
    log_i("Processing command %s%s", commandWithArg, log ? "" : " (logging suppressed)");
    Atoll::ApiReply reply;
    reply.log = log;
    char commandStr[ATOLL_API_COMMAND_NAME_LENGTH] = "";
    int commandWithArgLength = strlen(commandWithArg);
    char *eqSign = strstr(commandWithArg, "=");
    int commandEnd = eqSign ? eqSign - commandWithArg : commandWithArgLength;
    if (commandEnd < 1) {
        log_e("missing command: %s", commandWithArg);
        reply.result = result("commandMissing");
        return reply;
    }
    if (sizeof(commandStr) < commandEnd) {
        log_e("command too long: %s", commandWithArg);
        reply.result = result("commandTooLong");
        return reply;
    }
    strncpy(commandStr, commandWithArg, commandEnd);

    if (eqSign) {
        int argLength = commandWithArgLength - commandEnd - 1;
        log_i("argLength=%d", argLength);
        if (ATOLL_API_ARG_LENGTH < argLength) {
            log_e("arg too long: %s", commandWithArg);
            reply.result = result("argTooLong");
            return reply;
        }
        strncpy(reply.arg, eqSign + 1, sizeof(reply.arg));
    }
    log_i("commandStr=%s arg=%s", commandStr, reply.arg);

    ApiCommand *c = command(commandStr);
    if (nullptr == c) {
        reply.result = result("unknownCommand");
        return reply;
    }
    reply.commandCode = c->code;

    // call the command's processor, it will set reply.result, reply text and reply.value
    c->call(&reply);

    return reply;
}

ApiResult *Api::initProcessor(ApiReply *reply) {
    // value format: commandCode:commandName=value;...
    char value[valueLength] = "";
    char token[6 + ATOLL_API_COMMAND_NAME_LENGTH + ATOLL_API_VALUE_LENGTH];
    ApiResult *successResult = success();
    for (int i = 0; i < numCommands; i++) {
        if (0 == strcmp(commands[i].name, "init")) continue;
        // calling command without arg, suppress logging
        ApiReply reply = process(commands[i].name, false);
        if (reply.result == successResult)
            snprintf(token, sizeof(token), "%d:%s=%s;",
                     commands[i].code,
                     commands[i].name,
                     reply.value);
        else
            snprintf(token, sizeof(token), "%d:%s;",
                     commands[i].code,
                     commands[i].name);
        int16_t remaining = valueLength - strlen(value) - 1;
        if (remaining < strlen(token)) {
            log_e("no space left for adding %s to %s", token, value);
            return result("internalError");
        }
        strncat(value, token, remaining);
    }
    strncpy(reply->value, value, valueLength);
    return successResult;
}

ApiResult *Api::hostnameProcessor(ApiReply *reply) {
    // set hostname
    // ...
    // get hostname
    strncpy(reply->value, "libAtollApiHost", valueLength);
    return success();
}

ApiResult *Api::buildProcessor(ApiReply *reply) {
    snprintf(reply->value, valueLength, "%s %s", __DATE__, __TIME__);
    return success();
}