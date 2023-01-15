#include "heating.h"
#include <string.h>

static const char* TAG = "logic_task";

static _strLogic logicController {
    renderState : _RenderMain,
    pwmIsLock : _PWM_UnLock,
    pwmOutStatus : _PWM_STOP,
    lastOperationKeyTick : 0,
    startOutputTick : 0,
    statusCode : TEMP_STATUS_OFF, // 默认停止
};

static char strStatusCode[][8] = {
    "停止",
    "加热",
    "锁温",
    "拔出",
    "休息",
    "停机",
    "加热",
    "Error",
};

TEMP_CTRL_STATUS_CODE getStatusCode(char* pChar, int16_t charLen)
{
    if (pChar) {
        strncpy(pChar, strStatusCode[logicController.statusCode], charLen);
    }
    return logicController.statusCode;
}

/**
 * @brief 获得用户操作按键的时间
 *
 * @return TickType_t
 */
TickType_t getUserOperatorTick(void)
{
    return logicController.lastOperationKeyTick;
}

/**
 * @brief 获取启动输出时间
 *
 * @return TickType_t
 */
TickType_t getStartOutputTick(void)
{
    return logicController.startOutputTick;
}

/**
 * @brief PID是否允许输出
 *
 * @return true 允许输出
 * @return false 禁止输出
 */
bool getPIDIsStartOutput(void)
{
    return _PWM_UnLock == logicController.pwmIsLock && _PWM_START == logicController.pwmOutStatus;
}

/**
 * @brief 获取温度信息
 *
 * @param pCurTemp 当前温度 传感器值
 * @param pDestTemp 目标温度 设定值
 * @param pMinTemp 最小温度 设定值
 * @param pMaxTemp 最大温度 设定值
 */
void getTempInfo(float* pCurTemp, float* pDestTemp, float* pMinTemp, float* pMaxTemp)
{
    const _HeatingConfig* pCurConfig = getCurrentHeatingConfig();
    const _HeatSystemConfig* pSystemConfig = getHeatingSystemConfig();

    // 获取 当前传感器温度
    if (pCurTemp) {
        *pCurTemp = adcGetHeatingTemp();
    }

    // 获取 设定目标温度
    if (pDestTemp) {
        if (TYPE_HEATING_VARIABLE == pCurConfig->type) {
            // 回流焊模式
            if (getPIDIsStartOutput()) {
                *pDestTemp = CalculateTemp((xTaskGetTickCount() - getStartOutputTick()) / 1000.0f, pCurConfig->PTemp, NULL);
            }
        } else {
            // 恒温 T12 模式
            *pDestTemp = pCurConfig->targetTemp;
        }
    }

    // 最小温度 ~ 最大温度
    if (pCurConfig->type == TYPE_HEATING_CONSTANT || pCurConfig->type == TYPE_HEATING_VARIABLE) {
        // 加热台
        if (pMinTemp) {
            *pMinTemp = pSystemConfig->HeatMinTemp;
        }
        if (pMaxTemp) {
            *pMaxTemp = pSystemConfig->HeatMaxTemp;
        }

    } else if (pCurConfig->type == TYPE_T12) {
        // T12
        if (pMinTemp) {
            *pMinTemp = pSystemConfig->T12MinTemp;
        }
        if (pMaxTemp) {
            *pMaxTemp = pSystemConfig->T12MaxTemp;
        }
    }
}

/**
 * @brief 切换PWM输出状态
 *
 */
static void switchPWMStatus(void)
{
    // 初始化PID
    resetPID();
    logicController.startOutputTick = 0;

    if (_PWM_UnLock == logicController.pwmIsLock) {
        // 解锁状态下
        if (_PWM_START == logicController.pwmOutStatus) {
            logicController.pwmOutStatus = _PWM_STOP;

        } else {
            // 可以开始输出
            logicController.pwmOutStatus = _PWM_START;

            // 记录启动时间
            logicController.startOutputTick = xTaskGetTickCount();
        }

    } else {
        logicController.pwmOutStatus = _PWM_STOP;
    }
}

/**
 * @brief 进入主界面
 *
 */
static void enterMain(void)
{
    ESP_LOGI(TAG, "进入主界面\n");

    // 进入主页
    logicController.renderState = _RenderMain;

    // 开始PWM输出
    logicController.pwmIsLock = _PWM_UnLock;
    logicController.pwmOutStatus = _PWM_STOP;

    startMain();

    resetPID();
}

/**
 * @brief 进入菜单
 *
 */
static void enterMenu(void)
{
    ESP_LOGI(TAG, "进入菜单\n");

    // 停止PWM输出
    logicController.pwmIsLock = _PWM_Lock;
    logicController.pwmOutStatus = _PWM_STOP;

    // 进入菜单
    logicController.renderState = _RenderMenu;

    startMenu();

    resetPID();
}

/**
 * @brief 进入屏保
 *
 */
void enterScreenSavers(void)
{
    ESP_LOGI(TAG, "进入屏保\n");

    // 停止PWM输出
    logicController.pwmIsLock = _PWM_Lock;
    logicController.pwmOutStatus = _PWM_STOP;

    // 进入屏保
    logicController.renderState = _RenderScreenSavers;
}

/**
 * @brief 用户有动作时将会触发此函数
 * 编码器旋转、按钮按键按下
 *
 */
static void TimerUpdateEvent(void)
{
    logicController.lastOperationKeyTick = xTaskGetTickCount();

    // 如果是在屏保模式 返回到主界面
    if (logicController.renderState == _RenderScreenSavers) {
        enterMain();
    }
}

/**
 * @brief 处理屏保程序
 *
 */
static void processScreenSavers(void)
{
    const TickType_t st = (TickType_t)getScreenProtectorTime(); // 屏保时间(s)

    // 屏保时间到 && 在主界面 && 没有输出
    if (logicController.renderState != _RenderScreenSavers && 0 != st && !getPIDIsStartOutput()) {
        const TickType_t now = xTaskGetTickCount(); // 当前时间(ms)
        if ((logicController.lastOperationKeyTick + (st * 1000)) < now) {
            // 进入屏幕保护程序
            enterScreenSavers();
        }
    }
}

/**
 * @brief 处理状态码
 *
 */
static void processStatusCode(void)
{
    if (getPIDIsStartOutput()) {
        const _HeatingConfig* pCurConfig = getCurrentHeatingConfig();

        float minTemp = 0.0f, maxTemp = 0.0f, currentTemp = 0.0f, destTemp = 0.0f;
        getTempInfo(&currentTemp, &destTemp, &minTemp, &maxTemp);

        if (TYPE_HEATING_CONSTANT == pCurConfig->type || TYPE_HEATING_VARIABLE == pCurConfig->type) {
            // 加热台
            float temp = abs(currentTemp - destTemp);
            if (temp < 5.0f) {
                // 控温
                logicController.statusCode = TEMP_STATUS_HEAT_HOLD;
            } else {
                // 加热
                logicController.statusCode = TEMP_STATUS_HEAT_HEAT;
            }

        } else if (TYPE_HEATING_VARIABLE == pCurConfig->type) {
            // T12
            if (!t12_isInsert()) {
                // T12 拔出
                logicController.statusCode = TEMP_STATUS_T12_PULLOUT;

            } else {
                if (t12_isPutDown()) {
                    // T12 放下
                    // 继续判断放下时间 确定是
                    // 停机状态
                    // 休息状态
                } else {
                    logicController.statusCode = TEMP_STATUS_T12_HEAT;
                }
            }
        }

    } else {
        logicController.statusCode = TEMP_STATUS_OFF;
    }
}

/**
 * @brief 主逻辑运行入口
 *
 * @param arg
 */
static void logic_task(void* arg)
{
    while (1) {
        // 事件处理
        const EventBits_t uxBitsToWaitFor = EVENT_LOGIC_MENUEXIT | EVENT_LOGIC_MENUENTER | EVENT_LOGIC_KEYUP | EVENT_LOGIC_HEATING;
        EventBits_t bits = xEventGroupWaitBits(pHandleEventGroup, uxBitsToWaitFor, pdFALSE, pdFALSE, 1 / portTICK_PERIOD_MS);
        xEventGroupClearBits(pHandleEventGroup, bits);

        if (bits & EVENT_LOGIC_MENUEXIT) {
            // 菜单退出
            enterMain();

        } else if (bits & EVENT_LOGIC_MENUENTER) {
            // 菜单进入
            enterMenu();

        } else if (bits & EVENT_LOGIC_KEYUP) {
            // 按键操作
            TimerUpdateEvent();

        } else if (bits & EVENT_LOGIC_HEATING) {
            // 开始 停止 加热台加热
            switchPWMStatus();
        }

        // 渲染处理
        switch (logicController.renderState) {
        case _RenderMain:
            // 渲染主界面
            RenerMain();
            break;

        case _RenderMenu:
            // 渲染菜单
            RenderMenu();
            break;

        case _RenderScreenSavers:
            // 渲染屏保
            RenderScreenSavers();
            break;

        default:
            break;
        }

        // PID输出
        TempCtrlLoop();

        // 处理状态码
        processStatusCode();

        // 处理屏保程序
        processScreenSavers();
    }
}

void logicInit(void)
{
    // 初始化菜单系统
    initMenuSystem();

    // 进入主菜单
    enterMain();

    // 启动logic线程
    xTaskCreatePinnedToCore(logic_task, "logic", 1024 * 20, NULL, 5, NULL, tskNO_AFFINITY);

    // 播放开机声音
    SetSound(BeepSoundBoot, false);
}
