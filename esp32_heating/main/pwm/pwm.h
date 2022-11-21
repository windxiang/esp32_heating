#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// PWM 输出控制类型
enum _PWMTYPE {
    _TYPE_T12,
    _TYPE_FAN,
    _TYPE_HEAT,
    _TYPE_BEEP,
    _TYPE_MAX,
};

bool beepOutput(uint32_t freq);
bool pwmOutput(_PWMTYPE type, uint16_t value);
void pwmInit(void);

#ifdef __cplusplus
}
#endif // __cplusplus