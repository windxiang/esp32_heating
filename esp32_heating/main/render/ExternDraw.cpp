#include "ExternDraw.h"
#include "heating.h"
#include <math.h>
#include <string.h>
#include "bitmap.h"

double TipTemperature = 0; // PID输入值 (当前温度)
double PID_Output = 0; // PID输出值 要输出PWM宽度
double PID_Setpoint = 0; // PID目标值 (设定温度值)

uint32_t POWER = 0;
uint8_t PWM_Resolution = 10; // 分辨率
float BoostTime = 0; // 爆发模式持续时间

void EnterLogo(void)
{
    // for (int16_t x = -128; x < 128; x += 12) {
    //     //绘制Logo
    //     Disp.setDrawColor(1);
    //     Draw_Slow_Bitmap(0, 0, Logo, 128, 64);
    //     //转场特效
    //     Disp.setBitmapMode(1);
    //     Disp.setDrawColor(0);

    //     Disp.drawXBM(x, 0, 128, 64, TranAnimation);
    //     if (x < 0)
    //         Disp.drawBox(128 + x, 0, -x, 64);

    //     Disp.setBitmapMode(0);
    //     Display();
    // }
    // Disp.setDrawColor(1);
#if 0
    float rate, i = 1;
    int x, y, w;
    uint8_t flag = 0;

    while (flag != 2) {
        GetADC0(); //播放动画是可以同时初始化软件滤波

        ClearOLEDBuffer();

        switch (flag) {
        case 0:
            if (i < 80)
                i += 0.3 * i;
            else
                flag++;
            break;
        case 1:
            if (i > 64)
                i -= 0.05 * i;
            else
                flag++;
            break;
        }

        rate = i / 128.0;
        w = 170 * rate;
        x = (128 - w) / 2;
        y = (64 - i - 1) / 2;
        Draw_Slow_Bitmap_Resize(x, y, Logo2, 170, 128, w, i);
        // Draw_Slow_Bitmap_Resize(x, y, Logo_RoboBrave, 128, 128, w, i);
        Display();
    }

    for (int16_t xx = -128; xx < 128; xx += 12) {
        GetADC0(); //播放动画是可以同时初始化软件滤波

        ClearOLEDBuffer();
        //绘制Logo
        Disp.setDrawColor(1);
        Draw_Slow_Bitmap_Resize(x, y, Logo2, 170, 128, w, i);
        // Draw_Slow_Bitmap_Resize(x, y, Logo_RoboBrave, 128, 128, w, i);
        //转场特效
        Disp.setBitmapMode(1);
        Disp.setDrawColor(0);

        Disp.drawXBM(xx, 0, 128, 64, TranAnimation2);
        if (xx > 0)
            Disp.drawBox(0, 0, xx, 64);

        Disp.setBitmapMode(0);
        Display();
    }
    Disp.setDrawColor(1);
#endif
}

/**
 * @brief 清屏
 *
 */
void ClearOLEDBuffer(void)
{
    u8g2_ClearBuffer(&u8g2);
}

void Display(void)
{
    // PlaySoundLoop();

    // OLED_ScreenshotPrint();

    u8g2_SendBuffer(&u8g2);

    vTaskDelay(1);
}

/**
 * @brief 让指定的区域 变淡处理
 *
 * @param sx OLED左上角坐标
 * @param sy OLED左上角坐标
 * @param ex OLED右下角坐标
 * @param ey OLED右下角坐标
 * @param f 背景变淡系数
 * @param delayMs 延时时间
 */
void Blur(int sx, int sy, int ex, int ey, int f, int delayMs)
{
    for (int i = 0; i < f; i++) {
        for (int y = 0; y < ey; y++) {
            for (int x = 0; x < ex; x++) {
                if (x % 2 == y % 2 && x % 2 == 0 && x >= sx && x <= ex && y >= sy && y <= ey) {
                    u8g2_DrawPixel(&u8g2, x + (i > 0 && i < 3), y + (i > 1));
                }
                // else {u8g2_DrawPixel(&u8g2, x + (i > 0 && i < 3), y + (i > 1), 0);}
            }
        }

        if (delayMs) {
            u8g2_SendBuffer(&u8g2);
            delay(delayMs);
        }
    }
}

/**
 * @brief 绘制右边滚动条
 *
 * @param x 起始坐标
 * @param y 起始坐标
 * @param w 总宽度
 * @param h 总高度
 * @param iTotal 一共有多少格子
 * @param iCurPos 当前第几个格子
 */
void DrawScrollBar(int x, int y, int w, int h, int iTotal, int iCurPos)
{
    u8g2_SetDrawColor(&u8g2, 1);

    if (w < h) {
        // 绘制垂直格子

        // 在滚动条中间绘制一条竖线
        u8g2_DrawVLine(&u8g2, x + (w / 2), y, h);

        if (iTotal < h && h / iTotal >= 4) {
            // 大于等于4个格子 才绘制横线
            float xOffset = w / (float)iTotal;
            float yOffset = h / (float)iTotal;
            u8g2_uint_t len = w / 2 + 1;

            for (int i = 0; i < iTotal + 1; i++) {
                if (i % 2)
                    u8g2_DrawHLine(&u8g2, x + xOffset, y + yOffset * i, len); // 半条横线
                else
                    u8g2_DrawHLine(&u8g2, x, y + yOffset * i, w); // 整条横线
            }
        }

        if (iTotal > h) {
            iTotal = h;
        }
        // 当前在第几个格子位置 高亮
        u8g2_DrawBox(&u8g2, x, iCurPos, w, h / iTotal);

    } else {
        // 绘制水平格子
        u8g2_DrawHLine(&u8g2, x, y + (h / 2), w);

        if (iTotal < h && h / iTotal >= 4) {
            float xOffset = w / (float)iTotal;
            float yOffset = h / (float)iTotal;
            u8g2_uint_t len = h / 2 + 1;
            for (int i = 0; i < iTotal + 1; i++) {
                if (i % 2)
                    u8g2_DrawVLine(&u8g2, x + xOffset * i, y + yOffset, len);
                else
                    u8g2_DrawVLine(&u8g2, x + xOffset * i, y, h);
            }
        }

        if (iTotal > w) {
            iTotal = w;
        }
        u8g2_DrawBox(&u8g2, iCurPos, y, w / iTotal, w);
    }
}

/**
 * @brief 绘制数字条
 *
 * @param i 当前进度值
 * @param a
 * @param b
 * @param x
 * @param y
 * @param w
 * @param h
 * @param c
 */
void Draw_Num_Bar(float i, float a, float b, int x, int y, int w, int h, int c)
{
    // 当前进度
    char buffer[20];
    sprintf(buffer, "%.2f", i);
    uint8_t textWidth = u8g2_GetUTF8Width(&u8g2, buffer) + 3;

    u8g2_SetDrawColor(&u8g2, c);
    u8g2_DrawFrame(&u8g2, x, y, w - textWidth - 2, h);
    u8g2_DrawBox(&u8g2, x + 2, y + 2, map(i, a, b, 0, w - textWidth - 6), h - 4);

    u8g2_DrawStr(&u8g2, x + w - textWidth, y - 1, buffer);

    // 进行去棱角操作:增强文字视觉焦点
    u8g2_SetDrawColor(&u8g2, 0);
    u8g2_SetDrawColor(&u8g2, 1);
}

void Draw_Pixel_Resize(int x, int y, int ox, int oy, int w, int h)
{
    int xi = x - ox;
    int yi = y - oy;

    u8g2_DrawBox(&u8g2, ox + xi * w, oy + yi * h, w, h);
}

#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const unsigned char*)(addr))
#endif // pgm_read_byte

void Draw_Slow_Bitmap(int x, int y, const unsigned char* bitmap, unsigned char w, unsigned char h)
{
    uint8_t color = u8g2_GetDrawColor(&u8g2);
    int xi, yi, intWidth = (w + 7) / 8;
    for (yi = 0; yi < h; yi++) {
        for (xi = 0; xi < w; xi++) {
            if (pgm_read_byte(bitmap + yi * intWidth + xi / 8) & (128 >> (xi & 7))) {
                u8g2_DrawPixel(&u8g2, x + xi, y + yi);

            } else if (color != 2) {
                u8g2_SetDrawColor(&u8g2, 0);
                u8g2_DrawPixel(&u8g2, x + xi, y + yi);
                u8g2_SetDrawColor(&u8g2, color);
            }
        }
    }
}

/**
 * @brief 位图缩放 代码片段改自arduboy2
 *
 * @param x
 * @param y
 * @param bitmap
 * @param w1
 * @param h1
 * @param w2
 * @param h2
 */
void Draw_Slow_Bitmap_Resize(int x, int y, uint8_t* bitmap, int w1, int h1, int w2, int h2)
{
    uint8_t color = u8g2_GetDrawColor(&u8g2);
    float mw = (float)w2 / w1;
    float mh = (float)h2 / h1;
    uint8_t cmw = ceil(mw);
    uint8_t cmh = ceil(mh);
    int xi, yi, byteWidth = (w1 + 7) / 8;

    for (yi = 0; yi < h1; yi++) {
        for (xi = 0; xi < w1; xi++) {
            if (pgm_read_byte(bitmap + yi * byteWidth + xi / 8) & (1 << (7 - (xi & 7)))) {
                u8g2_DrawBox(&u8g2, x + xi * mw, y + yi * mh, cmw, cmh);
            } else if (color != 2) {
                u8g2_SetDrawColor(&u8g2, 0);
                u8g2_DrawBox(&u8g2, x + xi * mw, y + yi * mh, cmw, cmh);
                u8g2_SetDrawColor(&u8g2, color);
            }
        }
    }
}

//绘制屏保-密集运算线条
void DrawIntensiveComputingLine(void)
{
    static uint8_t Line[4];
    for (uint8_t a = 0; a < 4; a++) {
        Line[a] += rand() % 2 - 1;
        if (Line[a] > 128)
            Line[a] -= 128;
        for (uint8_t b = 0; b < rand() % 3 + 3; b++) {
            u8g2_DrawHLine(&u8g2, 0, Line[a] + rand() % 20 - 10, 128); //水平线
            u8g2_DrawVLine(&u8g2, Line[a] + rand() % 20 - 10, 0, 64); //垂直线
        }
    }
}

// FP 密集运算屏保
void DrawIntensiveComputing(void)
{
    float calculate;

    //随机线条
    DrawIntensiveComputingLine();

    calculate = sin(xTaskGetTickCount() / 4000.0);
    //模拟噪点
    for (int i = 0; i < calculate * 256 + 256; i++)
        u8g2_DrawPixel(&u8g2, rand() % 128, rand() % 64);

    //波浪警告声
    // SetTone(64 + calculate * 64 + rand() % 16 - 8);
    // SetTone(1500 + calculate * 500 + rand() % 64 - 32 - (((xTaskGetTickCount() / 1000) % 2 == 1) ? 440 : 0));
}

/***
 * @description: 在屏幕中心绘制文本
 * @param {*}
 * @return {*}
 */
void DrawMsgBox(const char* s)
{
    int w = Get_UTF8_Ascii_Pix_Len(1, s) + 2;
    int h = 12;
    int x = (OLED_SCREEN_WIDTH - w) / 2;
    int y = (OLED_SCREEN_HEIGHT - h) / 2;

    u8g2_SetDrawColor(&u8g2, 0);

    u8g2_SetDrawColor(&u8g2, 0);
    Blur(0, 0, OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, 3, 0);
    u8g2_DrawFrame(&u8g2, x - 1, y - 3, w + 1, h + 3);
    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_DrawRBox(&u8g2, x, y - 2, w, h + 2, 2);
    u8g2_SetDrawColor(&u8g2, 0);
    u8g2_DrawUTF8(&u8g2, x + 1, y, s);
    u8g2_SetDrawColor(&u8g2, 1);
}
/***
 * @description: 绘制高亮文本
 * @param {int} x
 * @param {int} y
 * @param {char} *s
 * @return {*}
 */
void DrawHighLightText(int x, int y, const char* s)
{
    int TextWidth = u8g2_GetUTF8Width(&u8g2, s);
    int TextHigh = u8g2_GetMaxCharHeight(&u8g2);
    uint8_t color = u8g2_GetDrawColor(&u8g2);

    if (color == 2) {
        u8g2_DrawUTF8(&u8g2, x + 1, y + 2, s);
        u8g2_DrawRBox(&u8g2, x, y, TextWidth + 2, TextHigh, 3);
    } else {
        u8g2_DrawRBox(&u8g2, x, y, TextWidth + 2, TextHigh, 3);
        u8g2_SetDrawColor(&u8g2, !color);
        u8g2_DrawUTF8(&u8g2, x + 1, y + 2, s);
        u8g2_SetDrawColor(&u8g2, color);
    }
}

/***
 * @description: 绘制温度状态条
 * @param bool color 颜色
 * @return {*}
 */
void DrawStatusBar(bool color)
{
    u8g2_SetDrawColor(&u8g2, color);
    //温度条
    //框
    u8g2_DrawFrame(&u8g2, 0, 53, 103, 11);
    //条
    if (TipTemperature <= HeatMaxTemp)
        u8g2_DrawBox(&u8g2, 0, 53, map(TipTemperature, HeatMinTemp, HeatMaxTemp, 5, 98), 11);

    //功率条
    u8g2_DrawFrame(&u8g2, 104, 53, 23, 11);
    u8g2_DrawBox(&u8g2, 104, 53, map(POWER, 0, pow(2, PWM_Resolution) - 1, 0, 23), 11);

    u8g2_DrawHLine(&u8g2, 117, 51, 11);
    u8g2_DrawPixel(&u8g2, 103, 52);
    u8g2_DrawPixel(&u8g2, 127, 52);

    //////////////进入反色////////////////////////////////
    u8g2_SetDrawColor(&u8g2, 2);

    //画指示针
    // Draw_Slow_Bitmap(map(PID_Setpoint, HeatMinTemp, HeatMaxTemp, 5, 98) - 4, 54, PositioningCursor, 8, 8);

    char buf[128];
    // sprintf("%.0f", PID_Setpoint);
    u8g2_DrawUTF8(&u8g2, 2, 53, buf);

    // 显示输出功率 百分比
    // sprintf("%d%%", map(POWER, 0, pow(2, PWM_Resolution) - 1, 0, 100));
    u8g2_DrawUTF8(&u8g2, 105, 53, buf);

    //显示真实功率
    // u8g2_Printf(&u8g2, "%.0fW", SYS_Voltage * SYS_Current);

    // arduboy.setCursor(105, 55); arduboy.print(map(PID_Output, 255, 0, 0, 100)); arduboy.print(F("%")); //功率百分比
    u8g2_SetDrawColor(&u8g2, color);
}

/**
 * @brief 短文本编辑器
 *
 * @param title 文本标题
 * @param text 要修改的文本
 */
void TextEditor(const char* title, char* text, int32_t textSize)
{
    char newText[20] = { 0 }; // 最多20个格子
    strncpy(newText, text, sizeof(newText));

    uint8_t charCounter = 0; // 光标指针
    char editChar = 'A';

    bool exitRenameGUI = false;
    bool editFlag = 0, // 编辑器状态：0:选择要编辑的字符    1:选择ASCII
        lastEditFlag = 1;

    while (!exitRenameGUI) {
        // 设置编码器
        if (editFlag != lastEditFlag) {
            if (editFlag == 0) {
                // 选择位置
                RotarySet(0.f, 19.f, 1.f, (float)charCounter);
            } else {
                // 选择ASCII
                RotarySet(0.f, 255.f, 1.f, (float)newText[charCounter]);
            }

            lastEditFlag = editFlag;
        }

        // 获取编码器输入
        switch (editFlag) {
        case 0:
            // 左右选择位置
            charCounter = (char)GetRotaryPositon();
            break;

        case 1:
            // 选择ASCII
            editChar = (char)GetRotaryPositon();
            newText[charCounter] = editChar;
            break;
        }

        // 清屏
        ClearOLEDBuffer();

        u8g2_SetDrawColor(&u8g2, 1);

        // 第一行显示标题
        u8g2_DrawUTF8(&u8g2, 0, 1, title);

        // 第二行显示编辑文本
        u8g2_SetFont(&u8g2, u8g2_font_6x12_tf);
        u8g2_DrawUTF8(&u8g2, 0, 12 + 1, newText);

        // 显示当前选中的ASCII
        u8g2_SetDrawColor(&u8g2, 1);
        u8g2_SetFont(&u8g2, u8g2_font_logisoso26_tf);

        // 文本编辑器
        char str[5] = { 0 };
        str[0] = newText[charCounter];
        u8g2_DrawUTF8(&u8g2, 0, 34, str);

        // 文本编辑器 显示十六进制 值
        sprintf(str, "0X%02X", newText[charCounter]);
        u8g2_DrawUTF8(&u8g2, 32, 34, str);

        u8g2_SetFont(&u8g2, u8g2_font_wqy12_t_gb2312);

        // 反色显示光标
        u8g2_SetDrawColor(&u8g2, 2);
        if (editFlag) {
            // 编辑状态下 选择字符时 光标闪烁
            if ((xTaskGetTickCount() / 50) % 2) {
                u8g2_DrawBox(&u8g2, charCounter * 6, 12, 6, 12);
            }

        } else {
            // 不是编辑状态
            u8g2_DrawBox(&u8g2, charCounter * 6, 12, 6, 12);
        }

        // 字符选择区反色高亮
        if (editFlag) {
            u8g2_DrawBox(&u8g2, 0, 32, 32, 32);
        }

        Display();

        //处理按键事件
        ROTARY_BUTTON_TYPE rotaryButton = getRotaryButton();
        switch (rotaryButton) {
        case BUTTON_CLICK:
            // 单击切换编辑器状态
            editFlag = !editFlag;
            break;

        case BUTTON_LONGCLICK:
        case BUTTON_DOUBLECLICK:
            // 保存并退出
            strncpy(text, newText, textSize);
            exitRenameGUI = true;
            break;

        case BUTTON_NULL:
        default:
            break;
        }
    }

    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_SetFont(&u8g2, u8g2_font_wqy12_t_gb2312);
    Display();
}

/**
 * @brief 获得字符串的宽度
 *
 * @param size
 * @param s
 * @return uint32_t
 */
uint32_t Get_UTF8_Ascii_Pix_Len(uint8_t size, const char* s)
{
    return u8g2_GetUTF8Width(&u8g2, s);
}

/*
    @brief 自适应屏幕右下角角标绘制
    @param int curPage 当前页
    @param int TotalPage 所有页
*/
void DrawPageFootnotes(int curPage, int TotalPage)
{
    char buffer[20];
    const TickType_t destTime = 1000 / portTICK_PERIOD_MS;
    const uint8_t w = (GetNumberLength(curPage) + GetNumberLength(TotalPage) + 3) * 6;
    const uint8_t x = OLED_SCREEN_WIDTH - 8 - w;
    const uint8_t y = OLED_SCREEN_HEIGHT - 12;

    if (xTaskGetTickCount() < g_ShowPageNumTime + destTime) {
        //绘制白色底色块
        u8g2_SetDrawColor(&u8g2, 1);
        u8g2_DrawRBox(&u8g2, x + 1, y - 1, w, 13, 1);

        //绘制下标文字
        u8g2_SetDrawColor(&u8g2, 0);
        sprintf(buffer, "[%d/%d]", curPage, TotalPage);
        u8g2_DrawUTF8(&u8g2, x, y + 1, buffer);
    }

    // 恢复颜色设置
    u8g2_SetDrawColor(&u8g2, 1);
}

/**
 * @brief 绘制图片
 *
 * @param x 坐标X
 * @param y 坐标Y
 * @param bitmap 绘制图片
 */
void Draw_APP(int x, int y, uint8_t* bitmap)
{
    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_DrawRBox(&u8g2, x - 3, y - 3, 42 + 6, 42 + 6, 4);
    u8g2_SetDrawColor(&u8g2, 0);
    u8g2_SetDrawColor(&u8g2, 1);

    Draw_Slow_Bitmap_Resize(x, y, bitmap + 1, bitmap[0], bitmap[0], 42, 42);
}

/**
 * @brief 绘制温度曲线
 *
 */
void DrawTempCurve(void)
{
    int y;
    CalculateTemp(0, HeatingConfig.curConfig.PTemp);

    int countStep = BoostTime / 127 + 1;

    RotarySet(0, 127, 1, 0);

    while (BUTTON_NULL == getRotaryButton()) {
        // 得到编码器位置
        float rotaryPositon = GetRotaryPositon();

        ClearOLEDBuffer();

        // 绘制参考文字
        char buffer[64];
        sprintf(buffer, "时间 %dm%ds", (int)(rotaryPositon * countStep) / 60, (int)(rotaryPositon * countStep) % 60);
        DrawHighLightText(128 - u8g2_GetUTF8Width(&u8g2, buffer) - 2, 36, buffer);

        sprintf(buffer, "温度 %.1f", CalculateTemp(rotaryPositon * countStep, HeatingConfig.curConfig.PTemp));
        DrawHighLightText(128 - u8g2_GetUTF8Width(&u8g2, buffer) - 2, 51, buffer);

        // 绘制曲线
        u8g2_SetDrawColor(&u8g2, 2);
        for (int x = 0; x < 128; x++) {
            y = map(CalculateTemp(x * countStep, HeatingConfig.curConfig.PTemp), 0, (HeatingConfig.curConfig.PTemp[4] > HeatingConfig.curConfig.PTemp[1] ? HeatingConfig.curConfig.PTemp[4] : HeatingConfig.curConfig.PTemp[1]) + 1, 0, 63);
            u8g2_DrawPixel(&u8g2, x, 63 - y);

            // 画指示针
            if (x == rotaryPositon)
                Draw_Slow_Bitmap(x - 4, 63 - y - 4, PositioningCursor, 8, 8);
        }

        u8g2_SetDrawColor(&u8g2, 1);

        // 绘制底部点点
        for (int yy = 0; yy < 64; yy += 8) {
            for (int xx = 0; xx < 128; xx += 8) {
                u8g2_DrawPixel(&u8g2, xx + 2, yy + 4);
            }
        }

        Display();
    }

    u8g2_SetDrawColor(&u8g2, 1);
}

/**
 * @brief 弹窗显示
 *
 * @param s
 */
void PopWindows(const char* s)
{
    int w = Get_UTF8_Ascii_Pix_Len(1, s) + 2;
    int h = 12;
    const int x = (OLED_SCREEN_WIDTH - w) / 2;
    const int y = (OLED_SCREEN_HEIGHT - h) / 2;

    u8g2_SetDrawColor(&u8g2, 0);
    Blur(0, 0, OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, 3, 66 * *SwitchControls[SwitchSpace_SmoothAnimation]); //<=15FPS以便人眼察觉细节变化

    int ix = 0;
    for (int i = 1; i <= 10; i++) {
        //震荡动画
        if (*SwitchControls[SwitchSpace_SmoothAnimation])
            ix = (10 * cos((i * 3.14) / 2.0)) / i;

        u8g2_SetDrawColor(&u8g2, 0);
        Blur(0, 0, OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, 3, 0);
        u8g2_DrawFrame(&u8g2, x - 1 + ix, y - 3, w + 1, h + 3);
        u8g2_SetDrawColor(&u8g2, 1);
        u8g2_DrawRBox(&u8g2, x + ix, y - 2, w, h + 2, 2);
        u8g2_SetDrawColor(&u8g2, 0);
        u8g2_DrawUTF8(&u8g2, x + 1 + ix, y, s);
        u8g2_SetDrawColor(&u8g2, 1);

        Display();
        delay(20 * *SwitchControls[SwitchSpace_SmoothAnimation]);
    }
}
