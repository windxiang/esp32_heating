#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

float adcGetHeatingTemp(void);
float adcGetT12Temp(void);
float adcGetT12Cur(void);
float adcGetT12NTC(void);
float adcGetRoomNTCTemp(void);
float adcGetSystemVol(void);
void adcInit(void);

#ifdef __cplusplus
}
#endif // __cplusplus
