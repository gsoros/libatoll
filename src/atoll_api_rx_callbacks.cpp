#include "atoll_api_rx_callbacks.h"

using namespace Atoll;

void ApiRxCallbacks::onWrite(BLECharacteristic *c) {
    ApiMessage msg = Api::process(c->getValue().c_str());

    // length = length(uint8max) + ":" + resultName
    char resultStr[4 + ATOLL_API_RESULT_NAME_LENGTH];

    if (msg.result->code == Api::success()->code) {
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
    if (msg.result->code == Api::success()->code) {
        size_t replyTextLength = strlen(reply);
        // msg.replyLength will be set when msg.reply contains binary data
        size_t replyDataLength = 0 < msg.replyLength ? msg.replyLength : strlen(msg.reply);
        if (sizeof(reply) < replyTextLength + replyDataLength - 1) {
            size_t prevLength = replyDataLength;
            replyDataLength = sizeof(reply) - replyTextLength - 1;
            log_w("%s reply has been cropped from %d to %d bytes",
                  reply, prevLength, replyDataLength);
        }
        // log_i("reply: '%s', msg.replyLength: %d, replyTextLength: %d, replyDataLength: %d",
        //      reply, msg.replyLength, replyTextLength, replyDataLength);
        // using memcpy to deal with binary data
        memcpy(reply + replyTextLength, msg.reply, replyDataLength);
        replyLength = replyTextLength + replyDataLength;
    } else {  // in case of an error we append the arg
        msg.replyAppend(msg.arg);
        replyLength = strlen(reply);
    }
    log_i("apiRxChar reply(%d): %s", replyLength, reply);
    if (Api::bleServer)
        Api::bleServer->notify(
            Api::serviceUuid,
            BLEUUID(API_TX_CHAR_UUID),
            (uint8_t *)reply,
            replyLength);

    BLECharacteristicCallbacks::onWrite(c);
}