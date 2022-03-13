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
    addResult(ApiResult(6, "argTooLong"));

    addCommand(ApiCommand(
        1,
        "hostname",
        Atoll::Api::hostname));
    addCommand(ApiCommand(
        2,
        "build",
        Atoll::Api::build));
}

bool Api::addCommand(ApiCommand newCommand) {
    if (ATOLL_API_MAX_COMMANDS <= numCommands) {
        log_e("no slot left for '%s'", newCommand.name);
        return false;
    }
    if (nullptr != command(newCommand.name, false) || nullptr != command(newCommand.code, false)) {
        log_e("'%s' (%d) already exists", newCommand.name, newCommand.code);
        return false;
    }
    commands[numCommands] = newCommand;
    numCommands++;
    return true;
}

bool Api::addResult(ApiResult newResult) {
    if (ATOLL_API_MAX_RESULTS <= numCommands) {
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
        if (commands[i].code == code) return &commands[i];
    if (logOnError) log_e("no command with code %d", code);
    return nullptr;
}

ApiCommand *Api::command(const char *name, bool logOnError) {
    if (strlen(name) < 1) return nullptr;
    for (int i = 0; i < numCommands; i++)
        if (0 == strcmp(commands[i].name, name)) return &commands[i];
    if (logOnError) log_e("no command with name '%s'", name);
    return nullptr;
}

ApiResult *Api::result(uint8_t code, bool logOnError) {
    if (code < 1) return nullptr;
    for (int i = 0; i < numResults; i++)
        if (results[i].code == code) return &results[i];
    if (logOnError) log_e("no result with code %d", code);
    return nullptr;
}

ApiResult *Api::result(const char *name, bool logOnError) {
    if (strlen(name) < 1) return nullptr;
    for (int i = 0; i < numResults; i++)
        if (0 == strcmp(results[i].name, name)) return &results[i];
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
// Reply format: replyCode;[commandCode|commandStr[=value]]
ApiResult *Api::process(const char *commandWithArg, char *reply, char *value) {
    log_i("Processing command %s\n", commandWithArg);
    Atoll::ApiResult *r;
    char commandStr[ATOLL_API_COMMAND_NAME_LENGTH] = "";
    char arg[replyLength] = "";
    int commandWithArgLength = strlen(commandWithArg);
    char *eqSign = strstr(commandWithArg, "=");
    int commandEnd = eqSign ? eqSign - commandWithArg : commandWithArgLength;
    if (commandEnd < 1) {
        log_e("missing command: %s", commandWithArg);
        r = result("commandMissing");
        snprintf(reply, replyLength, "%d;%s", r->code, commandWithArg);
        return r;
    }
    if (ATOLL_API_COMMAND_NAME_LENGTH < commandEnd) {
        log_e("command too long: %s", commandWithArg);
        r = result("commandTooLong");
        snprintf(reply, replyLength, "%d;%s", r->code, commandWithArg);
        return r;
    }
    strncpy(commandStr, commandWithArg, commandEnd);

    if (eqSign) {
        int argLength = commandWithArgLength - commandEnd - 1;
        log_i("argLength=%d", argLength);
        if (ATOLL_API_ARG_LENGTH < argLength) {
            log_e("arg too long: %s", commandWithArg);
            r = result("argTooLong");
            snprintf(reply, replyLength, "%d;%s", r->code, commandWithArg);
            return r;
        }
        strncpy(arg, eqSign + 1, argLength);
    }

    log_i("commandStr=%s arg=%s", commandStr, arg);

    ApiCommand *c = command(commandStr);
    if (nullptr == c) {
        r = result("unknownCommand");
        snprintf(reply, replyLength, "%d;%s", r->code, commandWithArg);
        return r;
    }

    // by default echo back the commandCode= so clients can
    // verify that this is a response to the correct command
    snprintf(reply, replyLength, "%d=", c->code);

    // call the command's processor
    r = c->call(arg, reply, value);

    // prepend the result code
    char replyTmp[replyLength];
    strncpy(replyTmp, reply, replyLength);
    if (r->code == success()->code)  // in case of succes we only return "1;..."
        snprintf(reply, replyLength, "%d;%s", r->code, replyTmp);
    else  // in case of an error, we also supply the error name: "code:name;..."
        snprintf(reply, replyLength, "%d:%s;%s", r->code, r->name, replyTmp);
    return r;
}

ApiResult *Api::hostname(const char *arg, char *reply, char *value) {
    // set hostname
    // ...
    // get hostname
    const char *hostname = "libAtollApiHost";
    // reply already contains "commandcode=", we append our hostname
    char replyTmp[replyLength];
    strncpy(replyTmp, reply, replyLength);
    snprintf(reply, replyLength, "%s%s", replyTmp, hostname);
    // set value
    strncpy(value, hostname, valueLength);
    return success();
}

ApiResult *Api::build(const char *arg, char *reply, char *value) {
    char build[32];
    snprintf(build, 32, "%s %s", __DATE__, __TIME__);
    // reply already contains "commandcode=", we append our hostname
    char replyTmp[replyLength];
    strncpy(replyTmp, reply, replyLength);
    snprintf(reply, replyLength, "%s%s", replyTmp, build);
    // set value
    strncpy(value, build, valueLength);
    return success();
}