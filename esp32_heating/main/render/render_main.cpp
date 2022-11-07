#include "heating.h"
#include "ExternDraw.h"
#include "bitmap.h"
#include <string.h>
#include <math.h>

// 面板输出状态
enum _TEMP_CONSOLE_STATUS_CODE {
    TEMP_STATUS_ERROR = 0,
    TEMP_STATUS_OFF,
    TEMP_STATUS_SLEEP, // 休眠
    TEMP_STATUS_BOOST,
    TEMP_STATUS_WORKY,
    TEMP_STATUS_HEAT, // 加热
    TEMP_STATUS_HOLD, // 锁住
    TEMP_STATUS_POWER,
};

struct _consoleStatus {
    _TEMP_CONSOLE_STATUS_CODE statusID; // 状态
    char statusDesc[10]; // 状态名字
    uint8_t* pImage; // 图标
};

_consoleStatus consoleStatus[] = {
    { TEMP_STATUS_ERROR, "错误", c1 },
    { TEMP_STATUS_OFF, "停机", c2 },
    { TEMP_STATUS_SLEEP, "休眠", c3 },
    { TEMP_STATUS_BOOST, "回流", Lightning },
    { TEMP_STATUS_WORKY, "正常", c5 },
    { TEMP_STATUS_HEAT, "加热", c6 },
    { TEMP_STATUS_HOLD, "维持", c7 },
    { TEMP_STATUS_POWER, "功率", c7 },
};

static int HeatingRuningAni = 0;

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
            // sprintf(buffer, "状态%d:%s 控温:%s", TempCTRL_Status, TempCTRL_Status_Mes[TempCTRL_Status]);
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
        }
        sprintf(buffer, "hello world");

        u8g2_DrawUTF8(&u8g2, 0, 12 * i + 1, buffer);
    }
}

uint8_t* C_table[] = { c1, c2, c3, Lightning, c5, c6, c7, c7 }; // 温度控制器状态图标

/**
 * @brief 渲染主页 图形模式
 *
 */
static void renderMain_Graphics(void)
{
    _HeatingConfig* pConfig = getCurrentHeatingConfig();

    float currentTemp = adcGetHeatingTemp(); // 当前温度
    float destTemp = 0.0f;
    if (TYPE_HEATING_VARIABLE == pConfig->type) {
        if (getPIDIsStartOutput()) {
            destTemp = CalculateTemp((xTaskGetTickCount() - getStartOutputTick()) / 1000.0f, pConfig->PTemp, NULL);
        }
    } else {
        destTemp = pConfig->targetTemp;
    }

    char buf[128];

    // 显示当前配置名称
    if (strlen(pConfig->name) > 0) {
        sprintf(buf, "%s:%s", pConfig->name, heatingModeStr[pConfig->type]);
        u8g2_DrawUTF8(&u8g2, 0, 1, buf);
    } else {
        u8g2_DrawUTF8(&u8g2, 0, 1, "[未知配置]");
    }

    // 温度控制状态图标
    // Draw_Slow_Bitmap(74, 37, C_table[TempCTRL_Status], 14, 14);

    // 显示中文状态信息
    // Disp.drawUTF8(91, 40, TempCTRL_Status_Mes[TempCTRL_Status]);

    /////////////////////////////////////
    // 电源电压
    sprintf(buf, "%1.1fV", adcGetSystemVol());
    u8g2_DrawUTF8(&u8g2, 0, 42, buf);

    // 欠压告警图标闪烁
    float curVol = getSystemVoltage();
    if (0.0f != adcGetSystemVol() && curVol < adcGetSystemVol()) {
        if ((xTaskGetTickCount() / 1000) % 2) {
            uint32_t x = Get_UTF8_Ascii_Pix_Len(0, buf) + 2;
            Draw_Slow_Bitmap(x, 42, Battery_NoPower, 14, 14);
        }
    }
    /////////////////////////////////////

    // 显示蓝牙图标
    if (getBlueToolsStatus())
        Draw_Slow_Bitmap(92, 25, IMG_BLE_S, 9, 11);

    // 显示当前温度
    u8g2_SetFont(&u8g2, u8g2_font_logisoso30_tr);
    sprintf(buf, "%1.0f/%1.0f", currentTemp, destTemp);
    u8g2_DrawUTF8(&u8g2, 0, 12, buf);
    u8g2_SetFont(&u8g2, u8g2_font_wqy12_t_gb2312);

    // 右上角运行指示角标
    if (getPIDIsStartOutput()) {
        HeatingRuningAni += 2;
        if (HeatingRuningAni > 100) {
            HeatingRuningAni = 0;
        }
        uint8_t TriangleSize = map(HeatingRuningAni, 0, 100, 16, 0);
        u8g2_DrawTriangle(&u8g2, (119 - 12) + TriangleSize, 12, 125, 12, 125, (18 + 12) - TriangleSize);
    }

    ///////////////////////////////////// 绘制主界面大三角形遮罩层 /////////////////////////////////////
    // 几何图形切割
    u8g2_SetDrawColor(&u8g2, 2);
    u8g2_DrawBox(&u8g2, 0, 12, 96, 40);
    u8g2_DrawTriangle(&u8g2, 96, 12, 96, 52, 125, 42);
    u8g2_DrawTriangle(&u8g2, 125, 42, 96, 52, 118, 52);
    u8g2_SetDrawColor(&u8g2, 1);

    ///////////////////////////////////// 绘制底部状态栏 /////////////////////////////////////
    // 绘制底部状态条

    // 绘制底部 左边框
    u8g2_DrawFrame(&u8g2, 0, 53, 103, 11);

    // 当前温度 条
    u8g2_DrawBox(&u8g2, 0, 53, map(currentTemp, HeatMinTemp, HeatMaxTemp, 5, 98), 11);

    // 右边框
    u8g2_DrawFrame(&u8g2, 104, 53, 23, 11);

    // 右边框上 叠加一个 当前输出PID值的百分比
    u8g2_DrawBox(&u8g2, 104, 53, map(getPIDOutput(), 0, 100, 0, 23), 11);

    u8g2_DrawHLine(&u8g2, 117, 51, 11);
    u8g2_DrawPixel(&u8g2, 103, 52);
    u8g2_DrawPixel(&u8g2, 127, 52);

    // 进入反色
    u8g2_SetDrawColor(&u8g2, 2);

    // 绘制 设定温度 图标
    if (TYPE_HEATING_CONSTANT == pConfig->type || TYPE_T12 == pConfig->type) {
        Draw_Slow_Bitmap(map(pConfig->targetTemp, HeatMinTemp, HeatMaxTemp, 5, 98) - 4, 54, PositioningCursor, 8, 8);

    } else if (TYPE_HEATING_VARIABLE == pConfig->type) {
        float allTime = 0;
        float temp = CalculateTemp((xTaskGetTickCount() - getStartOutputTick()) / 1000.0f, pConfig->PTemp, &allTime);
        Draw_Slow_Bitmap(map(temp, HeatMinTemp, HeatMaxTemp, 5, 98) - 4, 54, PositioningCursor, 8, 8);

        // 绘制回流焊剩余时间
        if (getPIDIsStartOutput()) {
            allTime = allTime - (xTaskGetTickCount() - getStartOutputTick()) / 1000.0f;
            sprintf(buf, "%dm%ds", ((int32_t)allTime / 60), ((int32_t)allTime % 60));
            u8g2_DrawUTF8(&u8g2, 90, 0, buf);
        }
    }

    // 显示输出功率 百分比
    sprintf(buf, "%1.0f%%", getPIDOutput());
    u8g2_DrawUTF8(&u8g2, 105, 53, buf);

    u8g2_SetDrawColor(&u8g2, 1);
}

/**
 * @brief 按键处理
 *
 */
static void processKey(void)
{
    // 处理按键
    ROTARY_BUTTON_TYPE rotaryButton = getRotaryButton();
    switch (rotaryButton) {
    case RotaryButton_Click:
        // 单击 开始停止 加热
        if (NULL != pHandleEventGroup)
            xEventGroupSetBits(pHandleEventGroup, EVENT_LOGIC_HEATING);
        break;

    case RotaryButton_DoubleClick:
        // 双击
        break;

    case RotaryButton_LongClick:
        // 长按 进入菜单
        if (NULL != pHandleEventGroup)
            xEventGroupSetBits(pHandleEventGroup, EVENT_LOGIC_MENUENTER);
        break;

    default:
        break;
    }

    // 处理编码器左右旋转
    _HeatingConfig* pConfig = getCurrentHeatingConfig();
    switch (pConfig->type) {
    case TYPE_HEATING_CONSTANT: // 恒温焊台
    case TYPE_T12: // T12
        // 改变温度
        pConfig->targetTemp = GetRotaryPositon();
        break;

    case TYPE_HEATING_VARIABLE: // 回流焊
        // PopWindows("回流焊无法设置温度");
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
