#include "atoll_api.h"

using namespace Atoll;

ApiCommand Api::commands[ATOLL_API_MAX_COMMANDS];
uint8_t Api::numCommands;
ApiResult Api::results[ATOLL_API_MAX_RESULTS];
uint8_t Api::numResults;

void Api::setup() {
    numCommands = 0;
    numResults = 0;

    addResult(ApiResult(1, "success"));
    addResult(ApiResult(2, "error"));
    addResult(ApiResult(3, "commandMissing"));
    addResult(ApiResult(4, "unknownCommand"));
    addResult(ApiResult(5, "commandTooLong"));
    addResult(ApiResult(6, "argInvalid"));
    addResult(ApiResult(7, "argTooLong"));
    addResult(ApiResult(8, "internalError"));

    addCommand(ApiCommand(
        1,
        "hostname",
        Atoll::Api::hostnameProcessor));
    addCommand(ApiCommand(
        2,
        "build",
        Atoll::Api::buildProcessor));
}

bool Api::addCommand(ApiCommand newCommand) {
    if (ATOLL_API_MAX_COMMANDS <= numCommands) {
        log_e("no slot left for '%s'", newCommand.name);
        return false;
    }
    ApiCommand *byCode = command(newCommand.code, false);
    ApiCommand *byName = command(newCommand.name, false);
    if (nullptr != byCode || nullptr != byName) {
        if (byCode != byName) {
            log_e("%d:%s already exists", newCommand.name, newCommand.code);
            return false;
        }
        for (uint8_t i = 0; i < numCommands; i++)
            if (commands[i].code == byCode->code && commands[i].name == byCode->name) {
                log_i("Warning: replacing command %d:%s", byCode->code, byCode->name);
                commands[i] = newCommand;
                return true;
            }
    }
    commands[numCommands] = newCommand;
    numCommands++;
    return true;
}

bool Api::addResult(ApiResult newResult) {
    if (ATOLL_API_MAX_RESULTS <= numResults) {
        log_e("no slot left for '%s'", newResult.name);
        return false;
    }
    if (nullptr != result(newResult.name, false) || nullptr != result(newResult.code, false)) {
        log_e("'%s' (%d) already exists", newResult.name, newResult.code);
        return false;
    }
    results[numResults] = newResult;
    numResults++;
    return true;
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
ApiReply Api::process(const char *commandWithArg) {
    log_i("Processing command %s\n", commandWithArg);
    Atoll::ApiReply reply;
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