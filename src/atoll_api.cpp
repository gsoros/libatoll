#ifdef FEATURE_API
#include "atoll_api.h"

#ifdef FEATURE_BLE_SERVER
#include "atoll_ble.h"
#endif

using namespace Atoll;

Api::Command Api::commands[ATOLL_API_MAX_COMMANDS];
uint8_t Api::numCommands = 0;
Api::Result Api::results[ATOLL_API_MAX_RESULTS];
uint8_t Api::numResults = 0;
CircularBuffer<char, ATOLL_API_COMMAND_BUF_LENGTH> Api::_commandBuf;

Api *Api::instance = nullptr;

#ifdef FEATURE_BLE_SERVER
BleServer *Api::bleServer = nullptr;
BLEUUID Api::serviceUuid = BLEUUID("DEAD");
bool Api::secureBle = false;                // whether to use LESC for BLE API service
uint32_t Api::passkey = ATOLL_API_PASSKEY;  // passkey for BLE API service, max 6 digits
#endif

Api::Result::Result(
    const char *name,
    uint8_t code) {
    this->code = code;
    snprintf(this->name, sizeof(this->name), "%s", name);
}

bool Api::Message::argIs(const char *str) {
    return argStartsWith(str) && strlen(arg) == strlen(str);
}

bool Api::Message::argStartsWith(const char *str) {
    int res = strncmp(arg, str, strlen(str));
    // log_i("arg: %s, str: %s, res: %d", arg, str, res);
    return res == 0;
}

bool Api::Message::argHasParam(const char *str) {
    char *match;
    match = strstr(arg, str);
    // log_d("arg: %s, str: %s, res: %s", arg, str, match ? "true" : "false");
    return match ? true : false;
}

size_t Api::Message::argGetParam(const char *str, char *buf, size_t size, char delim) {
    char *cp;
    cp = strstr(arg, str);
    if (!cp) {
        // log_d("no match for '%s' in '%s'", str, arg);
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

size_t Api::Message::replyAppend(const char *str, bool onlyIfNotEmpty) {
    size_t sVal = strlen(reply);
    if (onlyIfNotEmpty && !sVal) return 0;
    strncat(reply, str, ATOLL_API_MSG_REPLY_LENGTH - sVal - strlen(str) - 1);
    return strlen(reply) - sVal;
}

Api::Command::Command(
    const char *name,
    Processor processor,
    uint8_t code) {
    this->code = code;
    snprintf(this->name, sizeof(this->name), "%s", name);
    this->processor = processor;
}

Api::Result *Api::Command::call(Api::Message *msg) {
    if (nullptr == processor) {
        log_e("Command %d:%s has no processor", code, name);
        return nullptr;
    }
    msg->result = processor(msg);
    // log_i("command(%d:%s)=arg(%s) ===> result(%d:%s) value(%s)",
    //       code, name, msg->arg, msg->result->code, msg->result->name, msg->value);
    return msg->result;
}

void Api::setup(
    Api *instance,
    ::Preferences *p,
    const char *preferencesNS
#ifdef FEATURE_BLE_SERVER
    ,
    BleServer *bleServer,
    const char *serviceUuid
#endif
) {
    Atoll::Api::instance = instance;

#ifdef FEATURE_BLE_SERVER
    Atoll::Api::bleServer = bleServer;
    if (serviceUuid && strlen(serviceUuid))
        Atoll::Api::serviceUuid = BLEUUID(serviceUuid);
    else if (bleServer)
        log_e("bleServer is set but serviceUuid is not");
#endif

    if (instance)
        instance->preferencesSetup(p, preferencesNS);

    addResult(Result("success", 1));
    addResult(Result("error"));
    addResult(Result("commandMissing"));
    addResult(Result("unknownCommand"));
    addResult(Result("commandTooLong"));
    addResult(Result("argInvalid"));
    addResult(Result("argTooLong"));
    addResult(Result("internalError"));

    addCommand(Command("init", Atoll::Api::initProcessor, 1));
    addCommand(Command("system", Atoll::Api::systemProcessor));

    loadSettings();
    // printSettings();

#ifdef FEATURE_BLE_SERVER
    if (nullptr != bleServer && nullptr != serviceUuid)
        addBleService();
#endif
    _commandBuf.clear();
}

#ifdef FEATURE_BLE_SERVER
bool Api::addBleService() {
    // log_d("adding API service to BleServer");
    if (!bleServer) {
        log_e("no bleServer");
        return false;
    }
    if (!instance) {
        log_e("no instance");
        return false;
    }
    if (secureBle) {
        // log_d("setting BleServer security with passkey %d", passkey);
        bleServer->setSecurity(true, passkey);
    }

    BLEService *s = bleServer->createService(serviceUuid);
    if (nullptr == s) {
        log_e("could not create service");
        return false;
    }
    uint32_t properties = BLE_PROP::WRITE;
    if (secureBle)
        properties |= BLE_PROP::WRITE_ENC | BLE_PROP::WRITE_AUTHEN;
    BLECharacteristic *rx = s->createCharacteristic(
        BLEUUID(API_RX_CHAR_UUID),
        properties);
    if (nullptr == rx) {
        log_e("could not create char");
        return false;
    }
    rx->setCallbacks(instance);

    char str[SETTINGS_STR_LENGTH] = "";
    strncpy(str, "Ready", sizeof(str));
    properties = BLE_PROP::READ | BLE_PROP::INDICATE | BLE_PROP::NOTIFY;
    if (secureBle)
        properties |= BLE_PROP::READ_ENC | BLE_PROP::READ_AUTHEN;
    BLECharacteristic *tx = s->createCharacteristic(
        BLEUUID(API_TX_CHAR_UUID),
        properties);
    if (nullptr == tx) {
        log_e("could not create char");
        return false;
    }
    tx->setCallbacks(instance);
    tx->setValue((uint8_t *)str, strlen(str));

#ifdef FEATURE_BLELOG
    strncpy(str, "Log start", sizeof(str));
    properties = BLE_PROP::READ | BLE_PROP::INDICATE | BLE_PROP::NOTIFY;
    if (secureBle)
        properties |= BLE_PROP::READ_ENC | BLE_PROP::READ_AUTHEN;
    BLECharacteristic *log = s->createCharacteristic(
        BLEUUID(API_LOG_CHAR_UUID),
        properties);
    if (nullptr == log) {
        log_e("could not create char");
        return false;
    }
    // log->setCallbacks(instance);
    log->setValue((uint8_t *)str, strlen(str));
    Log::setWriteCallback([](const char *buf, size_t size) { Api::onLogWrite(buf, size); });
#endif

    snprintf(str, sizeof(str), "%s API v0.1", bleServer->deviceName);
    BLEDescriptor *d = rx->createDescriptor(
        BLEUUID(API_DESC_UUID),
        BLE_PROP::READ);
    if (nullptr == d) {
        log_e("could not create descriptor");
        return false;
    }
    d->setValue((uint8_t *)str, strlen(str));

    instance->beforeBleServiceStart(s);

    if (!s->start()) {
        log_e("could not start service");
        return false;
    }
    bleServer->advertiseService(serviceUuid, 1);
    // log_d("added ble service");
    return true;
}
#endif

// call with newCommand.code = 0 to assign the next available slot
bool Api::addCommand(Command newCommand) {
    if (strlen(newCommand.name) < 1) {
        log_e("no name");
        return false;
    }
    Command *existing = command(newCommand.code, false);
    if (nullptr != existing) {
        log_e("code %d already exists: %s", existing->code, existing->name);
        return false;
    }
    if (0 == newCommand.code)
        newCommand.code = nextAvailableCommandCode();
    if (0 == newCommand.code) {
        log_e("no slot for '%s'", newCommand.name);
        return false;
    }
    existing = command(newCommand.name, false);
    if (nullptr != existing) {
        for (uint8_t i = 0; i < numCommands; i++)
            if (0 == strcmp(commands[i].name, existing->name)) {
                // log_d("replacing command %d:%s", existing->code, existing->name);
                newCommand.code = existing->code;
                commands[i] = newCommand;
                return true;
            }
        log_e("error adding %d:%s", newCommand.code, newCommand.name);
        return false;
    }
    // log_d("%2d:%s", newCommand.code, newCommand.name);
    commands[numCommands] = newCommand;
    numCommands++;
    return true;
}

// returns 0 if there are no available slots
uint8_t Api::nextAvailableCommandCode() {
    if (ATOLL_API_MAX_COMMANDS <= numCommands) {
        log_e("no slots");
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
    log_e("no slot");
    return 0;
}

// call with newResult.code = 0 to assign the next available slot
bool Api::addResult(Result newResult) {
    if (strlen(newResult.name) < 1) {
        log_e("no name");
        return false;
    }
    Result *existing = result(newResult.code, false);
    if (nullptr != existing) {
        log_e("code %d already exists: %s", existing->code, existing->name);
        return false;
    }
    if (0 == newResult.code)
        newResult.code = nextAvailableResultCode();
    if (0 == newResult.code) {
        log_e("no slot for '%s'", newResult.name);
        return false;
    }
    existing = result(newResult.name, false);
    if (nullptr != existing) {
        for (uint8_t i = 0; i < numResults; i++)
            if (0 == strcmp(results[i].name, existing->name)) {
                log_w("replacing result %d:%s", existing->code, existing->name);
                newResult.code = existing->code;
                results[i] = newResult;
                return true;
            }
        log_e("error adding %d:%s", newResult.code, newResult.name);
        return false;
    }
    // log_d("%2d:%s", newResult.code, newResult.name);
    results[numResults] = newResult;
    numResults++;
    return true;
}

// returns 0 if there are no available slots
uint8_t Api::nextAvailableResultCode() {
    if (ATOLL_API_MAX_RESULTS <= numResults) {
        log_e("no slots");
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
    log_e("no slot");
    return 0;
}

void Api::loadSettings() {
    if (nullptr == instance) {
        log_e("instance null");
        return;
    }
    if (!instance->preferencesStartLoad()) return;
    secureBle = instance->preferences->getBool("secureBle", secureBle);
    passkey = (uint32_t)instance->preferences->getInt("passkey", passkey);
    instance->preferencesEnd();
}

void Api::saveSettings() {
    if (nullptr == instance) {
        log_e("instance is null");
        return;
    }
    if (!instance->preferencesStartSave()) return;
    instance->preferences->putBool("secureBle", secureBle);
    instance->preferences->putInt("passkey", (int32_t)passkey);
    instance->preferencesEnd();
}

void Api::printSettings() {
    log_i("secureBle: %s, passkey: %d", secureBle ? "yes" : "no", passkey);
}

bool Api::isAlNumStr(const char *str) {
    size_t len = strlen(str);
    size_t cnt = (size_t)0;
    while (*str != '\0' && cnt < len) {
        if (!isalnum(*str)) return false;
        str++;
        cnt++;
    }
    return true;
}

size_t Api::write(const uint8_t *buffer, size_t size) {
    size_t i = 0;
    while (i < size) {
        const char c = buffer[i];
        // log_d("%d", c);
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
                log_d("processing '%s'", buf);
                Message msg = process(buf);
#ifdef FEATURE_SERIAL
                Serial.printf("%s: %s%s%s\n", buf, msg.result->name,
                              strlen(msg.reply) ? ", " : "", msg.reply);
#endif
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

Api::Command *Api::command(uint8_t code, bool logOnError) {
    if (code < 1) return nullptr;
    for (int i = 0; i < numCommands; i++)
        if (commands[i].code == code)
            return &commands[i];
    if (logOnError) log_e("no command with code %d", code);
    return nullptr;
}

Api::Command *Api::command(const char *name, bool logOnError) {
    if (strlen(name) < 1) return nullptr;
    for (int i = 0; i < numCommands; i++)
        if (0 == strcmp(commands[i].name, name))
            return &commands[i];
    if (logOnError) log_e("no command with name '%s'", name);
    return nullptr;
}

Api::Result *Api::result(uint8_t code, bool logOnError) {
    if (code < 1) return nullptr;
    for (int i = 0; i < numResults; i++)
        if (results[i].code == code)
            return &results[i];
    if (logOnError) log_e("no result with code %d", code);
    return nullptr;
}

Api::Result *Api::result(const char *name, bool logOnError) {
    if (strlen(name) < 1) return nullptr;
    for (int i = 0; i < numResults; i++)
        if (0 == strcmp(results[i].name, name))
            return &results[i];
    if (logOnError) log_e("no result with name '%s'", name);
    return nullptr;
}

Api::Result *Api::success() {
    return result("success");
}

Api::Result *Api::error() {
    return result("error");
}

Api::Result *Api::internalError() {
    return result("internalError");
}

Api::Result *Api::argInvalid() {
    return result("argInvalid");
}

#ifdef FEATURE_BLE_SERVER
void Api::notifyTxChar(const char *str) {
    if (!bleServer) {
        log_e("no bleServer");
        return;
    }
    bleServer->notify(serviceUuid,
                      BLEUUID(API_TX_CHAR_UUID),
                      (uint8_t *)str, strlen(str));
}
#endif

// Api::Command format: commandCode|commandStr[=[arg]];
// Reply format: resultCode[:resultName];[commandCode[=value]]
Api::Message Api::process(const char *commandWithArg, bool log) {
    // log_d("Processing command %s%s", commandWithArg, log ? "" : " (logging suppressed)");
    Message msg;
    msg.log = log;
    char commandStr[ATOLL_API_COMMAND_NAME_LENGTH] = "";
    int commandWithArgLength = strlen(commandWithArg);
    char *eqSign = strstr(commandWithArg, "=");
    int commandEnd = eqSign ? eqSign - commandWithArg : commandWithArgLength;
    if (commandEnd < 1) {
        log_e("missing command: %s", commandWithArg);
        msg.result = result("commandMissing");
        return msg;
    }
    if (sizeof(commandStr) < commandEnd) {
        log_e("command too long: %s", commandWithArg);
        msg.result = result("commandTooLong");
        return msg;
    }
    strncpy(commandStr, commandWithArg, commandEnd);

    if (eqSign) {
        int argLength = commandWithArgLength - commandEnd - 1;
        // log_d("argLength=%d", argLength);
        if (ATOLL_API_MSG_ARG_LENGTH < argLength) {
            log_e("arg too long: %s", commandWithArg);
            msg.result = result("argTooLong");
            return msg;
        }
        strncpy(msg.arg, eqSign + 1, sizeof(msg.arg));
    }
    // log_d("commandStr=%s arg=%s", commandStr, msg.arg);

    Command *c = command(commandStr, false);  // try parsing command as string, don't log error
    if (nullptr == c) {
        int code = atoi(commandStr);
        if (code < 1 || UINT8_MAX < code) {  // first command index assumed to be 1
            msg.result = result("unknownCommand");
            return msg;
        }
        c = command((uint8_t)code);  // parse command as int
        if (nullptr == c) {
            msg.result = result("unknownCommand");
            return msg;
        }
    }
    msg.commandCode = c->code;

    // call the command's processor, it will set msg.result msg.reply
    c->call(&msg);

    return msg;
}

// process all available commands except 'init' without arguments
// and return the results in the format: commandCode:commandName=value;...
Api::Result *Api::initProcessor(Message *msg) {
    char reply[msgReplyLength] = "";
    char token[6 + ATOLL_API_COMMAND_NAME_LENGTH + msgReplyLength];
    Result *successResult = success();
    for (int i = 0; i < numCommands; i++) {
        if (0 == strcmp(commands[i].name, "init")) continue;
        // call command without arg, suppress logging
        Message msg = process(commands[i].name, false);
        if (msg.result == successResult)
            snprintf(token, sizeof(token), "%d:%s=%s;",
                     commands[i].code,
                     commands[i].name,
                     msg.reply);
        else
            snprintf(token, sizeof(token), "%d:%s;",
                     commands[i].code,
                     commands[i].name);
        int16_t remaining = msgReplyLength - strlen(reply) - 1;
        if (remaining < strlen(token)) {
            log_e("reply out of space: %d + %d + 1 > %d",
                  strlen(reply), strlen(token), msgReplyLength);
            log_e("token: %s", token);
            log_e("reply: %s", reply);
            return internalError();
        }
        strncat(reply, token, remaining);
    }
    strncpy(msg->reply, reply, msgReplyLength);
    return successResult;
}

Api::Result *Api::systemProcessor(Message *msg) {
    if (msg->argIs("build")) {
        snprintf(msg->reply, msgReplyLength, "%s%s %s %s",
                 VERSION, BUILDTAG, __DATE__, __TIME__);
        return success();
    } else if (msg->argIs("bootlog")) {
        Log::dumpBootLog();
        strncpy(msg->reply, "bootlog", ATOLL_API_MSG_REPLY_LENGTH);
        return success();
    } else if (msg->argIs("reboot")) {
        if (bleServer) {
            BLECharacteristic *c = bleServer->getChar(
                serviceUuid,
                BLEUUID(API_TX_CHAR_UUID));
            if (c) {
                char msg[32];
                snprintf(msg, sizeof(msg), "%d;%d=reboot", success()->code, command("system")->code);
                c->setValue((uint8_t *)msg, strlen(msg));
                c->notify();
            }
        }
        log_i("rebooting");
        delay(500);
        ESP.restart();
    }
    {
        const char *str = "secureApi";
        uint8_t sStr = strlen(str);
        if (sStr == strspn(msg->arg, str)) {
            char *arg = msg->arg;
            size_t sArg = strlen(arg);
            if (sStr < sArg) {
                // set secureApi
                if (':' != arg[sStr]) goto argInvalid;
                arg += sStr + 1;
                sArg = strlen(arg);
                if (sArg != 1) goto argInvalid;
                int i = atoi(arg);
                if (i < 0 || 1 < i) goto argInvalid;
                secureBle = (bool)i;
                saveSettings();
            }
            // get secureApi
            snprintf(msg->reply, msgReplyLength, "%d", secureBle);
            return success();
        }
    }
    {
        const char *str = "passkey";
        uint8_t sStr = strlen(str);
        if (sStr == strspn(msg->arg, str)) {
            char *arg = msg->arg;
            size_t sArg = strlen(arg);
            if (sStr < sArg) {
                // set passkey
                if (':' != arg[sStr]) goto argInvalid;
                arg += sStr + 1;
                sArg = strlen(arg);
                if (6 < sArg) goto argInvalid;
                int i = atoi(arg);
                if (i < 1 || 999999 < i) goto argInvalid;
                passkey = (uint32_t)i;
                saveSettings();
            }
            // get passkey
            snprintf(msg->reply, msgReplyLength, "%d", passkey);
            return success();
        }
    }
    {
        const char *str = "deleteBond";
        uint8_t sStr = strlen(str);
        if (sStr == strspn(msg->arg, str)) {
            char *arg = msg->arg;
            size_t sArg = strlen(arg);
            if (sArg <= sStr) goto argInvalid;
            if (':' != arg[sStr]) goto argInvalid;
            arg += sStr + 1;
            if (Ble::deleteBond(arg)) {
                snprintf(msg->reply, msgReplyLength, "deleteBond:%s", arg);
                return success();
            }
            goto argInvalid;
        }
    }
argInvalid:
    msg->replyAppend("|", true);
    msg->replyAppend("build|bootlog|reboot|secureApi[:0|1]|passkey[:1..999999]|deleteBond:[address|*]");
    return result("argInvalid");
}

// BleCharacteristicCallbacks
void Api::onWrite(BLECharacteristic *c, BLEConnInfo &connInfo) {
    if (c->getUUID().equals(BLEUUID(API_RX_CHAR_UUID))) {
        Message msg = process(c->getValue().c_str());

        // length = length(uint8max) + ":" + resultName
        char resultStr[4 + ATOLL_API_RESULT_NAME_LENGTH];

        if (msg.result->code == success()->code) {
            // in case of success we omit the resultName: "resultCode;..."
            snprintf(resultStr, sizeof(resultStr), "%d", msg.result->code);
        } else {
            // in case of an error we provide the resultName: "resultCode:resultName;..."
            snprintf(resultStr, sizeof(resultStr), "%d:%s",
                     msg.result->code, msg.result->name);
        }
        char reply[ATOLL_BLE_SERVER_CHAR_VALUE_MAXLENGTH] = "";
        snprintf(reply, sizeof(reply), "%s;%d=",
                 resultStr, msg.commandCode);
        size_t replyLength = 0;
        // in case of success we append the reply
        if (msg.result->code == success()->code) {
            size_t replyTextLength = strlen(reply);
            // msg.replyLength will be set when msg.reply contains binary data
            size_t replyDataLength = 0 < msg.replyLength ? msg.replyLength : strlen(msg.reply);
            if (sizeof(reply) < replyTextLength + replyDataLength - 1) {
                size_t prevLength = replyDataLength;
                replyDataLength = sizeof(reply) - replyTextLength - 1;
                log_w("%s reply has been cropped from %d to %d bytes",
                      reply, prevLength, replyDataLength);
            }
            // log_d("reply: '%s', msg.replyLength: %d, replyTextLength: %d, replyDataLength: %d",
            //       reply, msg.replyLength, replyTextLength, replyDataLength);
            // using memcpy to deal with binary data
            memcpy(reply + replyTextLength, msg.reply, replyDataLength);
            replyLength = replyTextLength + replyDataLength;
        } else {  // in case of an error we append the arg
            msg.replyAppend(msg.arg);
            replyLength = strlen(reply);
        }
        // log_d("apiRxChar reply(%d): %s", replyLength, reply);

#ifdef FEATURE_BLE_SERVER
        if (bleServer)
            bleServer->notify(
                serviceUuid,
                BLEUUID(API_TX_CHAR_UUID),
                (uint8_t *)reply,
                replyLength);
#endif
        return;
    }
#ifdef FEATURE_BLE_SERVER
    BleCharacteristicCallbacks::onWrite(c, connInfo);
#endif
}

#ifdef FEATURE_BLE_SERVER
void Api::onLogWrite(const char *buf, size_t size) {
#ifndef FEATURE_BLELOG
    return;
#endif
    BLECharacteristic *logChar = bleServer->getChar(serviceUuid, BLEUUID(API_LOG_CHAR_UUID));
    if (nullptr == logChar) {
#ifdef FEATURE_SERIAL
        Serial.print("[Api::onLogWrite] Error: could not get char\n");
#endif
        return;
    }
    logChar->setValue((uint8_t *)buf, size);
    logChar->notify();
}
#endif  // FEATURE_BLE_SERVER

#endif  //  FEATURE_API
