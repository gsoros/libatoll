#include "atoll_recorder.h"

using namespace Atoll;

Recorder *Recorder::instance = nullptr;

ApiResult *Recorder::recProcessor(ApiReply *reply) {
    if (nullptr == instance) return Api::error();
    if (0 < strlen(reply->arg)) {
    }
    return Api::success();
}