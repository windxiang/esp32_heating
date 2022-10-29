#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum {
    BUTTON_NULL = 0, // 无
    BUTTON_CLICK = 1, // 单击
    BUTTON_LONGCLICK = 2, // 长按
    BUTTON_DOUBLECLICK = 3, // 双击
} ROTARY_BUTTON_TYPE;

void rotary_task(void* arg);
void RotarySet(float min, float max, float step, float position);
void RotarySetPositon(float position);

float GetRotaryPositon(void);
void setRotaryLock(bool isLock);
ROTARY_BUTTON_TYPE getRotaryButton(void);

#ifdef __cplusplus
}
#endif // __cplusplus