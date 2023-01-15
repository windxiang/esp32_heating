#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// 渲染状态
enum _RENDERSTATE {
    _RenderMain, // 渲染主界面
    _RenderMenu, // 渲染菜单
    _RenderScreenSavers, // 渲染屏保
};

// PWM 输出状态
enum _PWMIsLock {
    _PWM_UnLock = 0, // PWM没有锁住
    _PWM_Lock, // PWM锁住
};

enum _PWMIsOutput {
    _PWM_STOP = 0, // PWM 停止输出
    _PWM_START, // PWM 开始输出
};

// 状态码
enum TEMP_CTRL_STATUS_CODE {
    TEMP_STATUS_OFF, // 停止加热
    TEMP_STATUS_HEAT_HEAT, // 加热台 加热
    TEMP_STATUS_HEAT_HOLD, // 加热台 锁住温度
    TEMP_STATUS_T12_PULLOUT, // T12 拔出
    TEMP_STATUS_T12_SLEEP, // 休息
    TEMP_STATUS_T12_STOP, // 停机
    TEMP_STATUS_T12_HEAT, // 加热
    TEMP_STATUS_MAX, // 放在末尾
};

/**
 * @brief 控制器逻辑处理
 *
 */
struct _strLogic {
    _RENDERSTATE renderState; // 渲染状态
    _PWMIsLock pwmIsLock; // PWM输出状态
    _PWMIsOutput pwmOutStatus; // PWM输出状态
    TickType_t lastOperationKeyTick; // 最后按键操作时间 目前用于计算屏保时间
    TickType_t startOutputTick; // 启动PID输出时间 记录下已经输出多久时间了
    TEMP_CTRL_STATUS_CODE statusCode; // 状态码
};

TickType_t getUserOperatorTick(void);
TickType_t getStartOutputTick(void);
bool isUnderVoltage(void);
bool getPIDIsStartOutput(void);
void logicInit(void);
void getTempInfo(float* pCurTemp, float* pDestTemp, float* pMinTemp, float* pMaxTemp);
TEMP_CTRL_STATUS_CODE getStatusCode(char* pChar, int16_t charLen);

// PID接口
void resetPID(void);
double getPIDOutput(void);
void TempCtrlLoop(void);

// 菜单
void startMenu(void);

// 主界面
void startMain(void);

void RenerMain(void);
void RenderMenu(void);
void RenderScreenSavers(void);

#ifdef __cplusplus
}
#endif // __cplusplus
