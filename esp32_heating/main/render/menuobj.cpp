#include "heating.h"
#include "bitmap.h"
#include <string.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 菜单相关变量

_SystemMenuSaveData SystemMenuSaveData = {
    Language : 0, // 系统语言
    BLEState : 0, // 蓝牙开关
    ScreenFlip : 0, // 屏幕翻转
    MenuListMode : 0, // true:菜单使用文本渲染方式 不使用图标、 false:混合显示模式
    SmoothAnimationFlag : 1, // 菜单动画标志位
    Volume : 1, // 编码器声音设置
    PanelSettings : PANELSET_Simple, // 首页设置 详细面板 简约面板
    OptionStripFixedLength_Flag : 0, // 选项条固定 自适应

    ScreenProtectorTime : 60.0f, // 屏保在休眠后的触发时间 (秒)
    ScreenBrightness : 128.0f, // 屏幕亮度
    UndervoltageAlert : 18.0f, // 系统电压 欠压警告阈值 (单位V)

    BLEName : { 'H', 'e', 'a', 't', 'i', 'n', 'g' }, // 蓝牙设备名称
    BootPasswd : {}, // 开机密码
};

static float MenuScroll = 0; // 当前菜单的位置

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ExitMenuSystem(void);
void BLE_Restart(void);
void BLE_Rename(void);
void SetPasswd(void);
void About(void);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 开关控件
 *
 */
uint8_t* SwitchControls[] = {
    &SystemMenuSaveData.Language, // 语言设置
    &SystemMenuSaveData.BLEState, // 蓝牙开关
    &SystemMenuSaveData.ScreenFlip, // 翻转OLED
    &SystemMenuSaveData.MenuListMode, // 图标模式 列表模式
    &SystemMenuSaveData.SmoothAnimationFlag, // 过度动画标志位
    &SystemMenuSaveData.Volume, // 声音设置
    &SystemMenuSaveData.PanelSettings, // 首页设置 详细面板 简约面板
    &SystemMenuSaveData.OptionStripFixedLength_Flag, // 选项条固定 自适应
    // &RotaryDirection, // 编码器反向

    ///////////////////////////////////
    (uint8_t*)&HeatingConfig.curConfigIndex, // 当前加热台配置索引
    (uint8_t*)&HeatingConfig.curConfig.type, // 恒温加热台 回流焊加热台 T12加热台

    ///////////////////////////////////
    &HandleTrigger, // T12 震动开关检测配置

    ///////////////////////////////////
    (uint8_t*)&KalmanInfo[adc_HeatingTemp].parm.UseKalman, // 卡尔曼滤波开关
    (uint8_t*)&KalmanInfo[adc_T12Temp].parm.UseKalman, // 卡尔曼滤波开关
    (uint8_t*)&KalmanInfo[adc_T12Cur].parm.UseKalman, // 卡尔曼滤波开关
    (uint8_t*)&KalmanInfo[adc_T12NTC].parm.UseKalman, // 卡尔曼滤波开关
    (uint8_t*)&KalmanInfo[adc_SystemVol].parm.UseKalman, // 卡尔曼滤波开关
    (uint8_t*)&KalmanInfo[adc_SystemRef].parm.UseKalman, // 卡尔曼滤波开关
    (uint8_t*)&KalmanInfo[adc_RoomTemp].parm.UseKalman, // 卡尔曼滤波开关
};

/**
 * @brief 滑动控件
 *
 */
struct SlideBar SlideControls[] = {
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 系统配置
    { (float*)&SystemMenuSaveData.ScreenBrightness, 0, 255, 5 }, // 屏幕亮度
    { (float*)&SystemMenuSaveData.UndervoltageAlert, 0, 36, 0.25 }, // 欠压提醒(V)
    { (float*)&MenuScroll, 0, OLED_SCREEN_HEIGHT / 16, 1 }, // 当前菜单显示的位置（每页只显示4个，所以范围在0~3） 高度为16像素
    { (float*)&SystemMenuSaveData.ScreenProtectorTime, 0, 600, 1 }, // 屏保在休眠后的触发时间 (秒)

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 加热台 T12 配置
    { (float*)&HeatingConfig.curConfig.targetTemp, 0, 0, 1 }, // 恒温模式目标输出温度(°C)

    // 温度曲线
    { (float*)&HeatingConfig.curConfig.PTemp[WarmUpRampRate], 0.5, 5, 0.5 }, // 升温斜率
    { (float*)&HeatingConfig.curConfig.PTemp[WarmUpTemp], 0, 0, 5 }, // 预热区温度(摄氏度)
    { (float*)&HeatingConfig.curConfig.PTemp[WarmUpTime], 0, 600, 5 }, // 预热区时间(秒)
    { (float*)&HeatingConfig.curConfig.PTemp[RefloatRampRate], 0.5, 5, 0.5 }, // 升温斜率
    { (float*)&HeatingConfig.curConfig.PTemp[RefloatTemp], 0, 0, 5 }, // 回流区温度(摄氏度)
    { (float*)&HeatingConfig.curConfig.PTemp[RefloatTime], 0, 600, 5 }, // 回流区时间(秒)
    { (float*)&HeatingConfig.curConfig.PTemp[CoolDownTime], 0.5, 5, 0.5 }, // 降温斜率

    // PID参数
    { (float*)&HeatingConfig.curConfig.PIDSample, 0, 500, 1 }, // PID采样时间
    { (float*)&HeatingConfig.curConfig.PIDTemp, 0, 500, 1 }, // PID温度切换度数
    { (float*)&HeatingConfig.curConfig.PID[0][0], 0, 10, 0.01 }, // 比例P(爬升期)
    { (float*)&HeatingConfig.curConfig.PID[0][1], 0, 10, 0.01 }, // I
    { (float*)&HeatingConfig.curConfig.PID[0][2], 0, 10, 0.01 }, // D
    { (float*)&HeatingConfig.curConfig.PID[1][0], 0, 10, 0.01 }, // 比例P(接近期)
    { (float*)&HeatingConfig.curConfig.PID[1][1], 0, 10, 0.01 }, // I
    { (float*)&HeatingConfig.curConfig.PID[1][2], 0, 10, 0.01 }, // D

    // 卡尔曼滤波设置
    { (float*)&KalmanInfo[adc_HeatingTemp].parm.Cycle, 180, 1000, 1 }, { (float*)&KalmanInfo[adc_HeatingTemp].parm.calibrationVal, 0, 100, 1 }, { (float*)&KalmanInfo[adc_HeatingTemp].parm.KalmanQ, 0, 10, 0.01 }, { (float*)&KalmanInfo[adc_HeatingTemp].parm.KalmanR, 0, 10, 0.01 },
    { (float*)&KalmanInfo[adc_T12Temp].parm.Cycle, 10, 1000, 1 }, { (float*)&KalmanInfo[adc_T12Temp].parm.calibrationVal, 0, 100, 1 }, { (float*)&KalmanInfo[adc_T12Temp].parm.KalmanQ, 0, 10, 0.01 }, { (float*)&KalmanInfo[adc_T12Temp].parm.KalmanR, 0, 10, 0.01 },
    { (float*)&KalmanInfo[adc_T12Cur].parm.Cycle, 10, 1000, 1 }, { (float*)&KalmanInfo[adc_T12Cur].parm.calibrationVal, 0, 100, 1 }, { (float*)&KalmanInfo[adc_T12Cur].parm.KalmanQ, 0, 10, 0.01 }, { (float*)&KalmanInfo[adc_T12Cur].parm.KalmanR, 0, 10, 0.01 },
    { (float*)&KalmanInfo[adc_T12NTC].parm.Cycle, 10, 1000, 1 }, { (float*)&KalmanInfo[adc_T12NTC].parm.calibrationVal, 0, 100, 1 }, { (float*)&KalmanInfo[adc_T12NTC].parm.KalmanQ, 0, 10, 0.01 }, { (float*)&KalmanInfo[adc_T12NTC].parm.KalmanR, 0, 10, 0.01 },
    { (float*)&KalmanInfo[adc_SystemVol].parm.Cycle, 10, 1000, 1 }, { (float*)&KalmanInfo[adc_SystemVol].parm.calibrationVal, 0, 100, 1 }, { (float*)&KalmanInfo[adc_SystemVol].parm.KalmanQ, 0, 10, 0.01 }, { (float*)&KalmanInfo[adc_SystemVol].parm.KalmanR, 0, 10, 0.01 },
    { (float*)&KalmanInfo[adc_SystemRef].parm.Cycle, 10, 1000, 1 }, { (float*)&KalmanInfo[adc_SystemRef].parm.calibrationVal, 0, 100, 1 }, { (float*)&KalmanInfo[adc_SystemRef].parm.KalmanQ, 0, 10, 0.01 }, { (float*)&KalmanInfo[adc_SystemRef].parm.KalmanR, 0, 10, 0.01 },
    { (float*)&KalmanInfo[adc_RoomTemp].parm.Cycle, 10, 1000, 1 }, { (float*)&KalmanInfo[adc_RoomTemp].parm.calibrationVal, 0, 100, 1 }, { (float*)&KalmanInfo[adc_RoomTemp].parm.KalmanQ, 0, 10, 0.01 }, { (float*)&KalmanInfo[adc_RoomTemp].parm.KalmanR, 0, 10, 0.01 },

    // 最小 最大温度值
    { (float*)&HeatingConfig.systemConfig.HeatMinTemp, 0, 350, 1 }, // 加热台最小温度
    { (float*)&HeatingConfig.systemConfig.HeatMaxTemp, 0, 350, 1 }, // 加热台最大温度
    { (float*)&HeatingConfig.systemConfig.T12MinTemp, 0, 350, 1 }, // T12最小温度
    { (float*)&HeatingConfig.systemConfig.T12MaxTemp, 0, 350, 1 }, // T12最大温度
    { (float*)&HeatingConfig.systemConfig.T12IdleTime, 0, 600, 1 }, // T12降温时间(s)
    { (float*)&HeatingConfig.systemConfig.T12IdleTemp, 0, 300, 1 }, // T12降温温度
    { (float*)&HeatingConfig.systemConfig.T12StopTime, 0, 600, 1 }, // T12停机时间(s)
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 菜单动画结构体
 * 不要将值设置太大
 */
struct MenuSmoothAnimation menuSmoothAnimation[] = {
    { 0, 0, 0, 0.4, 1 }, // 0 菜单项目滚动动画
    { 0, 0, 0, 0.15, 1 }, // 1 垂直滚动Y位置计算
    { 0, 0, 0, 0.15, 1 }, // 2 垂直滚动W宽度计算
    { 0, 0, 0, 0.2, 0 }, // 3 项目归位动画
};
const int sizeMenuSmoothAnimatioin = sizeof(menuSmoothAnimation) / sizeof(menuSmoothAnimation[0]);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 重启系统
 *
 */
void SYSReboot(void)
{
    settings_write_all();
    printf("Restarting now.\n");
    fflush(stdout);
    delay(100);
    esp_restart();
}

/**
 * @brief 更新屏幕亮度
 *
 */
void updateOledLight(void)
{
    Update_OLED_Light_Level((uint8_t)SystemMenuSaveData.ScreenBrightness);
}

/**
 * @brief 是否镜像显示
 *
 */
void updateOledFlip(void)
{
    Update_OLED_Flip(SystemMenuSaveData.ScreenFlip);
}

/**
 * @brief 蓝牙重启
 *
 */
void BLE_Restart(void)
{
    printf("BLE_Restart %d\r\n", SystemMenuSaveData.BLEState);
}

/**
 * @brief 重命名蓝牙名称
 *
 */
void BLE_Rename(void)
{
    TextEditor("蓝牙设备名", SystemMenuSaveData.BLEName, sizeof(SystemMenuSaveData.BLEName));
}

/**
 * @brief 输入密码
 *
 * @return int8_t 0:无密码 1:密码输入正确 2:密码输入错误
 */
int8_t EnterPasswd(void)
{
    // 空密码无需授权
    if (strlen(SystemMenuSaveData.BootPasswd) == 0)
        return 0;

    char passwdBuffer[20] = { 0 };
    TextEditor("[需要授权]", passwdBuffer, sizeof(passwdBuffer));

    if (0 == strcmp(passwdBuffer, SystemMenuSaveData.BootPasswd))
        return 1;
    return 2;
}

/**
 * @brief 设置密码
 *
 */
void SetPasswd(void)
{
    if (2 == EnterPasswd()) {
        PopWindows("密码错误 请重试");
        return;
    }

    char passwdBuffer[2][20] = { 0 };
    TextEditor("[设置新密码]", passwdBuffer[0], sizeof(passwdBuffer[0]));
    TextEditor("[确认密码]", passwdBuffer[1], sizeof(passwdBuffer[1]));

    if (strcmp(passwdBuffer[0], passwdBuffer[1])) {
        PopWindows("两次密码不一致");
        return;
    }

    strncpy(SystemMenuSaveData.BootPasswd, passwdBuffer[0], sizeof(SystemMenuSaveData.BootPasswd));
    PopWindows("密码已更新");
}

/**
 * @brief 关于本系统
 *
 */
void About(void)
{
    ClearOLEDBuffer();

    u8g2_DrawUTF8(&u8g2, 0, 0, "Hello World");

    while (BUTTON_NULL == getRotaryButton()) {
        Display();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static std::vector<menuSystem> menuInfo = {
    // 菜单入口
    { 0, 0, 0, 0, MenuRenderText, {
                                      { Type_MenuName, "[设置]", NULL, 0, 0, *ExitMenuSystem },
                                      { Type_GotoMenu, "温度设置", NULL, 200, 0, NULL },
                                      { Type_GotoMenu, "系统设置", NULL, 100, 0, NULL },
                                      { Type_RunFunction, "返回", NULL, 0, 0, *ExitMenuSystem },
                                      { Type_RunFunction, "重启系统", NULL, 0, 0, *SYSReboot },
                                  } },

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 系统设置部分
    { 100, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "系统设置", NULL, 0, 2, NULL },
                                         { Type_GotoMenu, "个性化", IMG_Pen, 101, 0, NULL },
                                         { Type_GotoMenu, "蓝牙", IMG_BLE, 102, 0, NULL },
                                         { Type_GotoMenu, "电压设置", Set6, 104, 0, NULL },
                                         { Type_RunFunction, "开机密码", Lock, 0, 0, *SetPasswd },
                                         { Type_GotoMenu, "语言设置", Set_LANG, 103, 0, NULL },
                                         { Type_RunFunction, "关于系统", QRC, 5, 5, *About },
                                         { Type_ReturnMenu, "返回", Set7, 0, 2, NULL },
                                     } },

    { 101, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "个性化", NULL, 100, 1, NULL },
                                         { Type_GotoMenu, "显示效果", Set4, 110, 0, NULL },
                                         { Type_GotoMenu, "声音设置", Set5, 111, 0, NULL },
                                         // { Type_Switch, "编码器方向", Set19, SwitchComponents_RotaryDirection, 0, *PopMsg_RotaryDirection },
                                         { Type_ReturnMenu, "返回", Set7, 100, 1, NULL },
                                     } },

    { 102, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "蓝牙", NULL, 100, 2, NULL },
                                        { Type_Switch, "状态", NULL, SwitchComponents_BLE_State, 0, *BLE_Restart },
                                        { Type_RunFunction, "设备名称", NULL, 22, 2, *BLE_Rename },
                                        { Type_ReturnMenu, "返回", NULL, 100, 2, NULL },
                                    } },

    { 103, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "语言设置", NULL, 100, 5, NULL },
                                         { Type_CheckBox, "简体中文", Lang_CN, SwitchComponents_Language, 0, *JumpWithTitle },
                                     } },

    { 104, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "电压设置", NULL, 100, 3, NULL },
                                         { Type_Slider, "欠压提醒(V)", Set6, SlideComponents_UndervoltageAlert, 0, NULL },
                                     } },
    /////////////////////////////////////////////////////
    // 三级菜单
    { 110, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "显示效果", NULL, 101, 1, NULL },
                                         { Type_GotoMenu, "首页设置", Set0, 112, 0, NULL },
                                         { Type_Switch, "翻转屏幕", IMG_Flip, SwitchComponents_ScreenFlip, 0, *updateOledFlip },
                                         { Type_GotoMenu, "过渡动画", IMG_Animation, 113, 0, NULL },
                                         { Type_Slider, "屏幕亮度", IMG_Sun, SlideComponents_ScreenBrightness, 1, *updateOledLight },
                                         { Type_GotoMenu, "选项条定宽", IMG_Size, 114, 0, NULL },
                                         { Type_Switch, "列表模式", IMG_ListMode, SwitchComponents_MenuListMode, 0, NULL },
                                         { Type_Slider, "屏保时间(s)", Set4, SlideComponents_ScreenProtectorTime, 0, NULL },
                                         { Type_ReturnMenu, "返回", Set7, 101, 1, NULL },
                                     } },

    { 111, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "声音设置", NULL, 101, 2, NULL },
                                         { Type_CheckBox, "关闭", Set5_1, SwitchComponents_Volume, 0, *JumpWithTitle },
                                         { Type_CheckBox, "开启", Set5, SwitchComponents_Volume, 1, *JumpWithTitle },
                                     } },

    { 112, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "首页设置", NULL, 110, 1, NULL },
                                         { Type_CheckBox, "详细", Set17, SwitchComponents_PanelSettings, 0, *JumpWithTitle },
                                         { Type_CheckBox, "精简", Set18, SwitchComponents_PanelSettings, 1, *JumpWithTitle },
                                         { Type_CheckBox, "曲线", SetTrend, SwitchComponents_PanelSettings, 2, *JumpWithTitle },
                                     } },

    { 113, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "过渡动画", NULL, 110, 1, NULL },
                                         { Type_CheckBox, "关闭", IMG_Animation_DISABLE, SwitchComponents_SmoothAnimation, 0, *JumpWithTitle },
                                         { Type_CheckBox, "开启", IMG_Animation, SwitchComponents_SmoothAnimation, 1, *JumpWithTitle },
                                     } },

    { 114, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "选项条定宽设置&测试", NULL, 110, 5, NULL },
                                        { Type_CheckBox, "自适应", NULL, SwitchComponents_OptionWidth, 0, NULL },
                                        { Type_CheckBox, "固定", NULL, SwitchComponents_OptionWidth, 1, NULL },

                                        { Type_GotoMenu, "--- 往下翻 ---", NULL, 9, 4, NULL },
                                        { Type_NULL, "你好!", NULL, 0, 0, NULL },
                                        { Type_NULL, "我是谁~", NULL, 0, 0, NULL },
                                        { Type_NULL, "我才哪里啊???", NULL, 0, 1, NULL },
                                        { Type_NULL, "动 力！", NULL, 0, 0, NULL },
                                        { Type_GotoMenu, "--- 往上翻 ---", NULL, 9, 0, NULL },
                                        { Type_ReturnMenu, "返回", NULL, 110, 5, NULL },
                                    } },

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 温度设置部分
    { 200, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "温度设置", NULL, 0, 1, NULL },
                                         { Type_GotoMenu, "加热台管理", IMG_Tip, 201, 0, *flushMaxTemp },
                                         { Type_GotoMenu, "卡尔曼滤波器", kalmanImg, 202, 0, NULL },
                                         { Type_GotoMenu, "温度管理", kalmanImg, 203, 0, NULL },
                                         { Type_ReturnMenu, "返回", Set7, 0, 1, NULL },
                                     } },

    { 201, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "加热台管理", NULL, 200, 1, NULL },
                                         { Type_GotoMenu, "切换配置", Set8, 210, 0, *flushHeatConfig },
                                         { Type_GotoMenu, "PID参数", pidImg, 212, 0, NULL },
                                         { Type_GotoMenu, "其他设置", Set3, 215, 0, NULL },
                                         { Type_GotoMenu, "设置回流焊曲线", Set9, 211, 0, NULL },
                                         { Type_RunFunction, "查看回流焊曲线", Set0, 0, 0, *DrawTempCurve },
                                         { Type_RunFunction, "新建配置", IMG_Files, 0, 0, *newHeatConfig },
                                         { Type_RunFunction, "重命名配置", IMG_Pen2, 0, 0, *renameHeatConfig },
                                         { Type_RunFunction, "删除配置", Set10, 0, 0, *delHeatConfig },
                                         { Type_ReturnMenu, "返回", Save, 200, 1, NULL },
                                     } },

    { 202, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "卡尔曼滤波器", NULL, 200, 2, NULL },

                                        // 加热台
                                        { Type_Switch, "加热台-状态", NULL, SwitchComponents_KFP1, 0, NULL },
                                        { Type_Slider, "采样周期(ms)", NULL, SlideComponents_ADC_Cycle1, 0, NULL },
                                        { Type_Slider, "温度补偿", NULL, SlideComponents_TempComp_1, 0, NULL },
                                        { Type_Slider, "过程噪声协方差", NULL, SlideComponents_KFP_Q1, 0, NULL },
                                        { Type_Slider, "观察噪声协方差", NULL, SlideComponents_KFP_R1, 0, NULL },
                                        { Type_MenuName, "----------", NULL, 200, 2, NULL },

                                        // T12
                                        { Type_Switch, "T12温度-状态", NULL, SwitchComponents_KFP2, 0, NULL },
                                        { Type_Slider, "采样周期(ms)", NULL, SlideComponents_ADC_Cycle2, 0, NULL },
                                        { Type_Slider, "温度补偿", NULL, SlideComponents_TempComp_2, 0, NULL },
                                        { Type_Slider, "过程噪声协方差", NULL, SlideComponents_KFP_Q2, 0, NULL },
                                        { Type_Slider, "观察噪声协方差", NULL, SlideComponents_KFP_R2, 0, NULL },
                                        { Type_MenuName, "----------", NULL, 200, 2, NULL },

                                        // T12
                                        { Type_Switch, "T12-状态", NULL, SwitchComponents_KFP3, 0, NULL },
                                        { Type_Slider, "采样周期(ms)", NULL, SlideComponents_ADC_Cycle3, 0, NULL },
                                        { Type_Slider, "温度补偿", NULL, SlideComponents_TempComp_3, 0, NULL },
                                        { Type_Slider, "过程噪声协方差", NULL, SlideComponents_KFP_Q3, 0, NULL },
                                        { Type_Slider, "观察噪声协方差", NULL, SlideComponents_KFP_R3, 0, NULL },
                                        { Type_MenuName, "----------", NULL, 200, 2, NULL },

                                        // T12
                                        { Type_Switch, "T12-状态", NULL, SwitchComponents_KFP4, 0, NULL },
                                        { Type_Slider, "采样周期(ms)", NULL, SlideComponents_ADC_Cycle4, 0, NULL },
                                        { Type_Slider, "温度补偿", NULL, SlideComponents_TempComp_4, 0, NULL },
                                        { Type_Slider, "过程噪声协方差", NULL, SlideComponents_KFP_Q4, 0, NULL },
                                        { Type_Slider, "观察噪声协方差", NULL, SlideComponents_KFP_R4, 0, NULL },
                                        { Type_MenuName, "----------", NULL, 200, 2, NULL },

                                        // 系统电压
                                        { Type_Switch, "系统电压-状态", NULL, SwitchComponents_KFP5, 0, NULL },
                                        { Type_Slider, "采样周期(ms)", NULL, SlideComponents_ADC_Cycle5, 0, NULL },
                                        { Type_Slider, "温度补偿", NULL, SlideComponents_TempComp_5, 0, NULL },
                                        { Type_Slider, "过程噪声协方差", NULL, SlideComponents_KFP_Q5, 0, NULL },
                                        { Type_Slider, "观察噪声协方差", NULL, SlideComponents_KFP_R5, 0, NULL },
                                        { Type_MenuName, "----------", NULL, 200, 2, NULL },

                                        // 5V电压
                                        { Type_Switch, "5V电压-状态", NULL, SwitchComponents_KFP6, 0, NULL },
                                        { Type_Slider, "采样周期(ms)", NULL, SlideComponents_ADC_Cycle6, 0, NULL },
                                        { Type_Slider, "温度补偿", NULL, SlideComponents_TempComp_6, 0, NULL },
                                        { Type_Slider, "过程噪声协方差", NULL, SlideComponents_KFP_Q6, 0, NULL },
                                        { Type_Slider, "观察噪声协方差", NULL, SlideComponents_KFP_R6, 0, NULL },
                                        { Type_MenuName, "----------", NULL, 200, 2, NULL },

                                        // 环境温度
                                        { Type_Switch, "环境温度-状态", NULL, SwitchComponents_KFP7, 0, NULL },
                                        { Type_Slider, "采样周期(ms)", NULL, SlideComponents_ADC_Cycle7, 0, NULL },
                                        { Type_Slider, "温度补偿", NULL, SlideComponents_TempComp_7, 0, NULL },
                                        { Type_Slider, "过程噪声协方差", NULL, SlideComponents_KFP_Q7, 0, NULL },
                                        { Type_Slider, "观察噪声协方差", NULL, SlideComponents_KFP_R7, 0, NULL },
                                        { Type_MenuName, "----------", NULL, 200, 2, NULL },
                                        { Type_ReturnMenu, "返回", NULL, 200, 2, NULL },
                                    } },

    { 203, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "温度管理", NULL, 200, 3, NULL },
                                        // 整体温度
                                        { Type_Slider, "加热台最小温度", Set13, SlideComponents_HeatMinTemp, 0, NULL },
                                        { Type_Slider, "加热台最大温度", Set13, SlideComponents_HeatMaxTemp, 0, NULL },
                                        { Type_Slider, "T12最小温度", Set13, SlideComponents_T12MinTemp, 0, NULL },
                                        { Type_Slider, "T12最大温度", Set13, SlideComponents_T12MaxTemp, 0, NULL },
                                        // T12
                                        { Type_Slider, "T12休息时间(s)", Set13, SlideComponents_IdleTime, 0, NULL },
                                        { Type_Slider, "T12休息温度", Set13, SlideComponents_IdleTemp, 0, NULL },
                                        { Type_Slider, "T12停机时间(s)", Set13, SlideComponents_StopTime, 0, NULL },
                                        { Type_ReturnMenu, "返回", Save, 200, 3, NULL },
                                    } },

    /////////////////////////////////////////////////////
    // 三级菜单
    { 210, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "配置列表", NULL, 201, 1, *loadHeatConfig },
                                        // { Type_CheckBox, "", NULL, SwitchComponents_CurConfigIndex, 0, *JumpWithTitle },
                                        // { Type_CheckBox, "", NULL, SwitchComponents_CurConfigIndex, 1, *JumpWithTitle },
                                    } },

    { 211, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "设置温度曲线", NULL, 201, 4, *saveCurrentHeatData },
                                        { Type_Slider, "升温斜率", NULL, SlideComponents_PTemp_WarmUpRampRate, 0, NULL },
                                        { Type_Slider, "预热区温度", NULL, SlideComponents_PTemp_WarmUpTemp, 0, NULL },
                                        { Type_Slider, "预热区时间", NULL, SlideComponents_PTemp_WarmUpTime, 0, NULL },
                                        { Type_Slider, "升温斜率", NULL, SlideComponents_PTemp_RefloatRampRate, 0, NULL },
                                        { Type_Slider, "回流区温度", NULL, SlideComponents_PTemp_RefloatTemp, 0, NULL },
                                        { Type_Slider, "回流区时间", NULL, SlideComponents_PTemp_RefloatTime, 0, NULL },
                                        { Type_Slider, "降温斜率", NULL, SlideComponents_PTemp_CoolDownTime, 0, NULL },
                                        { Type_ReturnMenu, "返回", NULL, 201, 4, *saveCurrentHeatData },
                                    } },

    { 212, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "PID参数", NULL, 201, 2, *saveCurrentHeatData },
                                        { Type_Slider, "PID采样时间", NULL, SlideComponents_PIDSample, 0, NULL },
                                        { Type_Slider, "PID切换温度", NULL, SlideComponents_SwitchTemp, 0, NULL }, // 相差多少度开始切换
                                        { Type_GotoMenu, "PID爬升期参数", NULL, 213, 0, NULL },
                                        { Type_GotoMenu, "PID接近期参数", NULL, 214, 0, NULL },
                                        { Type_ReturnMenu, "返回", NULL, 201, 2, *saveCurrentHeatData },
                                    } },

    { 213, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "PID爬升期", NULL, 212, 1, *saveCurrentHeatData },
                                        { Type_Slider, "比例P", NULL, SlideComponents_PID_AP, 0, NULL },
                                        { Type_Slider, "积分I", NULL, SlideComponents_PID_AI, 0, NULL },
                                        { Type_Slider, "微分D", NULL, SlideComponents_PID_AD, 0, NULL },
                                        { Type_ReturnMenu, "返回", NULL, 212, 1, *saveCurrentHeatData },
                                    } },

    { 214, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "PID接近期", NULL, 212, 2, *saveCurrentHeatData },
                                        { Type_Slider, "比例P", NULL, SlideComponents_PID_CP, 0, NULL },
                                        { Type_Slider, "积分I", NULL, SlideComponents_PID_CI, 0, NULL },
                                        { Type_Slider, "微分D", NULL, SlideComponents_PID_CD, 0, NULL },
                                        { Type_ReturnMenu, "返回", NULL, 212, 2, *saveCurrentHeatData },
                                    } },

    { 215, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "其他设置", NULL, 201, 3, *saveCurrentHeatData },
                                        { Type_GotoMenu, "模式配置", NULL, 217, 0, NULL },
                                        { Type_Slider, "恒温温度(°C)", NULL, SlideComponents_TargetTemp, 0, NULL },
                                        { Type_ReturnMenu, "返回", NULL, 201, 3, *saveCurrentHeatData },
                                    } },

    { 217, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "模式配置", NULL, 215, 1, *saveCurrentHeatData },
                                        { Type_CheckBox, "恒温加热台", NULL, SwitchComponents_HeatMoe, 0, NULL },
                                        { Type_CheckBox, "回流焊加热台", NULL, SwitchComponents_HeatMoe, 1, NULL },
                                        { Type_CheckBox, "T12电烙铁", NULL, SwitchComponents_HeatMoe, 2, NULL },
                                        { Type_ReturnMenu, "返回", NULL, 215, 1, *saveCurrentHeatData },
                                    } },

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    { 14, 0, 0, 0, MenuRenderImage, {
                                        { Type_MenuName, "手柄触发", NULL, 6, 4, NULL },
                                        { Type_CheckBox, "关闭", IMG_VibrationSwitch, SwitchComponents_T12VibrationDetection, 0, *JumpWithTitle },
                                        { Type_CheckBox, "开启", IMG_ReedSwitch, SwitchComponents_T12VibrationDetection, 1, *JumpWithTitle },
                                    } },

};

/*
    @brief 获取当前菜单的渲染类型
    @param uint8_t id 菜单层对象id
    @return 菜单层对象 的索引值
*/
menuSystem* getCurRenderMenu(int menuID)
{
    int size = menuInfo.size();
    for (int i = 0; i < size; i++) {
        if (menuInfo[i].menuID == menuID) {
            return &menuInfo[i];
        }
    }
    return NULL;
}

/**
 * @brief 得到指定位置的菜单
 *
 * @param pMenuRoot
 * @param id
 * @return subMenu*
 */
subMenu* getSubMenu(menuSystem* pMenuRoot, int id)
{
    if (NULL != pMenuRoot)
        return &pMenuRoot->subSystem[id];
    return NULL;
}

/**
 * @brief 打印出所有菜单内容
 *
 * @param argc
 * @param argv
 * @return int
 */
static int do_dumpmenu_cmd(int argc, char** argv)
{
    const char kalmanStr[][32] = {
        "MAX6675",
        "T12温度",
        "T12",
        "T12",
        "环境",
        "电压",
    };

    settings_write_all();
    settings_read_all();

    printf("----- dump menu -----\n");
    printf("语言:%d 蓝牙状态:%d 屏幕翻转:%d 渲染方式:%d 动画标志:%d\n", SystemMenuSaveData.Language, SystemMenuSaveData.BLEState, SystemMenuSaveData.ScreenFlip, SystemMenuSaveData.MenuListMode, SystemMenuSaveData.SmoothAnimationFlag);
    printf("音量:%d 首页设置:%d 固定方式:%d 屏幕亮度:%1.1f 欠压提醒:%1.1f\n", SystemMenuSaveData.Volume, SystemMenuSaveData.PanelSettings, SystemMenuSaveData.OptionStripFixedLength_Flag, SystemMenuSaveData.ScreenBrightness, SystemMenuSaveData.UndervoltageAlert);
    printf("屏保时间:%1.0f\n", SystemMenuSaveData.ScreenProtectorTime);
    printf("蓝牙名称:%s 密码:%s\n", SystemMenuSaveData.BLEName, SystemMenuSaveData.BootPasswd);

    printf("----- dump config -----\n");
    printf("配置个数:%d 当前配置:%d\r\n", HeatingConfig.heatingConfig.size(), HeatingConfig.curConfigIndex);
    for (int i = 0; i < HeatingConfig.heatingConfig.size(); i++) {
        printf("------------------------------- 第%d个配置 -------------------------------\n", i);
        printf("配置名:%s 模式:%s\r\n", HeatingConfig.heatingConfig[i].name, heatingModeStr[HeatingConfig.heatingConfig[i].type]);
        printf("PID %1.2f %1.2f %1.2f, %1.2f %1.2f %1.2f\r\n", HeatingConfig.heatingConfig[i].PID[0][0], HeatingConfig.heatingConfig[i].PID[0][1], HeatingConfig.heatingConfig[i].PID[0][2], HeatingConfig.heatingConfig[i].PID[1][0], HeatingConfig.heatingConfig[i].PID[1][1], HeatingConfig.heatingConfig[i].PID[1][2]);
        printf("升温斜率 %1.1f %1.1f %1.1f %1.1f %1.1f %1.1f %1.1f\r\n", HeatingConfig.heatingConfig[i].PTemp[0], HeatingConfig.heatingConfig[i].PTemp[1], HeatingConfig.heatingConfig[i].PTemp[2], HeatingConfig.heatingConfig[i].PTemp[3], HeatingConfig.heatingConfig[i].PTemp[4], HeatingConfig.heatingConfig[i].PTemp[5], HeatingConfig.heatingConfig[i].PTemp[6]);
        if (TYPE_HEATING_VARIABLE != HeatingConfig.heatingConfig[i].type) {
            printf("恒温输出温度:%1.1f°C \n", HeatingConfig.heatingConfig[i].targetTemp);
        }
    }

    printf("----- dump 卡尔曼滤波 -----\n");
    for (int i = 0; i < adc_last_max; i++) {
        printf("%s卡尔曼:%s 采样周期:%1.1fms 过程噪声:%f 观测噪声:%f\n", kalmanStr[i], KalmanInfo[i].parm.UseKalman != 0 ? "开启" : "关闭", KalmanInfo[i].parm.Cycle, KalmanInfo[i].parm.KalmanQ, KalmanInfo[i].parm.KalmanR);
    }

    return 0;
}

/**
 * @brief 初始化菜单系统
 *
 */
void initMenuSystem(void)
{
    int size = menuInfo.size();
    for (int i = 0; i < size; i++) {
        menuInfo[i].minMenuSize = 0;
        menuInfo[i].maxMenuSize = menuInfo[i].subSystem.size();
    }

    // 初始化屏幕亮度
    updateOledLight();

    // 初始化是否镜像显示
    updateOledFlip();

    // 初始化扩展菜单
    initMenuExpand();

    // 初始化shell命令
    register_cmd("dumpmenu", "打印出所有菜单内容", NULL, do_dumpmenu_cmd, NULL);
}

/**
 * @brief 获取屏幕保护时间
 *
 * @return float
 */
float getScreenProtectorTime(void)
{
    return SystemMenuSaveData.ScreenProtectorTime;
}

/**
 * @brief 获取设置欠压设置(V)
 *
 * @return float
 */
float getSystemUndervoltageAlert(void)
{
    return SystemMenuSaveData.UndervoltageAlert;
}

/**
 * @brief 蓝牙是否开启
 *
 * @return uint8_t
 */
uint8_t getBlueToolsStatus(void)
{
    return SystemMenuSaveData.BLEState;
}

/**
 * @brief 获取声音设置
 *
 * @return uint8_t
 */
uint8_t getVolume(void)
{
    return SystemMenuSaveData.Volume;
}
