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
    char croppedReply[ATOLL_BLE_SERVER_CHAR_VALUE_MAXLENGTH] = "";
    if (msg.result->code == Api::success()->code) {
        // in case of success we append the reply
        snprintf(croppedReply, sizeof(croppedReply), "%s;%d=%s",
                 resultStr, msg.commandCode, msg.reply);
    } else {
        // in case of an error we append the arg
        snprintf(croppedReply, sizeof(croppedReply), "%s;%d=%s",
                 resultStr, msg.commandCode, msg.arg);
    }
    // log_i("apiRxChar reply(%d): %s", strlen(croppedReply), croppedReply);
    if (nullptr != Api::bleServer)
        Api::bleServer->notify(
            BLEUUID(API_SERVICE_UUID),
            BLEUUID(API_TX_CHAR_UUID),
            (uint8_t *)croppedReply,
            strlen(croppedReply));

    BLECharacteristicCallbacks::onWrite(c);
}