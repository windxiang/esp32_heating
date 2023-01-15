#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

bool t12_isInsert(void);
bool t12_isPutDown(void);
void t12PWMOutput(uint8_t dutyCycle);
void t12Init(void);

#ifdef __cplusplus
}
#endif // __cplusplus
