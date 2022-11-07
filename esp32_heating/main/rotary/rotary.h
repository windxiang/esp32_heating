#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum {
    BUTTON_NULL = 0, // 无
    RotaryButton_Click = 1, // 编码器按键-单击
    RotaryButton_LongClick, // 编码器按键-长按
    RotaryButton_DoubleClick, // 编码器按键-双击
    NormalButton_Click,
    NormalButton_LongClick,
    NormalButton_DoubleClick,
} ROTARY_BUTTON_TYPE;

void RotarySet(float min, float max, float step, float position);
void RotarySetPositon(float position);

float GetRotaryPositon(void);
void setRotaryLock(bool isLock);
ROTARY_BUTTON_TYPE getRotaryButton(void);
void rotaryResetQueue(void);

void rotaryInit(void);

#ifdef __cplusplus
}
#endif // __cplusplus
