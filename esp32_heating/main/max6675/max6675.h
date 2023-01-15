#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void max6675Init(void);
float MAX6675ReadFahrenheit(void);
bool MAX6675ReadCelsius(float* pTemp);

#ifdef __cplusplus
}
#endif // __cplusplus