#include "heating.h"

TickType_t g_ShowPageNumTime = 0;

/**
 * @brief 用户有动作时将会触发此函数
 * 编码器旋转、按钮按键按下
 *
 */
void TimerUpdateEvent(void)
{
    g_ShowPageNumTime = xTaskGetTickCount();
}
