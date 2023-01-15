#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int GetNumberLength(int x);
long map(long x, long in_min, long in_max, long out_min, long out_max);
uint32_t UTF8_HMiddle(uint32_t x, uint32_t w, uint8_t size, const char* s);
float CalculateTemp(float T, const float P[], float* pAllTime);
int32_t myRand(int32_t min, int32_t max);
float kalmanFilter(_KalmanInfo* kfp, float input);
float convertNTCTemp(float adcVoltage, float systemReferenceVoltage);
float ConverNThermocoupleInterpolation(uint16_t ad_data);

#ifdef __cplusplus
}
#endif // __cplusplus