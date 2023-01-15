#include "heating.h"
#include "bitmap.h"
#include <string.h>
#include <math.h>

static TickType_t HeatingRuningAni = 0; // 加热台运行动画
static int16_t TrendNextXPos = 0; // 下一个坐标点

void startMain(void)
{
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

    HeatingRuningAni = 0;
    TrendNextXPos = 0;

    ClearOLEDBuffer();
    Display();
}

/**
 * @brief 绘制底部状态栏
 *
 */
static void drawStatusBar(void)
{
    char buf[128];

    const _HeatingConfig* pCurConfig = getCurrentHeatingConfig();

    // 获取温度信息
    float minTemp = 0.0f, maxTemp = 0.0f, currentTemp = 0.0f;
    getTempInfo(&currentTemp, NULL, &minTemp, &maxTemp);

    // 绘制底部 左边框
    u8g2_DrawFrame(&u8g2, 0, 53, 100, 11);

    // 右边框
    u8g2_DrawFrame(&u8g2, 101, 53, 26, 11);

    // 右边框上 叠加一个 当前输出PID值的百分比
    u8g2_DrawBox(&u8g2, 101, 53, map(getPIDOutput(), 0, 100, 0, 25), 11);

    // 进入反色
    u8g2_SetDrawColor(&u8g2, 2);

    // 绘制 目标温度 菱形图标
    if (TYPE_HEATING_CONSTANT == pCurConfig->type || TYPE_T12 == pCurConfig->type) {
        Draw_Slow_Bitmap(map(pCurConfig->targetTemp, minTemp, maxTemp, 5, 95) - 4, 54, PositioningCursor, 8, 8);

    } else if (TYPE_HEATING_VARIABLE == pCurConfig->type) {
        if (getPIDIsStartOutput()) {
            float allTime = 0;
            float temp = CalculateTemp((xTaskGetTickCount() - getStartOutputTick()) / 1000.0f, pCurConfig->PTemp, &allTime);
            Draw_Slow_Bitmap(map(temp, minTemp, maxTemp, 5, 95) - 4, 54, PositioningCursor, 8, 8);
        }
    }

    // 显示输出功率 百分比
    sprintf(buf, "%1.0f%%", getPIDOutput());
    uint16_t offset = (OLED_SCREEN_WIDTH - 102 - Get_UTF8_Ascii_Pix_Len(0, buf)) / 2;
    u8g2_DrawUTF8(&u8g2, 102 + offset, 53, buf);

    u8g2_SetDrawColor(&u8g2, 1);
}

/**
 * @brief 绘制系统状态
 *
 * @param x
 * @param y
 */
static void drawStatusCode(uint16_t x, uint16_t y)
{
    // 输出状态
    char statusBuf[32];
    getStatusCode(statusBuf, sizeof(statusBuf));
    x = x - Get_UTF8_Ascii_Pix_Len(0, statusBuf);

    u8g2_DrawBox(&u8g2, x - 4, y, 30, 12); // 画背景矩形

    u8g2_SetDrawColor(&u8g2, 2);
    u8g2_DrawUTF8(&u8g2, x - 2, y + 1, statusBuf); // 输出文本
    u8g2_SetDrawColor(&u8g2, 1);
}

/**
 * @brief 绘制标题
 *
 */
static void drawTitle(void)
{
    const _HeatingConfig* pCurConfig = getCurrentHeatingConfig();
    char buf[128];

    // 当前配置
    if (strlen(pCurConfig->name) > 0) {
        snprintf(buf, sizeof(buf), "%s-%s", pCurConfig->name, heatingModeStr[pCurConfig->type]);
    } else {
        snprintf(buf, sizeof(buf), "[未知配置]");
    }
    u8g2_DrawUTF8(&u8g2, 0, 0, buf);
}

/**
 * @brief 渲染主页 详细信息
 *
 */
static void renderMain_Detailed(void)
{
    const _HeatingConfig* pCurConfig = getCurrentHeatingConfig();

    float minTemp = 0.0f, maxTemp = 0.0f, currentTemp = 0.0f, destTemp = 0.0f;
    getTempInfo(&currentTemp, &destTemp, &minTemp, &maxTemp);

    char buf[128];

    // 绘制标题
    drawTitle();

    // 绘制输出状态
    drawStatusCode(OLED_SCREEN_WIDTH, 0);

    // 当前温度
    u8g2_SetFont(&u8g2, u8g2_font_fub30_tn);
    snprintf(buf, sizeof(buf), "%03.0f", currentTemp);
    u8g2_DrawUTF8(&u8g2, 0, 14, buf);

    // 摄氏度符号
    u8g2_SetFont(&u8g2, u8g2_font_wqy16_t_gb2312);
    strncpy(buf, "°C", sizeof(buf));
    u8g2_DrawUTF8(&u8g2, 68, 14, buf);

    // 显示小数
    u8g2_SetFont(&u8g2, u8g2_font_chargen_92_mn);
    snprintf(buf, sizeof(buf), "%1.0f", (currentTemp - (int)currentTemp) * 10);
    u8g2_DrawUTF8(&u8g2, 72, 32, buf);

    // 设置温度
    snprintf(buf, sizeof(buf), "%03.0f", destTemp);
    u8g2_DrawUTF8(&u8g2, 91, 14, buf);

    if (getPIDIsStartOutput()) {
        static char maohao = ' '; // 冒号
        if (xTaskGetTickCount() - HeatingRuningAni >= 1000) {
            HeatingRuningAni = xTaskGetTickCount();
            if (maohao == ':') {
                maohao = ' ';
            } else {
                maohao = ':';
            }
        }

        // 启动时间
        u8g2_SetFont(&u8g2, u8g2_font_t0_16b_tn);
        TickType_t time = (xTaskGetTickCount() - getStartOutputTick()) / 1000.0f;
        snprintf(buf, sizeof(buf), "%02d%c%02d", time / 60, maohao, time % 60);
        u8g2_DrawUTF8(&u8g2, 90, 27, buf);

        if (pCurConfig->type == TYPE_HEATING_VARIABLE) {
            // 回流焊剩余时间
            float allTime;
            CalculateTemp(0, pCurConfig->PTemp, &allTime);
            allTime = allTime - (xTaskGetTickCount() - getStartOutputTick()) / 1000.0f;
            sprintf(buf, "%02d%c%02d", ((int32_t)allTime / 60), maohao, ((int32_t)allTime % 60));
            u8g2_DrawUTF8(&u8g2, 90, 38, buf);
        }
    }

    // 输出功率
    u8g2_SetFont(&u8g2, u8g2_font_wqy12_t_gb2312);
    drawStatusBar();
}

/**
 * @brief 渲染主页 图形模式
 *
 */
static void renderMain_Graphics(void)
{
    const _HeatingConfig* pCurConfig = getCurrentHeatingConfig();

    float minTemp = 0.0f, maxTemp = 0.0f, currentTemp = 0.0f, destTemp = 0.0f;
    getTempInfo(&currentTemp, &destTemp, &minTemp, &maxTemp);

    char buf[128];

    // 绘制标题
    drawTitle();

    // 绘制输出状态
    drawStatusCode(OLED_SCREEN_WIDTH, 0);

    // 显示当前温度
    u8g2_SetFont(&u8g2, u8g2_font_logisoso24_tr);
    sprintf(buf, "%03.0f/%03.0f", currentTemp, destTemp);
    u8g2_DrawUTF8(&u8g2, 0, 12, buf);
    u8g2_SetFont(&u8g2, u8g2_font_wqy12_t_gb2312);

    //////////////////////////////////////////////////////////////////////////
    // 电源电压
    u8g2_SetFont(&u8g2, u8g2_font_profont15_tf);
    float sysVol = adcGetSystemVol() / 1000.0f;
    sprintf(buf, "%1.1fV", sysVol);
    u8g2_DrawUTF8(&u8g2, 0, 40, buf);

    if (getPIDIsStartOutput()) {
        static char maohao = ' '; // 冒号
        if (xTaskGetTickCount() - HeatingRuningAni >= 1000) {
            HeatingRuningAni = xTaskGetTickCount();
            if (maohao == ':') {
                maohao = ' ';
            } else {
                maohao = ':';
            }
        }

        // 启动时间
        TickType_t time = (xTaskGetTickCount() - getStartOutputTick()) / 1000.0f;
        snprintf(buf, sizeof(buf), "%02d%c%02d", time / 60, maohao, time % 60);
        u8g2_DrawUTF8(&u8g2, 46, 40, buf);

        if (pCurConfig->type == TYPE_HEATING_VARIABLE) {
            // 回流焊剩余时间
            float allTime;
            CalculateTemp(0, pCurConfig->PTemp, &allTime);
            allTime = allTime - (xTaskGetTickCount() - getStartOutputTick()) / 1000.0f;
            sprintf(buf, "%02d%c%02d", ((int32_t)allTime / 60), maohao, ((int32_t)allTime % 60));
            u8g2_DrawUTF8(&u8g2, 90, 40, buf);
        }
    }

    u8g2_SetFont(&u8g2, u8g2_font_wqy12_t_gb2312);

    //////////////////////////////////////////////////////////////////////////
    // 欠压告警图标闪烁
    // float curVol = getSystemUndervoltageAlert() / 1000;
    // if (0.0f != sysVol && curVol < sysVol) {
    //     if ((xTaskGetTickCount() / 1000) % 2) {
    //         uint32_t x = Get_UTF8_Ascii_Pix_Len(0, buf) + 2;
    //         Draw_Slow_Bitmap(x, 42, Battery_NoPower, 14, 14);
    //     }
    // }

    // 显示蓝牙图标
    // if (getBlueToolsStatus())
    //     Draw_Slow_Bitmap(92, 25, IMG_BLE_S, 9, 11);

    // 绘制底部状态条
    drawStatusBar();
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
    _HeatingConfig* pCurConfig = getCurrentHeatingConfig();
    switch (pCurConfig->type) {
    case TYPE_HEATING_CONSTANT: // 恒温焊台
    case TYPE_T12: // T12
        // 改变温度
        pCurConfig->targetTemp = GetRotaryPositon();
        break;

    case TYPE_HEATING_VARIABLE: // 回流焊
        // PopWindows("回流焊无法设置温度");
        break;

    default:
        break;
    }
}

/**
 * @brief 绘制温度趋势图
 *
 */
void renderMain_Trend(void)
{
    int yBaseOffset = 15;
    char buf[64];

    // 清空标题
    u8g2_SetDrawColor(&u8g2, 0);
    u8g2_DrawBox(&u8g2, 0, 0, OLED_SCREEN_WIDTH, yBaseOffset); // 画背景矩形
    u8g2_SetDrawColor(&u8g2, 1);

    // 绘制输出状态
    drawStatusCode(OLED_SCREEN_WIDTH, 0);

    // 得到温度信息
    float currentTemp = 0, destTemp = 0, minTemp = 0, maxTemp = 0;
    getTempInfo(&currentTemp, &destTemp, &minTemp, &maxTemp);

    // 获取最大温度 最小温度 字体大小
    char cMinTemp[16], cMaxTemp[16];
    snprintf(cMinTemp, sizeof(cMinTemp), "%1.0f", minTemp);
    snprintf(cMaxTemp, sizeof(cMaxTemp), "%1.0f", maxTemp);
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tf);
    int minTempWidth = Get_UTF8_Ascii_Pix_Len(0, cMinTemp);
    int maxTempWidth = Get_UTF8_Ascii_Pix_Len(0, cMaxTemp);

    // 起始X坐标
    int xBaseOffset = max(minTempWidth, maxTempWidth) + 1;

    // 清空一下标题

    // 清空本列 和 下3列数据
    u8g2_SetDrawColor(&u8g2, 0);
    for (int i = 0; i < 4; i++) {
        uint16_t startX = xBaseOffset + TrendNextXPos + i + 1;
        if (startX >= OLED_SCREEN_WIDTH) {
            startX = xBaseOffset + TrendNextXPos + 1;
        }
        u8g2_DrawLine(&u8g2, startX, yBaseOffset, startX, OLED_SCREEN_HEIGHT - 1);
    }
    u8g2_SetDrawColor(&u8g2, 1);

    // 绘制当前列数据
    int y = map(maxTemp - currentTemp, minTemp, maxTemp + 20.0f, yBaseOffset + 1, OLED_SCREEN_HEIGHT - 1); // y轴
    u8g2_DrawPixel(&u8g2, xBaseOffset + TrendNextXPos, y);

    // 计算得出下一个绘制的横坐标
    TrendNextXPos++;
    if ((xBaseOffset + TrendNextXPos) >= OLED_SCREEN_WIDTH) {
        TrendNextXPos = 0;
    }

    // 绘制背景点
    for (int yy = yBaseOffset + 5; yy < OLED_SCREEN_HEIGHT; yy += 8) {
        for (int xx = xBaseOffset + 5; xx < OLED_SCREEN_WIDTH; xx += 8) {
            u8g2_DrawPixel(&u8g2, xx, yy);
        }
    }

    // 显示最高温度 最低温度
    u8g2_DrawStr(&u8g2, 0, yBaseOffset, cMaxTemp); // 最高温
    snprintf(cMaxTemp, sizeof(cMaxTemp), "%1.0f", maxTemp / 3);
    u8g2_DrawStr(&u8g2, 0, yBaseOffset + (OLED_SCREEN_HEIGHT - 9 - yBaseOffset) * 2 / 3, cMaxTemp); // 三分二位置
    snprintf(cMaxTemp, sizeof(cMaxTemp), "%1.0f", maxTemp / 3 * 2);
    u8g2_DrawStr(&u8g2, 0, yBaseOffset + (OLED_SCREEN_HEIGHT - 9 - yBaseOffset) / 3, cMaxTemp); // 三分一位置
    u8g2_DrawStr(&u8g2, (maxTempWidth - minTempWidth), OLED_SCREEN_HEIGHT - 9, cMinTemp); // 最低温

    // 当前温度 目标温度 显示在左上角
    u8g2_SetFont(&u8g2, u8g2_font_12x6LED_tf);
    snprintf(buf, sizeof(buf), "%03.0f/%03.0f", currentTemp, destTemp); // 当前温度 / 设置温度
    u8g2_DrawStr(&u8g2, 0, 0, buf);

    // 绘制竖线 和 横线
    u8g2_DrawLine(&u8g2, 0, yBaseOffset, OLED_SCREEN_WIDTH - 1, yBaseOffset); // 横线
    u8g2_DrawLine(&u8g2, xBaseOffset, yBaseOffset, xBaseOffset, OLED_SCREEN_HEIGHT - 1); // 竖线

    // 恢复成默认
    u8g2_SetFont(&u8g2, u8g2_font_wqy12_t_gb2312);
    u8g2_SetDrawColor(&u8g2, 1);
}

/**
 * @brief 渲染主页
 *
 */
void RenerMain(void)
{
    processKey();

    switch (SystemMenuSaveData.PanelSettings) {
    case 0:
        // 图形面板
        ClearOLEDBuffer();
        renderMain_Graphics();
        break;

    case 1:
        // 详细面板
        ClearOLEDBuffer();
        renderMain_Detailed();
        break;

    case 2:
        // 曲线面板
        // ClearOLEDBuffer(); //  因为 我没有保存所有的曲线数据  所以 曲线不能全部清空
        renderMain_Trend();
        break;
    }

    Display();
}
