#ifndef __ble_constants_h
#define __ble_constants_h

/*
    Parts based on https://github.com/ihaque/pelomon/blob/main/pelomon/ble_constants.h
*/

#define DEVICE_INFORMATION_SERVICE_UUID ((uint16_t)0x180a)
#define DEVICE_NAME_CHAR_UUID ((uint16_t)0x2a00)
#define MANUFACTURER_NAME_STRING_CHAR_UUID ((uint16_t)0x2a29)
#define MODEL_NUMBER_STRING_CHAR_UUID ((uint16_t)0x2a24)
#define FIRMWARE_REVISION_STRING_CHAR_UUID ((uint16_t)0x2a26)

#define CYCLING_POWER_SERVICE_UUID ((uint16_t)0x1818)
#define CYCLING_SPEED_CADENCE_SERVICE_UUID ((uint16_t)0x1816)

#define SC_CONTROL_POINT_CHAR_UUID ((uint16_t)0x2A55)
#define SC_CONTROL_POINT_MAX_LENGTH 5
#define SC_CONTROL_POINT_OP_SET_CUMULATIVE_VALUE ((uint8_t)1)
#define SC_CONTROL_POINT_OP_START_SENSOR_CALIBRATION ((uint8_t)2)
#define SC_CONTROL_POINT_OP_UPDATE_SENSOR_LOCATION ((uint8_t)3)
#define SC_CONTROL_POINT_OP_REQUEST_SUPPORTED_SENSOR_LOCATIONS ((uint8_t)4)
#define SC_CONTROL_POINT_RESPONSE_SUCCESS ((uint8_t)1)
#define SC_CONTROL_POINT_RESPONSE_OPCODE_NOT_SUPPORTED ((uint8_t)2)
#define SC_CONTROL_POINT_RESPONSE_INVALID_PARAMETER ((uint8_t)3)
#define SC_CONTROL_POINT_RESPONSE_OPERATION_FAILED ((uint8_t)4)

#define CSC_MEASUREMENT_CHAR_UUID ((uint16_t)0x2A5B)
#define CSCM_WHEEL_REV_DATA_PRESENT (1 << 0)
#define CSCM_CRANK_REV_DATA_PRESENT (1 << 1)
#define CSC_MEASUREMENT_DESC_UUID ((uint16_t)0x2901)

#define CSC_FEATURE_CHAR_UUID ((uint16_t)0x2A5C)
#define CSCF_WHEEL_REVOLUTION_DATA_SUPPORTED ((uint16_t)1 << 0)
#define CSCF_CRANK_REVOLUTION_DATA_SUPPORTED ((uint16_t)1 << 1)
#define CSCF_MULTIPLE_SENSOR_LOCATIONS_SUPPORTED ((uint16_t)1 << 2)

#define SENSOR_LOCATION_CHAR_UUID ((uint16_t)0x2A5D)
#define SENSOR_LOCATION_OTHER ((uint8_t)0)
#define SENSOR_LOCATION_TOP_OF_SHOE ((uint8_t)1)
#define SENSOR_LOCATION_IN_SHOE ((uint8_t)2)
#define SENSOR_LOCATION_HIP ((uint8_t)3)
#define SENSOR_LOCATION_FRONT_WHEEL ((uint8_t)4)
#define SENSOR_LOCATION_LEFT_CRANK ((uint8_t)5)
#define SENSOR_LOCATION_RIGHT_CRANK ((uint8_t)6)
#define SENSOR_LOCATION_LEFT_PEDAL ((uint8_t)7)
#define SENSOR_LOCATION_RIGHT_PEDAL ((uint8_t)8)
#define SENSOR_LOCATION_FRONT_HUB ((uint8_t)9)
#define SENSOR_LOCATION_REAR_DROPOUT ((uint8_t)10)
#define SENSOR_LOCATION_CHAINSTAY ((uint8_t)11)
#define SENSOR_LOCATION_REAR_WHEEL ((uint8_t)12)
#define SENSOR_LOCATION_REAR_HUB ((uint8_t)13)
#define SENSOR_LOCATION_CHEST ((uint8_t)14)
#define SENSOR_LOCATION_SPIDER ((uint8_t)15)
#define SENSOR_LOCATION_CHAIN_RING ((uint8_t)16)

#define CYCLING_POWER_FEATURE_CHAR_UUID ((uint16_t)0x2A65)
#define CPF_PEDAL_POWER_BALANCE_SUPPORTED ((uint32_t)1 << 0)
#define CPF_ACCUMULATED_TORQUE_SUPPORTED ((uint32_t)1 << 1)
#define CPF_WHEEL_REVOLUTION_DATA_SUPPORTED ((uint32_t)1 << 2)
#define CPF_CRANK_REVOLUTION_DATA_SUPPORTED ((uint32_t)1 << 3)
#define CPF_EXTREME_MAGNITUDES_SUPPORTED ((uint32_t)1 << 4)
#define CPF_EXTREME_ANGLES_SUPPORTED ((uint32_t)1 << 5)
#define CPF_TOP_BOTTOM_DEAD_SPOT_ANGLES_SUPPORTED ((uint32_t)1 << 6)
#define CPF_ACCUMULATED_ENERGY_SUPPORTED ((uint32_t)1 << 7)
#define CPF_OFFSET_COMPENSATION_INDICATOR_SUPPORTED ((uint32_t)1 << 8)
#define CPF_OFFSET_COMPENSATION_SUPPORTED ((uint32_t)1 << 9)
#define CPF_CPM_CHAR_CONTENT_MASKING_SUPPORTED ((uint32_t)1 << 10)
#define CPF_MULTIPLE_SENSOR_LOCATIONS_SUPPORTED ((uint32_t)1 << 11)
#define CPF_CRANK_LENGTH_ADJUSTMENT_SUPPORTED ((uint32_t)1 << 12)
#define CPF_CHAIN_LENGTH_ADJUSTMENT_SUPPORTED ((uint32_t)1 << 13)
#define CPF_CHAIN_WEIGHT_ADJUSTMENT_SUPPORTED ((uint32_t)1 << 14)
#define CPF_SPAN_LENGTH_ADJUSTMENT_SUPPORTED ((uint32_t)1 << 15)
#define CPF_SENSOR_MEASUREMENT_CONTEXT_TORQUE_BASED ((uint32_t)1 << 16)
#define CPF_INSTANTANEOUS_MEASUREMENT_DIRECTION_SUPPORTED ((uint32_t)1 << 17)
#define CPF_FACTORY_CALIBRATION_DATE_SUPPORTED ((uint32_t)1 << 18)
#define CPF_ENHANCED_OFFSET_COMPENSATION_SUPPORTED ((uint32_t)1 << 19)

#define CYCLING_POWER_MEASUREMENT_CHAR_UUID ((uint16_t)0x2A63)
#define CPM_FLAG_PEDAL_POWER_BALANCE_PRESENT ((uint16_t)1 << 0)
#define CPM_FLAG_PEDAL_POWER_BALANCE_REFERENCE_LEFT ((uint16_t)1 << 1)
#define CPM_ACCUMULATED_TORQUE_PRESENT ((uint16_t)1 << 2)
#define CPM_ACCUMULATED_TORQUE_CRANK_BASED ((uint16_t)1 << 3)
#define CPM_WHEEL_REV_DATA_PRESENT ((uint16_t)1 << 4)
#define CPM_CRANK_REV_DATA_PRESENT ((uint16_t)1 << 5)
#define CPM_EXTREME_FORCE_MAGNITUDES_PRESENT ((uint16_t)1 << 6)
#define CPM_EXTREME_TORQUE_MAGNITUDES_PRESENT ((uint16_t)1 << 7)
#define CPM_EXTREME_ANGLES_PRESENT ((uint16_t)1 << 8)
#define CPM_TOP_DEAD_SPOT_ANGLE_PRESENT ((uint16_t)1 << 9)
#define CPM_BOTTOM_DEAD_SPOT_ANGLE_PRESENT ((uint16_t)1 << 10)
#define CPM_ACCUMULATED_ENERGY_PRESENT ((uint16_t)1 << 11)
#define CPM_OFFSET_COMPENSATION_ACTION_REQUIRED ((uint16_t)1 << 12)
#define CYCLING_POWER_MEASUREMENT_DESC_UUID ((uint16_t)0x2901)

#define APPEARANCE_CYCLING_GENERIC ((uint16_t)0x0480)
#define APPEARANCE_CYCLING_COMPUTER ((uint16_t)0x0481)
#define APPEARANCE_CYCLING_SPEED_SENSOR ((uint16_t)0x0482)
#define APPEARANCE_CYCLING_CADENCE_SENSOR ((uint16_t)0x0483)
#define APPEARANCE_CYCLING_POWER_SENSOR ((uint16_t)0x0484)
#define APPEARANCE_CYCLING_SPEED_CADENCE_SENSOR ((uint16_t)0x0485)

#define BATTERY_SERVICE_UUID ((uint16_t)0x180F)
#define BATTERY_LEVEL_CHAR_UUID ((uint16_t)0x2A19)
#define BATTERY_LEVEL_DESC_UUID ((uint16_t)0x2901)

#define WEIGHT_SCALE_FEATURE_UUID ((uint16_t)0x2a9e)
#define WEIGHT_SCALE_SERVICE_UUID ((uint16_t)0x181d)
#define WEIGHT_MEASUREMENT_CHAR_UUID ((uint16_t)0x2a9d)
#define WEIGHT_MEASUREMENT_DESC_UUID ((uint16_t)0x2901)

#define API_SERVICE_UUID "55bebab5-1857-4b14-a07b-d4879edad159"
#define API_RX_CHAR_UUID "da34811a-03c0-4efe-a266-ed014e181b65"
#define API_TX_CHAR_UUID "422ecb2a-be77-43f2-bf79-f0a62d31fab7"
#define API_DESC_UUID ((uint16_t)0x2901)

#define HALL_CHAR_UUID "e43fb4d4-1dc7-4ecd-9409-fb9d65dc7187"
#define HALL_DESC_UUID ((uint16_t)0x2901)

#endif