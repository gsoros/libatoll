#ifndef __atoll_serial_h
#define __atoll_serial_h

#ifdef FEATURE_SERIAL
#include "atoll_split_stream.h"
#include "atoll_wifi_serial.h"
#else
#include "atoll_null_serial.h"
#endif

#endif