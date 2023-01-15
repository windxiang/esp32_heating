#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <freertos/task.h>

float adcGetHeatingTemp(void);
float adcGetT12Temp(void);
float adcGetT12Cur(void);
float adcGetT12NTC(void);
float adcGetRoomNTCTemp(void);
float adcGetSystemVol(void);
float adcGetSystem5VVol(void);
void adcProcess(int adcType, TickType_t now);
void adcInit(void);

#ifdef __cplusplus
}
#endif // __cplusplus
