#include "heating.h"

// 渲染状态
enum _RENDERSTATE {
    _RenderMain, // 渲染主界面
    _RenderMenu, // 渲染菜单
};

// PWM 输出状态
enum _PWMSTATE {
    _PWM_OFF, // 停止输出
    _PWM_ON, // 开始输出
};

/**
 * @brief 控制器逻辑处理
 *
 */
struct _strLogic {
    _RENDERSTATE renderState;
    _PWMSTATE pwmState;
};

static _strLogic logicController {
    renderState : _RenderMain,
    pwmState : _PWM_OFF,
};

/**
 * @brief 退出菜单
 *
 */
void exitMenu(void)
{
    logicController.renderState = _RenderMain;
}

/**
 * @brief 进入菜单
 *
 */
void enterMenu(void)
{
    // 停止PWM输出
    logicController.pwmState = _PWM_OFF;

    // 进入菜单
    logicController.renderState = _RenderMenu;
}

void logic_task(void* arg)
{
    // 初始化菜单系统
    initMenuSystem();

    while (1) {
        switch (logicController.renderState) {
        case _RenderMain:
            RenerMain();
            break;

        case _RenderMenu:
            // 渲染菜单
            RenderMenu();
            break;

        default:
            break;
        }

        // 按键处理

        // PID输出

        // 渲染主界面

        // vTaskDelay(1);
    }
}
