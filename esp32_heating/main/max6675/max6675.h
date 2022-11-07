#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void max6675Init(void);
float MAX6675ReadFahrenheit(void);
float MAX6675ReadCelsius(void);

#ifdef __cplusplus
}
#endif // __cplusplus