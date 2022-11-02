#include "heating.h"
#include "ExternDraw.h"

/**
 * @brief 渲染主页 详细信息
 *
 */
static void renderMain_Detailed(void)
{
    char buffer[50];
    // int remainingTime = TempCTRL_Status == TEMP_STATUS_BOOST ? (BoostTime - (millis() - BoostTimer) / 1000.0) : 0;

    for (uint8_t i = 0; i < 5; i++) {
        switch (i) {
        case 0:
            // sprintf(buffer, "状态%d:%s 控温:%s", TempCTRL_Status, TempCTRL_Status_Mes[TempCTRL_Status], (PIDMode == 1) ? "PID" : "模糊");
            break;
        case 1:
            // sprintf(buffer, "设定%.0lf°C 当前%.1lf°C", PID_Setpoint, (TempCTRL_Status == TEMP_STATUS_ERROR || (LastADC > 500)) ? NAN : TipTemperature);
            break;
        case 2:
            // sprintf(buffer, "PID:%.1lf 电压%.2lfV", PID_Output, SYS_Voltage);
            break;
        case 3:
            // sprintf(buffer, "回流剩余时间:%dm%ds", remainingTime / 60, remainingTime % 60);
            break;
        case 4:
            // sprintf(buffer, "p:%.3lf i:%.3lf d:%.3lf", MyPID.GetKp(), MyPID.GetKi(), MyPID.GetKd());
            break;
        }
        sprintf(buffer, "hello world");

        u8g2_DrawUTF8(&u8g2, 0, 12 * i + 1, buffer);
    }
}

/**
 * @brief 渲染主页 图形模式
 *
 */
static void renderMain_Graphics(void)
{
    // 显示当前配置名称
    u8g2_DrawUTF8(&u8g2, 0, 1, HeatingConfig.curConfig.name.c_str());

    /////////////////////////////////////绘制遮罩层//////////////////////////////////////////////
    // 几何图形切割
    u8g2_SetDrawColor(&u8g2, 2);
    u8g2_DrawBox(&u8g2, 0, 12, 96, 40);
    u8g2_DrawTriangle(&u8g2, 96, 12, 96, 52, 125, 42);
    u8g2_DrawTriangle(&u8g2, 125, 42, 96, 52, 118, 52);
    u8g2_SetDrawColor(&u8g2, 1);

    // 绘制底部状态条
    DrawStatusBar(1);
}

/**
 * @brief 按键处理
 *
 */
static void processKey(void)
{
    ROTARY_BUTTON_TYPE rotaryButton = getRotaryButton();
    switch (rotaryButton) {
    case BUTTON_CLICK:
        // 单击
        break;

    case BUTTON_DOUBLECLICK:
        // 双击
        break;

    case BUTTON_LONGCLICK:
        // 长按 进入菜单
        enterMenu();
        break;

    default:
        break;
    }
}

/**
 * @brief 渲染主页
 *
 */
void RenerMain(void)
{
    processKey();

    ClearOLEDBuffer();
    switch (SystemMenuSaveData.PanelSettings) {
    case 0:
        // 图形面板
        renderMain_Graphics();
        break;

    case 1:
        // 详细面板
        renderMain_Detailed();
        break;
    }
    Display();
}
