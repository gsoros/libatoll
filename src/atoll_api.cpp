#include "atoll_api.h"

using namespace Atoll;

ApiCommand Api::commands[ATOLL_API_MAX_COMMANDS];
uint8_t Api::numCommands = 0;
ApiResult Api::results[ATOLL_API_MAX_RESULTS];
uint8_t Api::numResults = 0;
CircularBuffer<char, ATOLL_API_COMMAND_BUF_LENGTH> Api::_commandBuf;

Api *Api::instance = nullptr;
BleServer *Api::bleServer = nullptr;
bool Api::secureBle = false;                // whether to use LESC for BLE API service
uint32_t Api::passkey = ATOLL_API_PASSKEY;  // passkey for BLE API service, max 6 digits

void Api::setup(
    Api *instance,
    ::Preferences *p,
    const char *preferencesNS,
    BleServer *bleServer,
    const char *serviceUuid) {
    Atoll::Api::instance = instance;
    Atoll::Api::bleServer = bleServer;

    if (nullptr != instance)
        instance->preferencesSetup(p, preferencesNS);

    addResult(ApiResult("success", 1));
    addResult(ApiResult("error"));
    addResult(ApiResult("commandMissing"));
    addResult(ApiResult("unknownCommand"));
    addResult(ApiResult("commandTooLong"));
    addResult(ApiResult("argInvalid"));
    addResult(ApiResult("argTooLong"));
    addResult(ApiResult("internalError"));

    addCommand(ApiCommand("init", Atoll::Api::initProcessor, 1));
    addCommand(ApiCommand("hostname", Atoll::Api::hostnameProcessor));
    addCommand(ApiCommand("build", Atoll::Api::buildProcessor));

    if (nullptr != bleServer && nullptr != serviceUuid)
        addBleService(bleServer, serviceUuid);
}

bool Api::addBleService(BleServer *bleServer, const char *serviceUuid) {
    log_i("adding API service to BleServer");
    if (nullptr == bleServer) {
        log_e("bleServer is null");
        return false;
    }
    if (secureBle) {
        log_i("setting up secure BLE API");
        NimBLEDevice::setSecurityAuth(true, true, true);
        NimBLEDevice::setSecurityPasskey(passkey);
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
    }

    BLEService *s = bleServer->createService(BLEUUID(serviceUuid));
    if (nullptr == s) {
        log_e("could not create service");
        return false;
    }
    uint32_t properties = NIMBLE_PROPERTY::WRITE;
    if (secureBle)
        properties |= NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHEN;
    BLECharacteristic *rx = s->createCharacteristic(
        BLEUUID(API_RX_CHAR_UUID),
        properties);
    if (nullptr == rx) {
        log_e("could not create char");
        return false;
    }
    rx->setCallbacks(new ApiRxCallbacks);

    char str[SETTINGS_STR_LENGTH] = "";
    strncpy(str, "Ready", sizeof(str));
    properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::INDICATE | NIMBLE_PROPERTY::NOTIFY;
    if (secureBle)
        properties |= NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN;
    BLECharacteristic *tx = s->createCharacteristic(
        BLEUUID(API_TX_CHAR_UUID),
        properties);
    if (nullptr == tx) {
        log_e("could not create char");
        return false;
    }
    tx->setValue((uint8_t *)str, strlen(str));
    snprintf(str, sizeof(str), "%s API v0.1", bleServer->deviceName);
    BLEDescriptor *d = rx->createDescriptor(
        BLEUUID(API_DESC_UUID),
        NIMBLE_PROPERTY::READ);
    if (nullptr == d) {
        log_e("could not create descriptor");
        return false;
    }
    d->setValue((uint8_t *)str, strlen(str));
    if (!s->start()) {
        log_e("could notstart service");
        return false;
    }
    bleServer->advertiseService(BLEUUID(serviceUuid), 1);
    return true;
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

void Api::loadSettings() {
    if (!instance->preferencesStartLoad()) return;
    secureBle = instance->preferences->getBool("secureBle", secureBle);
    passkey = (uint32_t)instance->preferences->getInt("passkey", passkey);
    instance->preferencesEnd();
}

void Api::saveSettings() {
    if (!instance->preferencesStartSave()) return;
    instance->preferences->putBool("secureBle", secureBle);
    instance->preferences->putInt("passkey", (int32_t)passkey);
    instance->preferencesEnd();
}

void Api::printSettings() {
    log_i("SecureBle: %s Passkey: %d", secureBle ? "Yes" : "No", passkey);
}

size_t Api::write(const uint8_t *buffer, size_t size) {
    size_t i = 0;
    while (i < size) {
        const char c = buffer[i];
        // log_i("%d", c);
        i++;
        switch (c) {
            case 10: {  // LF
                char buf[ATOLL_API_COMMAND_BUF_LENGTH + 1] = "";
                size_t bufLen = 0;
                while (!_commandBuf.isEmpty() && bufLen < sizeof(buf)) {
                    const char c = _commandBuf.shift();
                    strncat(buf, &c, 1);
                    bufLen++;
                }
                if (!strlen(buf)) continue;
                // log_i("processing '%s'", buf);
                ApiReply reply = process(buf);
                Serial.printf("%s: %s%s%s\n", buf, reply.result->name,
                              strlen(reply.value) ? ", " : "", reply.value);
                continue;
            }
            case 4:   // EOT
            case 13:  // CR
            case 23:  // ^W
                continue;
            case 24:  // ^X
                _commandBuf.clear();
                continue;
        }
        _commandBuf.push(c);
    }
    return i;
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
    // log_i("Processing command %s%s", commandWithArg, log ? "" : " (logging suppressed)");
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
        // log_i("argLength=%d", argLength);
        if (ATOLL_API_ARG_LENGTH < argLength) {
            log_e("arg too long: %s", commandWithArg);
            reply.result = result("argTooLong");
            return reply;
        }
        strncpy(reply.arg, eqSign + 1, sizeof(reply.arg));
    }
    // log_i("commandStr=%s arg=%s", commandStr, reply.arg);

    ApiCommand *c = command(commandStr, false);  // try parsing command as string, don't log error
    if (nullptr == c) {
        int code = atoi(commandStr);
        if (code < 1 || UINT8_MAX < code) {  // first command index assumed to be 1
            reply.result = result("unknownCommand");
            return reply;
        }
        c = command((uint8_t)code);  // parse command as int
        if (nullptr == c) {
            reply.result = result("unknownCommand");
            return reply;
        }
    }
    reply.commandCode = c->code;

    // call the command's processor, it will set reply.result, reply text and reply.value
    c->call(&reply);

    return reply;
}

// process all available commands except 'init' without arguments
// and return the results in the format: commandCode:commandName=value;...
ApiResult *Api::initProcessor(ApiReply *reply) {
    char value[replyValueLength] = "";
    char token[6 + ATOLL_API_COMMAND_NAME_LENGTH + ATOLL_API_VALUE_LENGTH];
    ApiResult *successResult = success();
    for (int i = 0; i < numCommands; i++) {
        if (0 == strcmp(commands[i].name, "init")) continue;
        // call command without arg, suppress logging
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
        int16_t remaining = replyValueLength - strlen(value) - 1;
        if (remaining < strlen(token)) {
            log_e("no space left for adding %s to %s", token, value);
            return result("internalError");
        }
        strncat(value, token, remaining);
    }
    strncpy(reply->value, value, replyValueLength);
    return successResult;
}

ApiResult *Api::hostnameProcessor(ApiReply *reply) {
    // set hostname
    // ...
    // get hostname
    strncpy(reply->value, "libAtollApiHost", replyValueLength);
    return success();
}

ApiResult *Api::buildProcessor(ApiReply *reply) {
    snprintf(reply->value, replyValueLength, "%s %s", __DATE__, __TIME__);
    return success();
}
