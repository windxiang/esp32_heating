#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int GetNumberLength(int x);
long map(long x, long in_min, long in_max, long out_min, long out_max);
uint32_t UTF8_HMiddle(uint32_t x, uint32_t w, uint8_t size, const char* s);

#ifdef __cplusplus
}
#endif // __cplusplus