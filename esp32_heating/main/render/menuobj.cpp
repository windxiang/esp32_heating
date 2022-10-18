#include "heating.h"
#include "bitmap.h"
#include "ExternDraw.h"

/// @brief
uint8_t SmoothAnimation_Flag = true; // 编码器滑动菜单 动画标志位
uint8_t OptionStripFixedLength_Flag = 0;
uint8_t PIDMode = true; // PID模式
uint8_t Use_KFP = true;
uint8_t PanelSettings = PANELSET_Simple;
uint8_t ScreenFlip = false;
uint8_t Volume = 100;
uint8_t HandleTrigger = HANDLETRIGGER_VibrationSwitch;
uint8_t Language = 1;
uint8_t TipID = 0;
uint8_t BLE_State = true; // 显示蓝牙图标
uint8_t MenuListMode = false; // true:菜单使用文本渲染方式 不使用图标、 false:混合显示模式

/// @brief
float ScreenBrightness = 128;
float MenuScroll = 0; // 当前菜单的位置
float BootTemp = 0; //开机温度 (°C)
float SleepTemp = 0; //休眠温度 (°C)
float ShutdownTime = 0; //关机提醒 (分)
float SleepTime = 0; //休眠触发时间 (分)
float ScreenProtectorTime = 0; //屏保在休眠后的触发时间 (秒)
float UndervoltageAlert = 24; // 系统电压欠压警告阈值

double aggKp = 4, aggKi = 0.2, aggKd = 1;
double consKp = 1, consKi = 0.05, consKd = 0.25;

KFP KFP_Temp = { 0.02, 0, 0, 0, 0.01, 0.1 };
float ADC_PID_Cycle_List[3] = { 200, 200, 200 };
#define FixNum 7
float PTemp[FixNum] = { 0 };

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Save_Exit_Menu_System(void);
void FlashTipMenu(void);
void JumpWithTitle(void);
void Update_OLED_Flip(void);
void Update_OLED_Light_Level(void);
void PopMsg_ListMode(void);
void LoadTipConfig(void);
void SaveTipConfig(void);
void BLE_Restart(void);
void BLE_Rename(void);
void ShowCurveCoefficient(void);
void NewTipConfig(void);
void TipRename(void);
void TipDel(void);
void SetPasswd(void);
void About(void);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief
uint8_t* SwitchControls[] = {
    &SmoothAnimation_Flag, // 编码器滑动菜单 动画标志位
    &OptionStripFixedLength_Flag,

    &PIDMode,
    &Use_KFP,
    &PanelSettings,
    &ScreenFlip,
    &Volume,
    // &RotaryDirection,
    &HandleTrigger,
    &Language,
    &TipID,

    &BLE_State,
    &MenuListMode,
};

// 滑动组建菜单
struct SlideBar SlideControls[] = {
    { (float*)&ScreenBrightness, 0, 255, 5 },
    { (float*)&MenuScroll, 0, OLED_SCREEN_HEIGHT / 16, 1 }, // 当前菜单显示的位置（每页只显示4个，所以范围在0~3） 高度为16像素

    { (float*)&BootTemp, 0, 300, 5 },
    { (float*)&SleepTemp, 0, 300, 5 },
    // {(float*) &BoostTemp,             0,   150,  1},

    { (float*)&ShutdownTime, 0, 60, 1 },
    { (float*)&SleepTime, 0, 60, 1 },
    // {(float*) &BoostTime,             0,   600,  1},
    { (float*)&ScreenProtectorTime, 0, 600, 1 },

    { (float*)&UndervoltageAlert, 0, 36, 0.25 },

    { (float*)&aggKp, 0, 10, 0.02 },
    { (float*)&aggKi, 0, 10, 0.02 },
    { (float*)&aggKd, 0, 10, 0.02 },
    { (float*)&consKp, 0, 10, 0.02 },
    { (float*)&consKi, 0, 10, 0.02 },
    { (float*)&consKd, 0, 10, 0.02 },

    { (float*)&KFP_Temp.Q, 0, 5, 0.01 },
    { (float*)&KFP_Temp.R, 0, 25, 0.1 },

    // {(float*) &SamplingRatioWork,     1,   100,  1},
    { (float*)&ADC_PID_Cycle_List[0], 25, 2000, 25 },
    { (float*)&ADC_PID_Cycle_List[1], 25, 2000, 25 },
    { (float*)&ADC_PID_Cycle_List[2], 25, 2000, 25 },

    { (float*)&PTemp[0], 0.5, 5, 0.5 },
    { (float*)&PTemp[1], 0, 300, 5 },
    { (float*)&PTemp[2], 0, 300, 5 },
    { (float*)&PTemp[3], 0.5, 5, 0.5 },
    { (float*)&PTemp[4], 0, 300, 5 },
    { (float*)&PTemp[5], 0, 300, 5 },
    { (float*)&PTemp[6], 0.5, 5, 0.5 },
};

/**
 * @brief 菜单滑动动画结构体
 *
 */
struct MenuSmoothAnimation menuSmoothAnimation[] = {
    { 0, 0, 0, 0.4, 1 }, // 0 菜单项目滚动动画
    { 0, 0, 0, 0.3, 1 }, // 1 垂直滚动Y位置计算
    { 0, 0, 0, 0.3, 1 }, // 2 垂直滚动W宽度计算
    { 0, 0, 0, 0.2, 0 }, // 3 项目归位动画
};
const int sizeMenuSmoothAnimatioin = sizeof(menuSmoothAnimation) / sizeof(menuSmoothAnimation[0]);

/*
    @brief 菜单渲染类型:文本、图标
    @变量
        uint8_t id    当前层id
        int index     对应的图标id
        uint8_t min   最小页
        uint8_t max   最大页
        uint8_t _RenderType     渲染类型  MenuRenderText：文本， MenuRenderImage：图标
*/
struct MenuLevelSystem MenuLevel[] = {
    { 0, 0, 0, 14, MenuRenderText },
    { 1, 0, 0, 5, MenuRenderImage },
    { 2, 0, 0, 8, MenuRenderImage },
    { 3, 0, 0, 3, MenuRenderImage },
    { 4, 0, 0, 4, MenuRenderImage },
    { 5, 0, 0, 7, MenuRenderImage },
    { 6, 0, 0, 4, MenuRenderImage },
    { 7, 0, 0, 7, MenuRenderImage },
    { 8, 0, 0, 2, MenuRenderImage },
    { 9, 0, 0, 9, MenuRenderText },
    { 10, 0, 0, 2, MenuRenderImage },
    { 11, 0, 0, 2, MenuRenderImage },
    { 12, 0, 0, 2, MenuRenderImage },
    { 13, 0, 0, 1, MenuRenderImage },
    { 14, 0, 0, 2, MenuRenderImage },
    { 15, 0, 0, MaxTipConfig, MenuRenderText },
    { 16, 0, 0, 3, MenuRenderText },
    { 17, 0, 0, 4, MenuRenderText },
    { 18, 0, 0, 4, MenuRenderText },
    { 19, 0, 0, 4, MenuRenderText },
    { 20, 0, 0, 4, MenuRenderText },
    { 21, 0, 0, 4, MenuRenderText },
    { 22, 0, 0, 3, MenuRenderText },
    { 23, 0, 0, 8, MenuRenderText },
};
const int sizeMenuLevel = sizeof(MenuLevel) / sizeof(MenuLevel[0]);

/*
    @brief 菜单层级项目 设定
    @param -

    @变量
        uint8_t lid; //层id
        uint8_t id; //选项id
        _MenuType type; // 菜单类型
        const char* name; //选项名称
        uint8_t* icon;
        uint8_t ParamA; //附加参数A (Type_GotoMenu-打开对应的lid) (Type_Switch-开关id) (Type_Slider-滑动条id)
        uint8_t ParamB; //附加参数B (Type_GotoMenu-打开对应的图标) (Type_Slider-滑动条：true?执行函数:无操作)
        void (*function)();
*/
// 菜单内容定义
extern void SYSReboot(void);

struct MenuSystem Menu[] = {
    // ==
    { 0, 0, Type_MenuName, "[加热台设置]", NULL, 0, 0, *Save_Exit_Menu_System },
    { 0, 1, Type_GotoMenu, "温度设置", NULL, 1, 0, NULL },
    { 0, 2, Type_GotoMenu, "系统设置", NULL, 5, 0, NULL },
    { 0, 3, Type_RunFunction, "返回", NULL, 0, 0, *Save_Exit_Menu_System },
    { 0, 4, Type_RunFunction, "重启系统", NULL, 0, 0, *(SYSReboot) },

    { 0, 5, Type_RunFunction, "0", NULL, 0, 0, *(SYSReboot) },
    { 0, 6, Type_RunFunction, "1", NULL, 0, 0, *(SYSReboot) },
    { 0, 7, Type_RunFunction, "2", NULL, 0, 0, *(SYSReboot) },
    { 0, 8, Type_RunFunction, "3", NULL, 0, 0, *(SYSReboot) },
    { 0, 9, Type_RunFunction, "4", NULL, 0, 0, *(SYSReboot) },
    { 0, 10, Type_RunFunction, "5", NULL, 0, 0, *(SYSReboot) },
    { 0, 11, Type_RunFunction, "6", NULL, 0, 0, *(SYSReboot) },
    { 0, 12, Type_RunFunction, "7", NULL, 0, 0, *(SYSReboot) },
    { 0, 13, Type_RunFunction, "8", NULL, 0, 0, *(SYSReboot) },
    { 0, 14, Type_RunFunction, "9", NULL, 0, 0, *(SYSReboot) },
    // { 0,4,       Type_RunFunction,          "测试",               NULL,                0,                                  0,          *(EnterLogo)},

    // ==
    { 1, 0, Type_MenuName, "此加热台", NULL, 0, 1, NULL },
    { 1, 1, Type_GotoMenu, "加热台", IMG_Tip, 2, 0, NULL },
    { 1, 2, Type_GotoMenu, "温度场景", Set1, 3, 0, NULL },
    { 1, 3, Type_GotoMenu, "定时场景", Set2, 4, 0, NULL },
    { 1, 4, Type_GotoMenu, "温控设置", Set3, 19, 0, NULL },
    { 1, 5, Type_GotoMenu, "返回", Set7, 0, 1, NULL },

    // ==
    { 2, 0, Type_MenuName, "加热台管理", NULL, 1, 1, NULL },
    { 2, 1, Type_GotoMenu, "切换配置", Set8, 15, 0, *FlashTipMenu },
    { 2, 2, Type_RunFunction, "查看温度曲线", Set0, 0, 0, *ShowCurveCoefficient },
    { 2, 3, Type_GotoMenu, "设置温度曲线", Set9, 23, 0, NULL },
    { 2, 4, Type_GotoMenu, "PID参数", Set3, 16, 0, NULL },
    { 2, 5, Type_RunFunction, "新建", IMG_Files, 0, 0, *NewTipConfig },
    { 2, 6, Type_RunFunction, "重命名", IMG_Pen2, 0, 0, *TipRename },
    { 2, 7, Type_RunFunction, "删除", Set10, 0, 0, *TipDel },
    { 2, 8, Type_GotoMenu, "返回", Save, 1, 1, *SaveTipConfig },

    // ==
    { 3, 0, Type_MenuName, "温度场景", NULL, 1, 2, NULL },
    { 3, 1, Type_Slider, "启动温度", Set13, Slide_space_BootTemp, 0, NULL },
    // {3,  2,  Type_Slider, "提温温度",            Set14,                 Slide_space_BoostTemp,              0,            NULL},
    { 3, 2, Type_Slider, "休眠温度", Set11, Slide_space_SleepTemp, 0, NULL },
    { 3, 3, Type_GotoMenu, "返回", Save, 1, 2, NULL },

    // ==
    { 4, 0, Type_MenuName, "定时场景", NULL, 1, 3, NULL },
    { 4, 1, Type_Slider, "停机触发(分)", Set13, Slide_space_ShutdownTime, 0, NULL },
    // {4,  2,  Type_Slider, "提温时长(秒)",        Set14,                 Slide_space_BoostTime,              0,            NULL},
    { 4, 2, Type_Slider, "休眠触发(分)", Set11, Slide_space_SleepTime, 0, NULL },
    { 4, 3, Type_Slider, "屏保触发(秒)", Set4, Slide_space_ScreenProtectorTime, 0, NULL },
    { 4, 4, Type_GotoMenu, "返回", Save, 1, 3, NULL },

    // ==
    { 5, 0, Type_MenuName, "此系统", NULL, 0, 2, NULL },
    { 5, 1, Type_GotoMenu, "个性化", IMG_Pen, 6, 0, NULL },
    { 5, 2, Type_GotoMenu, "蓝牙", IMG_BLE, 22, 0, NULL },
    { 5, 3, Type_Slider, "欠压提醒", Set6, Slide_space_UndervoltageAlert, 0, NULL },
    { 5, 4, Type_RunFunction, "开机密码", Lock, 0, 0, *SetPasswd },
    { 5, 5, Type_GotoMenu, "语言设置", Set_LANG, 13, 0, NULL },
    { 5, 6, Type_RunFunction, "关于朱雀", QRC, 5, 5, *About },
    { 5, 7, Type_GotoMenu, "返回", Set7, 0, 2, NULL },

    // ==
    { 6, 0, Type_MenuName, "个性化", NULL, 5, 1, NULL },
    { 6, 1, Type_GotoMenu, "显示效果", Set4, 7, 0, NULL },
    { 6, 2, Type_GotoMenu, "声音设置", Set5, 10, 0, NULL },
    // { 6, 3, Type_Switch, "编码器方向", Set19, SwitchSpace_RotaryDirection, 0, *PopMsg_RotaryDirection },
    // {6,  4,  Type_GotoMenu,         "手柄触发",            IMG_Trigger,           14,                                 0,            NULL},
    { 6, 3, Type_GotoMenu, "返回", Set7, 5, 1, NULL },

    // ==
    { 7, 0, Type_MenuName, "显示效果", NULL, 6, 1, NULL },
    { 7, 1, Type_GotoMenu, "面板设置", Set0, 8, 0, NULL },
    { 7, 2, Type_Switch, "翻转屏幕", IMG_Flip, SwitchSpace_ScreenFlip, 0, *Update_OLED_Flip },
    { 7, 3, Type_GotoMenu, "过渡动画", IMG_Animation, 11, 0, NULL },
    { 7, 4, Type_Slider, "屏幕亮度", IMG_Sun, Slide_space_ScreenBrightness, 1, *Update_OLED_Light_Level },
    { 7, 5, Type_GotoMenu, "选项条定宽", IMG_Size, 9, 0, NULL },
    { 7, 6, Type_Switch, "列表模式", IMG_ListMode, SwitchSpace_MenuListMode, 0, *PopMsg_ListMode },
    { 7, 7, Type_GotoMenu, "返回", Set7, 6, 1, NULL },

    // == 面板设置
    { 8, 0, Type_MenuName, "面板设置", NULL, 7, 1, NULL },
    { 8, 1, Type_CheckBox, "简约", Set17, SwitchSpace_PanelSettings, 0, *JumpWithTitle },
    { 8, 2, Type_CheckBox, "详细", Set18, SwitchSpace_PanelSettings, 1, *JumpWithTitle },

    // ==
    { 9, 0, Type_MenuName, "选项条定宽设置&测试", NULL, 7, 5, NULL },
    { 9, 1, Type_CheckBox, "固定", NULL, SwitchSpace_OptionStripFixedLength, true, NULL },
    { 9, 2, Type_CheckBox, "自适应", NULL, SwitchSpace_OptionStripFixedLength, false, NULL },
    { 9, 3, Type_GotoMenu, "--- 往下翻 ---", NULL, 9, 4, NULL },
    { 9, 4, Type_NULL, "人民!", NULL, 0, 0, NULL },
    { 9, 5, Type_NULL, "只有人民~", NULL, 0, 0, NULL },
    { 9, 6, Type_NULL, "才是创造世界历史的", NULL, 0, 1, NULL },
    { 9, 7, Type_NULL, "动 力！", NULL, 0, 0, NULL },
    { 9, 8, Type_GotoMenu, "--- 往上翻 ---", NULL, 9, 0, NULL },
    { 9, 9, Type_GotoMenu, "返回", NULL, 7, 5, NULL },

    // ==
    { 10, 0, Type_MenuName, "声音设置", NULL, 6, 2, NULL },
    { 10, 1, Type_CheckBox, "开启", Set5, SwitchSpace_Volume, true, *JumpWithTitle },
    { 10, 2, Type_CheckBox, "关闭", Set5_1, SwitchSpace_Volume, false, *JumpWithTitle },

    // == 滑动菜单动画标记
    { 11, 0, Type_MenuName, "动画设置", NULL, 7, 3, NULL },
    { 11, 1, Type_CheckBox, "开启", IMG_Animation, SwitchSpace_SmoothAnimation, true, *JumpWithTitle },
    { 11, 2, Type_CheckBox, "关闭", IMG_Animation_DISABLE, SwitchSpace_SmoothAnimation, false, *JumpWithTitle },

    // ==
    { 12, 0, Type_MenuName, "温控模式", NULL, 19, 2, NULL },
    { 12, 1, Type_CheckBox, "PID控制", Set16, SwitchSpace_PIDMode, true, *JumpWithTitle },
    { 12, 2, Type_CheckBox, "模糊控制", Set15, SwitchSpace_PIDMode, false, *JumpWithTitle },

    // ==
    { 13, 0, Type_MenuName, "语言设置", NULL, 5, 5, NULL },
    { 13, 1, Type_CheckBox, "简体中文", Lang_CN, SwitchSpace_Language, 1, *JumpWithTitle },

    // ==
    { 14, 0, Type_MenuName, "手柄触发", NULL, 6, 4, NULL },
    { 14, 1, Type_CheckBox, "震动开关", IMG_VibrationSwitch, SwitchSpace_HandleTrigger, 0, *JumpWithTitle },
    { 14, 2, Type_CheckBox, "干簧管", IMG_ReedSwitch, SwitchSpace_HandleTrigger, 1, *JumpWithTitle },

    // ==
    { 15, 0, Type_MenuName, "加热台列表", NULL, 2, 1, *LoadTipConfig },
    { 15, 1, Type_CheckBox, "", NULL, SwitchSpace_TipID, 0, *JumpWithTitle },
    { 15, 2, Type_CheckBox, "", NULL, SwitchSpace_TipID, 1, *JumpWithTitle },
    { 15, 3, Type_CheckBox, "", NULL, SwitchSpace_TipID, 2, *JumpWithTitle },
    { 15, 4, Type_CheckBox, "", NULL, SwitchSpace_TipID, 3, *JumpWithTitle },
    { 15, 5, Type_CheckBox, "", NULL, SwitchSpace_TipID, 4, *JumpWithTitle },
    { 15, 6, Type_CheckBox, "", NULL, SwitchSpace_TipID, 5, *JumpWithTitle },
    { 15, 7, Type_CheckBox, "", NULL, SwitchSpace_TipID, 6, *JumpWithTitle },
    { 15, 8, Type_CheckBox, "", NULL, SwitchSpace_TipID, 7, *JumpWithTitle },
    { 15, 9, Type_CheckBox, "", NULL, SwitchSpace_TipID, 8, *JumpWithTitle },
    { 15, 10, Type_CheckBox, "", NULL, SwitchSpace_TipID, 9, *JumpWithTitle },

    // ==
    { 16, 0, Type_MenuName, "PID参数", NULL, 2, 4, NULL },
    { 16, 1, Type_GotoMenu, "PID爬升期参数", NULL, 17, 0, NULL },
    { 16, 2, Type_GotoMenu, "PID接近期参数", NULL, 18, 0, NULL },
    { 16, 3, Type_GotoMenu, "返回", NULL, 2, 4, NULL },

    // ==
    { 17, 0, Type_MenuName, "PID爬升期", NULL, 16, 1, *SaveTipConfig },
    { 17, 1, Type_Slider, "比例P", NULL, Slide_space_PID_AP, 0, NULL },
    { 17, 2, Type_Slider, "积分I", NULL, Slide_space_PID_AI, 0, NULL },
    { 17, 3, Type_Slider, "微分D", NULL, Slide_space_PID_AD, 0, NULL },
    { 17, 4, Type_GotoMenu, "返回", NULL, 16, 1, *SaveTipConfig },

    // ==
    { 18, 0, Type_MenuName, "PID接近期", NULL, 16, 2, *SaveTipConfig },
    { 18, 1, Type_Slider, "比例P", NULL, Slide_space_PID_CP, 0, NULL },
    { 18, 2, Type_Slider, "积分I", NULL, Slide_space_PID_CI, 0, NULL },
    { 18, 3, Type_Slider, "微分D", NULL, Slide_space_PID_CD, 0, NULL },
    { 18, 4, Type_GotoMenu, "返回", NULL, 16, 2, *SaveTipConfig },

    // ==
    { 19, 0, Type_MenuName, "温控设置", NULL, 1, 4, NULL },
    { 19, 1, Type_Switch, "PID状态", NULL, SwitchSpace_PIDMode, 0, NULL },
    // {19, 2,  Type_Slider, "采样/加热 %",         NULL,         Slide_space_SamplingRatioWork,      0,            NULL},
    { 19, 2, Type_GotoMenu, "采样周期(ms)", NULL, 21, 0, NULL },
    { 19, 3, Type_GotoMenu, "卡尔曼滤波器", NULL, 20, 0, NULL },
    { 19, 4, Type_GotoMenu, "返回", NULL, 1, 4, NULL },

    // ==
    { 20, 0, Type_MenuName, "卡尔曼滤波器", NULL, 19, 3, NULL },
    { 20, 1, Type_Switch, "启用状态", NULL, SwitchSpace_KFP, 0, NULL },
    { 20, 2, Type_Slider, "过程噪声协方差", NULL, Slide_space_KFP_Q, 0, NULL },
    { 20, 3, Type_Slider, "观察噪声协方差", NULL, Slide_space_KFP_R, 0, NULL },
    { 20, 4, Type_GotoMenu, "返回", NULL, 19, 3, NULL },

    // ==
    { 21, 0, Type_MenuName, "采样周期", NULL, 19, 2, NULL },
    { 21, 1, Type_Slider, "温差>150", NULL, Slide_space_ADC_PID_Cycle_List_0, 0, NULL },
    { 21, 2, Type_Slider, "温差>50", NULL, Slide_space_ADC_PID_Cycle_List_1, 0, NULL },
    { 21, 3, Type_Slider, "温差≤50", NULL, Slide_space_ADC_PID_Cycle_List_2, 0, NULL },
    { 21, 4, Type_GotoMenu, "返回", NULL, 19, 2, NULL },

    // ==
    { 22, 0, Type_MenuName, "蓝牙", NULL, 5, 2, NULL },
    { 22, 1, Type_Switch, "状态", NULL, SwitchSpace_BLE_State, 0, *BLE_Restart },
    { 22, 2, Type_RunFunction, "设备名称", NULL, 22, 2, *BLE_Rename },
    { 22, 3, Type_GotoMenu, "返回", NULL, 5, 2, NULL },

    // ==
    { 23, 0, Type_MenuName, "设置温度曲线", NULL, 2, 3, NULL },
    { 23, 1, Type_Slider, "升温斜率", NULL, Slide_space_PTemp_0, 0, NULL },
    { 23, 2, Type_Slider, "预热区温度", NULL, Slide_space_PTemp_1, 0, NULL },
    { 23, 3, Type_Slider, "预热区时间", NULL, Slide_space_PTemp_2, 0, NULL },
    { 23, 4, Type_Slider, "升温斜率", NULL, Slide_space_PTemp_3, 0, NULL },
    { 23, 5, Type_Slider, "回流区温度", NULL, Slide_space_PTemp_4, 0, NULL },
    { 23, 6, Type_Slider, "回流区时间", NULL, Slide_space_PTemp_5, 0, NULL },
    { 23, 7, Type_Slider, "降温斜率", NULL, Slide_space_PTemp_6, 0, NULL },
    { 23, 8, Type_GotoMenu, "返回", NULL, 2, 3, NULL },
};
const int sizeMenu = (sizeof(Menu) / sizeof(Menu[0]));

/***
 * @description: 更新菜单配置列表
 * @param {*}
 * @return {*}
 */
void FlashTipMenu(void)
{
}

/**
 * @brief 反转屏幕
 *
 */
void Update_OLED_Flip(void)
{
}

/*
    @函数 Update_OLED_Light_Level
    @brief 更新屏幕亮度设置
    @param -

*/
void Update_OLED_Light_Level(void)
{
    u8g2_SendF(&u8g2, "c", 0x81); //向SSD1306发送指令：设置内部电阻微调
    u8g2_SendF(&u8g2, "c", (uint8_t)*SlideControls[Slide_space_ScreenBrightness].val); //微调范围（0-255）
}

void PopMsg_ListMode(void)
{
}

/***
 * @description: 载入烙铁头配置到系统环境
 * @param {*}
 * @return {*}
 */
void LoadTipConfig(void)
{
}

/***
 * @description: 保存烙铁头配置到系统环境
 * @param {*}
 * @return {*}
 */
void SaveTipConfig(void)
{
}

/**
 * @brief 蓝牙重启
 *
 */
void BLE_Restart(void)
{
    printf("BLE_Restart %d\r\n", BLE_State);
}

/**
 * @brief 重命名蓝牙名称
 *
 */
void BLE_Rename(void)
{
    static char BLE_name[20] = "Heating"; // 蓝牙设备名
    TextEditor("蓝牙设备名", BLE_name, sizeof(BLE_name));
}

//显示曲线系数
void ShowCurveCoefficient(void)
{
}

/***
 * @description: 新建配置
 * @param {*}
 * @return {*}
 */
void NewTipConfig(void)
{
}

/***
 * @description: 重命名当前的配置
 * @param {*}
 * @return {*}
 */
void TipRename(void)
{
}

/***
 * @description: 删除配置
 * @param {*}
 * @return {*}
 */
void TipDel(void)
{
}

/**
 * @brief 设置密码
 *
 */
void SetPasswd(void)
{
}

void About(void)
{
}