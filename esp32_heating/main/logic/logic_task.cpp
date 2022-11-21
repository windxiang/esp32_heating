#include "heating.h"

static const char* TAG = "logic_task";

static _strLogic logicController {
    renderState : _RenderMain,
    pwmIsLock : _PWM_UnLock,
    pwmOutStatus : _PWM_STOP,
    lastOperationKeyTick : 0,
    startOutputTick : 0,
};

/**
 * @brief 获得用户操作时间
 *
 * @return TickType_t
 */
TickType_t getUserOperatorTick(void)
{
    return logicController.lastOperationKeyTick;
}

/**
 * @brief 获取启动输出Tick
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
 * @brief 切换PWM输出状态
 *
 */
static void switchPWMStatus(void)
{
    if (_PWM_UnLock == logicController.pwmIsLock) {
        if (_PWM_START == logicController.pwmOutStatus) {
            logicController.pwmOutStatus = _PWM_STOP;

        } else {
            // 可以开始输出
            logicController.pwmOutStatus = _PWM_START;

            // 初始化PID
            startPID();

            // 记录时间
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

    _HeatingConfig* pCurConfig = getCurrentHeatingConfig();
    _HeatSystemConfig* pSystemConfig = getHeatingSystemConfig();
    if (pCurConfig->type == TYPE_HEATING_VARIABLE || pCurConfig->type == TYPE_HEATING_CONSTANT) {
        // 加热台 回流焊
        RotarySet(pSystemConfig->HeatMinTemp, pSystemConfig->HeatMaxTemp, 1, pCurConfig->targetTemp);
    } else {
        // T12
        RotarySet(pSystemConfig->T12MinTemp, pSystemConfig->T12MaxTemp, 1, pCurConfig->targetTemp);
    }

    rotaryResetQueue();
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

    // RotarySet(0, 5, 1, 2);
    startMenu();
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
    TickType_t st = (TickType_t)getScreenProtectorTime() * 1000; //

    // 屏保时间到 && 在主界面 && 没有输出
    if (logicController.renderState != _RenderScreenSavers && 0 != st && !getPIDIsStartOutput()) {
        TickType_t now = xTaskGetTickCount();
        if ((logicController.lastOperationKeyTick + st) < now) {
            // 进入屏幕保护程序
            enterScreenSavers();
        }
    }
}

/**
 * @brief 主逻辑运行入口
 *
 * @param arg
 */
static void logic_task(void* arg)
{
    SetSound(BeepSoundBoot, false);

    while (1) {
        // 事件处理
        EventBits_t uxBitsToWaitFor = EVENT_LOGIC_MENUEXIT | EVENT_LOGIC_MENUENTER | EVENT_LOGIC_KEYUPDATE | EVENT_LOGIC_HEATING;
        EventBits_t bits = xEventGroupWaitBits(pHandleEventGroup, uxBitsToWaitFor, pdFALSE, pdFALSE, 1 / portTICK_PERIOD_MS);
        xEventGroupClearBits(pHandleEventGroup, bits);

        if (bits & EVENT_LOGIC_MENUEXIT) {
            // 菜单退出
            enterMain();

        } else if (bits & EVENT_LOGIC_MENUENTER) {
            // 菜单进入
            enterMenu();

        } else if (bits & EVENT_LOGIC_KEYUPDATE) {
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

    xTaskCreatePinnedToCore(logic_task, "logic", 1024 * 20, NULL, 5, NULL, tskNO_AFFINITY);
}
