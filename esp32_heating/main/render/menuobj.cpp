#include "heating.h"
#include "bitmap.h"
#include "ExternDraw.h"
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
    PanelSettings : PANELSET_Simple, // 面板设置 详细面板 简约面板
    OptionStripFixedLength_Flag : 0, // 选项条固定 自适应
    ScreenBrightness : 128, // 屏幕亮度
    UndervoltageAlert : 18, // 系统电压 欠压警告阈值 (单位V)
    BLEName : { 'H', 'e', 'a', 't', 'i', 'n', 'g' }, // 蓝牙设备名称
    BootPasswd : {}, // 开机密码
};

static float MenuScroll = 0; // 当前菜单的位置

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 各模块相关变量
uint8_t PIDMode = true; // PID模式
uint8_t Use_KFP = true;
uint8_t HandleTrigger = HANDLETRIGGER_VibrationSwitch;

float BootTemp = 0; //开机温度 (°C)
float SleepTemp = 0; //休眠温度 (°C)
float ShutdownTime = 0; //关机提醒 (分)
float SleepTime = 0; //休眠触发时间 (分)
float ScreenProtectorTime = 0; //屏保在休眠后的触发时间 (秒)

KFP KFP_Temp = { 0.02, 0, 0, 0, 0.01, 0.1 };
float ADC_PID_Cycle_List[3] = { 200, 200, 200 };

_HeatingSystem HeatingConfig = {
    maxConfig : 10, // 最大配置数量
    curConfigIndex : -1, // 当前使用配置索引
    curConfig : {}, // 当前使用的配置参数
    heatingConfig : {
        // { "1", TYPE_HEATING_CONSTANT },
        // { "2", TYPE_HEATING_VARIABLE },
        // { "T12", TYPE_T12 },
    },
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ExitMenuSystem(void);
void BLE_Restart(void);
void BLE_Rename(void);
void SetPasswd(void);
void About(void);

void newHeatConfig(void);
void JumpWithTitle(void);
void PopMsg_ListMode(void);
void loadHeatConfig(void);
void saveHeatPIDConfig(void);
void saveHeatTempCurve(void);
void saveHeatConfig(void);

void renameHeatConfig(void);
void delHeatConfig(void);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief
uint8_t* SwitchControls[] = {
    &SystemMenuSaveData.Language, // 语言设置
    &SystemMenuSaveData.BLEState, // 蓝牙开关
    &SystemMenuSaveData.ScreenFlip, // 翻转OLED
    &SystemMenuSaveData.MenuListMode, // 图标模式 列表模式
    &SystemMenuSaveData.SmoothAnimationFlag, // 过度动画标志位
    &SystemMenuSaveData.Volume, // 声音设置
    &SystemMenuSaveData.PanelSettings, // 面板设置 详细面板 简约面板
    &SystemMenuSaveData.OptionStripFixedLength_Flag, // 选项条固定 自适应
    // &RotaryDirection, // 编码器反向

    ///////////////////////////////////
    &PIDMode,
    &Use_KFP,
    &HandleTrigger,
    (uint8_t*)&HeatingConfig.curConfigIndex, // 当前加热台配置索引
};

// 滑动组建菜单
struct SlideBar SlideControls[] = {
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    { (float*)&SystemMenuSaveData.ScreenBrightness, 0, 255, 5 }, // 屏幕亮度
    { (float*)&SystemMenuSaveData.UndervoltageAlert, 0, 36, 0.25 }, // 欠压提醒
    { (float*)&MenuScroll, 0, OLED_SCREEN_HEIGHT / 16, 1 }, // 当前菜单显示的位置（每页只显示4个，所以范围在0~3） 高度为16像素

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    { (float*)&BootTemp, 0, 300, 5 },
    { (float*)&SleepTemp, 0, 300, 5 },
    // {(float*) &BoostTemp,             0,   150,  1},

    { (float*)&ShutdownTime, 0, 60, 1 },
    { (float*)&SleepTime, 0, 60, 1 },
    // {(float*) &BoostTime,             0,   600,  1},
    { (float*)&ScreenProtectorTime, 0, 600, 1 },

    { (float*)&KFP_Temp.Q, 0, 5, 0.01 },
    { (float*)&KFP_Temp.R, 0, 25, 0.1 },

    // {(float*) &SamplingRatioWork,     1,   100,  1},
    { (float*)&ADC_PID_Cycle_List[0], 25, 2000, 25 },
    { (float*)&ADC_PID_Cycle_List[1], 25, 2000, 25 },
    { (float*)&ADC_PID_Cycle_List[2], 25, 2000, 25 },

    // 温度曲线
    { (float*)&HeatingConfig.curConfig.PTemp[0], 0.5, 5, 0.5 }, // 升温斜率
    { (float*)&HeatingConfig.curConfig.PTemp[1], 0, HeatMaxTemp, 5 }, // 预热区温度(摄氏度)
    { (float*)&HeatingConfig.curConfig.PTemp[2], 0, 600, 5 }, // 预热区时间(秒)
    { (float*)&HeatingConfig.curConfig.PTemp[3], 0.5, 5, 0.5 }, // 升温斜率
    { (float*)&HeatingConfig.curConfig.PTemp[4], 0, HeatMaxTemp, 5 }, // 回流区温度(摄氏度)
    { (float*)&HeatingConfig.curConfig.PTemp[5], 0, 600, 5 }, // 回流区时间(秒)
    { (float*)&HeatingConfig.curConfig.PTemp[6], 0.5, 5, 0.5 }, // 降温斜率

    // PID参数
    { (float*)&HeatingConfig.curConfig.PID[0][0], 0, 10, 0.01 }, // 比例P(爬升期)
    { (float*)&HeatingConfig.curConfig.PID[0][1], 0, 10, 0.01 }, // I
    { (float*)&HeatingConfig.curConfig.PID[0][2], 0, 10, 0.01 }, // D
    { (float*)&HeatingConfig.curConfig.PID[1][0], 0, 10, 0.01 }, // 比例P(接近期)
    { (float*)&HeatingConfig.curConfig.PID[1][1], 0, 10, 0.01 }, // I
    { (float*)&HeatingConfig.curConfig.PID[1][2], 0, 10, 0.01 }, // D
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
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}

/**
 * @brief 更新屏幕亮度
 *
 */
void updateOledLight(void)
{
    Update_OLED_Light_Level((uint8_t)*SlideControls[Slide_space_ScreenBrightness].val);
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

/**
 * @brief 刷新加热台配置列表
 *
 */
void flushHeatConfig(void)
{
    menuSystem* pMenuRoot = getCurRenderMenu(210);
    if (NULL != pMenuRoot) {
        // 删除旧的菜单
        int size = pMenuRoot->subSystem.size();

        for (int i = 1; i < size; i++) {
            pMenuRoot->subSystem.pop_back();
        }

        subMenu _subMenu = {
            type : Type_CheckBox,
            name : "",
            icon : NULL,
            ParamA : SwitchSpace_CurConfigIndex,
            ParamB : 0,
            function : *JumpWithTitle,
        };

        // 添加菜单
        for (int i = 0; i < HeatingConfig.heatingConfig.size(); i++) {
            _subMenu.name = HeatingConfig.heatingConfig[i].name;
            _subMenu.ParamB = i;
            pMenuRoot->subSystem.push_back(_subMenu);
        }

        // 刷新个数
        pMenuRoot->maxMenuSize = pMenuRoot->subSystem.size();
    }

    if (-1 != HeatingConfig.curConfigIndex && HeatingConfig.curConfigIndex < HeatingConfig.heatingConfig.size()) {
        memcpy((void*)&HeatingConfig.curConfig, (void*)&HeatingConfig.heatingConfig[HeatingConfig.curConfigIndex], sizeof(HeatingConfig.curConfig));
    }
}

/**
 * @brief 新建加热台配置
 *
 */
void newHeatConfig(void)
{
    if (HeatingConfig.heatingConfig.size() < HeatingConfig.maxConfig) {
        char name[20];
        snprintf(name, sizeof(name), "heating-%d", HeatingConfig.heatingConfig.size());
        TextEditor("[输入配置名]", name, sizeof(name));

        if (strlen(name) > 0) {
            _HeatingConfig heatingConfig = {};
            heatingConfig.name = name;
            heatingConfig.type = TYPE_HEATING_CONSTANT;

            heatingConfig.PTemp[0] = 2.0f; // 升温斜率
            heatingConfig.PTemp[1] = 120.0f; // 预热区温度(摄氏度)
            heatingConfig.PTemp[2] = 100.0f; // 预热区时间(秒)
            heatingConfig.PTemp[3] = 1.0f; // 升温斜率
            heatingConfig.PTemp[4] = 250.0f; // 回流区温度(摄氏度)
            heatingConfig.PTemp[5] = 60.0f; // 回流区时间(秒)
            heatingConfig.PTemp[6] = 3.0f; // 降温斜率

            // 爬升期 PID
            heatingConfig.PID[0][0] = 4.0f;
            heatingConfig.PID[0][1] = 0.2f;
            heatingConfig.PID[0][2] = 1.0f;
            // 接近期 PID
            heatingConfig.PID[1][0] = 1.0f;
            heatingConfig.PID[1][1] = 0.05f;
            heatingConfig.PID[1][2] = 0.25f;

            HeatingConfig.curConfigIndex = HeatingConfig.heatingConfig.size();
            HeatingConfig.heatingConfig.push_back(heatingConfig);

            // 复制设置
            memcpy((void*)&HeatingConfig.curConfig, (void*)&heatingConfig, sizeof(HeatingConfig.curConfig));

            PopWindows("新建成功");
        }

    } else {
        PopWindows("超过最大配置限制");
    }
}

/**
 * @brief 载入烙铁头配置
 *
 */
void loadHeatConfig(void)
{
    if (-1 != HeatingConfig.curConfigIndex && HeatingConfig.curConfigIndex < HeatingConfig.heatingConfig.size()) {
        memcpy((void*)&HeatingConfig.curConfig, (void*)&HeatingConfig.heatingConfig[HeatingConfig.curConfigIndex], sizeof(HeatingConfig.curConfig));

        // 提示
        char name[128];
        snprintf(name, sizeof(name), "载入[%s]成功", HeatingConfig.curConfig.name.c_str());
        PopWindows(name);
    }
}

/**
 * @brief 保存PID设置参数
 *
 */
void saveHeatPIDConfig(void)
{
    if (-1 != HeatingConfig.curConfigIndex && HeatingConfig.curConfigIndex < HeatingConfig.heatingConfig.size()) {
        memcpy((void*)&HeatingConfig.heatingConfig[HeatingConfig.curConfigIndex].PID, (void*)&HeatingConfig.curConfig.PID, sizeof(HeatingConfig.curConfig.PID));
    }
}

/**
 * @brief 保存温度曲线参数
 *
 */
void saveHeatTempCurve(void)
{
    if (-1 != HeatingConfig.curConfigIndex && HeatingConfig.curConfigIndex < HeatingConfig.heatingConfig.size()) {
        memcpy((void*)&HeatingConfig.heatingConfig[HeatingConfig.curConfigIndex].PTemp, (void*)&HeatingConfig.curConfig.PTemp, sizeof(HeatingConfig.curConfig.PTemp));
    }
}

/**
 * @brief 保存烙铁头配置
 *
 */
void saveHeatConfig(void)
{
}

/**
 * @brief 重命名当前的配置
 *
 */
void renameHeatConfig(void)
{
    if (-1 != HeatingConfig.curConfigIndex && HeatingConfig.curConfigIndex < HeatingConfig.heatingConfig.size()) {
        char name[20] = { 0 };
        strncpy(name, HeatingConfig.heatingConfig[HeatingConfig.curConfigIndex].name.c_str(), sizeof(name));
        TextEditor("[重命名配置]", name, sizeof(name));

        if (strlen(name) > 0) {
            HeatingConfig.heatingConfig[HeatingConfig.curConfigIndex].name = name;
            PopWindows("重命名成功");
        }
    } else {
        PopWindows("未选择配置");
    }
}

/**
 * @brief 删除配置
 *
 */
void delHeatConfig(void)
{
    if (-1 == HeatingConfig.curConfigIndex || HeatingConfig.curConfigIndex >= HeatingConfig.heatingConfig.size()) {
        PopWindows("配置选择错误");

    } else if (1 < HeatingConfig.heatingConfig.size()) {
        // 删除
        HeatingConfig.heatingConfig.erase(HeatingConfig.heatingConfig.begin() + HeatingConfig.curConfigIndex, HeatingConfig.heatingConfig.begin() + HeatingConfig.curConfigIndex + 1);

        // 选择第0个
        HeatingConfig.curConfigIndex = 0;

        PopWindows("删除成功");

        // 加载新配置
        loadHeatConfig();

    } else if (1 == HeatingConfig.heatingConfig.size()) {
        PopWindows("最后一个配置 无法删除");

    } else {
        PopWindows("未知错误");
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static std::vector<menuSystem> menuInfo = {
    // 菜单入口
    { 0, 0, 0, 0, MenuRenderText, {
                                      { Type_MenuName, "[加热台设置]", NULL, 0, 0, *ExitMenuSystem },
                                      { Type_GotoMenu, "温度设置", NULL, 200, 0, NULL },
                                      { Type_GotoMenu, "系统设置", NULL, 100, 0, NULL },
                                      { Type_RunFunction, "返回", NULL, 0, 0, *ExitMenuSystem },
                                      { Type_RunFunction, "重启系统", NULL, 0, 0, *(SYSReboot) },
                                  } },

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 系统设置部分
    { 100, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "系统设置", NULL, 0, 2, NULL },
                                         { Type_GotoMenu, "个性化", IMG_Pen, 101, 0, NULL },
                                         { Type_GotoMenu, "蓝牙", IMG_BLE, 102, 0, NULL },
                                         { Type_Slider, "欠压提醒(V)", Set6, Slide_space_UndervoltageAlert, 0, NULL },
                                         { Type_RunFunction, "开机密码", Lock, 0, 0, *SetPasswd },
                                         { Type_GotoMenu, "语言设置", Set_LANG, 103, 0, NULL },
                                         { Type_RunFunction, "关于系统", QRC, 5, 5, *About },
                                         { Type_ReturnMenu, "返回", Set7, 0, 2, NULL },
                                     } },

    { 101, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "个性化", NULL, 100, 1, NULL },
                                         { Type_GotoMenu, "显示效果", Set4, 110, 0, NULL },
                                         { Type_GotoMenu, "声音设置", Set5, 111, 0, NULL },
                                         // { Type_Switch, "编码器方向", Set19, SwitchSpace_RotaryDirection, 0, *PopMsg_RotaryDirection },
                                         // {  Type_GotoMenu,         "手柄触发",            IMG_Trigger,           14,                                 0,            NULL},
                                         { Type_ReturnMenu, "返回", Set7, 100, 1, NULL },
                                     } },

    { 102, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "蓝牙", NULL, 100, 2, NULL },
                                        { Type_Switch, "状态", NULL, SwitchSpace_BLE_State, 0, *BLE_Restart },
                                        { Type_RunFunction, "设备名称", NULL, 22, 2, *BLE_Rename },
                                        { Type_ReturnMenu, "返回", NULL, 100, 2, NULL },
                                    } },

    { 103, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "语言设置", NULL, 100, 5, NULL },
                                         { Type_CheckBox, "简体中文", Lang_CN, SwitchSpace_Language, 0, *JumpWithTitle },
                                     } },

    /////////////////////////////////////////////////////
    // 三级菜单
    { 110, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "显示效果", NULL, 101, 1, NULL },
                                         { Type_GotoMenu, "面板设置", Set0, 112, 0, NULL },
                                         { Type_Switch, "翻转屏幕", IMG_Flip, SwitchSpace_ScreenFlip, 0, *updateOledFlip },
                                         { Type_GotoMenu, "过渡动画", IMG_Animation, 113, 0, NULL },
                                         { Type_Slider, "屏幕亮度", IMG_Sun, Slide_space_ScreenBrightness, 1, *updateOledLight },
                                         { Type_GotoMenu, "选项条定宽", IMG_Size, 114, 0, NULL },
                                         { Type_Switch, "列表模式", IMG_ListMode, SwitchSpace_MenuListMode, 0, NULL },
                                         { Type_ReturnMenu, "返回", Set7, 101, 1, NULL },
                                     } },

    { 111, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "声音设置", NULL, 101, 2, NULL },
                                         { Type_CheckBox, "关闭", Set5_1, SwitchSpace_Volume, 0, *JumpWithTitle },
                                         { Type_CheckBox, "开启", Set5, SwitchSpace_Volume, 1, *JumpWithTitle },
                                     } },

    { 112, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "面板设置", NULL, 110, 1, NULL },
                                         { Type_CheckBox, "简约", Set17, SwitchSpace_PanelSettings, 0, *JumpWithTitle },
                                         { Type_CheckBox, "详细", Set18, SwitchSpace_PanelSettings, 1, *JumpWithTitle },
                                     } },

    { 113, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "过渡动画", NULL, 110, 1, NULL },
                                         { Type_CheckBox, "关闭", IMG_Animation_DISABLE, SwitchSpace_SmoothAnimation, 0, *JumpWithTitle },
                                         { Type_CheckBox, "开启", IMG_Animation, SwitchSpace_SmoothAnimation, 1, *JumpWithTitle },
                                     } },

    { 114, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "选项条定宽设置&测试", NULL, 110, 5, NULL },
                                        { Type_CheckBox, "自适应", NULL, SwitchSpace_OptionWidth, 0, NULL },
                                        { Type_CheckBox, "固定", NULL, SwitchSpace_OptionWidth, 1, NULL },

                                        { Type_GotoMenu, "--- 往下翻 ---", NULL, 9, 4, NULL },
                                        { Type_NULL, "你好!", NULL, 0, 0, NULL },
                                        { Type_NULL, "我是谁~", NULL, 0, 0, NULL },
                                        { Type_NULL, "我才哪里啊???", NULL, 0, 1, NULL },
                                        { Type_NULL, "动 力！", NULL, 0, 0, NULL },
                                        { Type_GotoMenu, "--- 往上翻 ---", NULL, 9, 0, NULL },
                                        { Type_ReturnMenu, "返回", NULL, 110, 5, NULL },
                                    } },

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 温度设置部分
    { 200, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "温度设置", NULL, 0, 1, NULL },
                                         { Type_GotoMenu, "加热台管理", IMG_Tip, 201, 0, NULL },
                                         { Type_GotoMenu, "温度场景", Set1, 202, 0, NULL },
                                         { Type_GotoMenu, "定时场景", Set2, 203, 0, NULL },
                                         { Type_GotoMenu, "温控设置", Set3, 204, 0, NULL },
                                         { Type_ReturnMenu, "返回", Set7, 0, 1, NULL },
                                     } },

    { 201, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "加热台管理", NULL, 200, 1, NULL },
                                         { Type_GotoMenu, "切换配置", Set8, 210, 0, *flushHeatConfig },
                                         { Type_GotoMenu, "设置温度曲线", Set9, 211, 0, NULL },
                                         { Type_GotoMenu, "PID参数", Set3, 212, 0, NULL },
                                         { Type_RunFunction, "查看温度曲线", Set0, 0, 0, *DrawTempCurve },
                                         { Type_RunFunction, "新建配置", IMG_Files, 0, 0, *newHeatConfig },
                                         { Type_RunFunction, "重命名配置", IMG_Pen2, 0, 0, *renameHeatConfig },
                                         { Type_RunFunction, "删除配置", Set10, 0, 0, *delHeatConfig },
                                         { Type_ReturnMenu, "返回", Save, 200, 1, *saveHeatConfig },
                                     } },

    { 202, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "温度场景", NULL, 200, 2, NULL },
                                         { Type_Slider, "启动温度", Set13, Slide_space_BootTemp, 0, NULL },
                                         // {  Type_Slider, "提温温度",            Set14,                 Slide_space_BoostTemp,              0,            NULL},
                                         { Type_Slider, "休眠温度", Set11, Slide_space_SleepTemp, 0, NULL },
                                         { Type_ReturnMenu, "返回", Save, 200, 2, NULL },
                                     } },

    { 203, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "定时场景", NULL, 200, 3, NULL },
                                         { Type_Slider, "停机触发(分)", Set13, Slide_space_ShutdownTime, 0, NULL },
                                         // {  Type_Slider, "提温时长(秒)",        Set14,                 Slide_space_BoostTime,              0,            NULL},
                                         { Type_Slider, "休眠触发(分)", Set11, Slide_space_SleepTime, 0, NULL },
                                         { Type_Slider, "屏保触发(秒)", Set4, Slide_space_ScreenProtectorTime, 0, NULL },
                                         { Type_ReturnMenu, "返回", Save, 200, 3, NULL },
                                     } },

    { 204, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "温控设置", NULL, 200, 4, NULL },
                                        { Type_Switch, "PID状态", NULL, SwitchSpace_PIDMode, 0, NULL },
                                        // { Type_Slider, "采样/加热 %",         NULL,         Slide_space_SamplingRatioWork,      0,            NULL},
                                        { Type_GotoMenu, "采样周期(ms)", NULL, 215, 0, NULL },
                                        { Type_GotoMenu, "卡尔曼滤波器", NULL, 216, 0, NULL },
                                        { Type_ReturnMenu, "返回", NULL, 200, 4, NULL },
                                    } },

    /////////////////////////////////////////////////////
    // 三级菜单
    { 210, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "配置列表", NULL, 201, 1, *loadHeatConfig },
                                        // { Type_CheckBox, "", NULL, SwitchSpace_CurConfigIndex, 0, *JumpWithTitle },
                                        // { Type_CheckBox, "", NULL, SwitchSpace_CurConfigIndex, 1, *JumpWithTitle },
                                    } },

    { 211, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "设置温度曲线", NULL, 201, 2, *saveHeatTempCurve },
                                        { Type_Slider, "升温斜率", NULL, Slide_space_PTemp_0, 0, NULL },
                                        { Type_Slider, "预热区温度", NULL, Slide_space_PTemp_1, 0, NULL },
                                        { Type_Slider, "预热区时间", NULL, Slide_space_PTemp_2, 0, NULL },
                                        { Type_Slider, "升温斜率", NULL, Slide_space_PTemp_3, 0, NULL },
                                        { Type_Slider, "回流区温度", NULL, Slide_space_PTemp_4, 0, NULL },
                                        { Type_Slider, "回流区时间", NULL, Slide_space_PTemp_5, 0, NULL },
                                        { Type_Slider, "降温斜率", NULL, Slide_space_PTemp_6, 0, NULL },
                                        { Type_ReturnMenu, "返回", NULL, 201, 2, *saveHeatTempCurve },
                                    } },

    { 212, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "PID参数", NULL, 201, 3, NULL },
                                        { Type_GotoMenu, "PID爬升期参数", NULL, 213, 0, NULL },
                                        { Type_GotoMenu, "PID接近期参数", NULL, 214, 0, NULL },
                                        { Type_ReturnMenu, "返回", NULL, 201, 3, NULL },
                                    } },

    { 213, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "PID爬升期", NULL, 212, 1, *saveHeatPIDConfig },
                                        { Type_Slider, "比例P", NULL, Slide_space_PID_AP, 0, NULL },
                                        { Type_Slider, "积分I", NULL, Slide_space_PID_AI, 0, NULL },
                                        { Type_Slider, "微分D", NULL, Slide_space_PID_AD, 0, NULL },
                                        { Type_ReturnMenu, "返回", NULL, 212, 1, *saveHeatPIDConfig },
                                    } },

    { 214, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "PID接近期", NULL, 212, 2, *saveHeatPIDConfig },
                                        { Type_Slider, "比例P", NULL, Slide_space_PID_CP, 0, NULL },
                                        { Type_Slider, "积分I", NULL, Slide_space_PID_CI, 0, NULL },
                                        { Type_Slider, "微分D", NULL, Slide_space_PID_CD, 0, NULL },
                                        { Type_ReturnMenu, "返回", NULL, 212, 2, *saveHeatPIDConfig },
                                    } },

    { 215, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "采样周期", NULL, 204, 2, NULL },
                                        { Type_Slider, "温差>150", NULL, Slide_space_ADC_PID_Cycle_List_0, 0, NULL },
                                        { Type_Slider, "温差>50", NULL, Slide_space_ADC_PID_Cycle_List_1, 0, NULL },
                                        { Type_Slider, "温差≤50", NULL, Slide_space_ADC_PID_Cycle_List_2, 0, NULL },
                                        { Type_ReturnMenu, "返回", NULL, 204, 2, NULL },
                                    } },

    { 216, 0, 0, 0, MenuRenderText, {
                                        { Type_MenuName, "卡尔曼滤波器", NULL, 204, 3, NULL },
                                        { Type_Switch, "启用状态", NULL, SwitchSpace_KFP, 0, NULL },
                                        { Type_Slider, "过程噪声协方差", NULL, Slide_space_KFP_Q, 0, NULL },
                                        { Type_Slider, "观察噪声协方差", NULL, Slide_space_KFP_R, 0, NULL },
                                        { Type_ReturnMenu, "返回", NULL, 204, 3, NULL },
                                    } },

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    { 12, 0, 0, 0, MenuRenderImage, {
                                        { Type_MenuName, "温控模式", NULL, 19, 2, NULL },
                                        { Type_CheckBox, "模糊控制", Set15, SwitchSpace_PIDMode, 0, *JumpWithTitle },
                                        { Type_CheckBox, "PID控制", Set16, SwitchSpace_PIDMode, 1, *JumpWithTitle },
                                    } },

    { 14, 0, 0, 0, MenuRenderImage, {
                                        { Type_MenuName, "手柄触发", NULL, 6, 4, NULL },
                                        { Type_CheckBox, "震动开关", IMG_VibrationSwitch, SwitchSpace_HandleTrigger, 0, *JumpWithTitle },
                                        { Type_CheckBox, "干簧管", IMG_ReedSwitch, SwitchSpace_HandleTrigger, 1, *JumpWithTitle },
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

    settings_write_all();
    settings_read_all();

    printf("----- dump menu -----\n");
    printf("语言:%d 蓝牙状态:%d 屏幕翻转:%d 渲染方式:%d 动画标志:%d\n", SystemMenuSaveData.Language, SystemMenuSaveData.BLEState, SystemMenuSaveData.ScreenFlip, SystemMenuSaveData.MenuListMode, SystemMenuSaveData.SmoothAnimationFlag);
    printf("音量:%d 面板设置:%d 固定方式:%d 屏幕亮度:%f 欠压提醒:%f\n", SystemMenuSaveData.Volume, SystemMenuSaveData.PanelSettings, SystemMenuSaveData.OptionStripFixedLength_Flag, SystemMenuSaveData.ScreenBrightness, SystemMenuSaveData.UndervoltageAlert);
    printf("蓝牙名称:%s 密码:%s\n", SystemMenuSaveData.BLEName, SystemMenuSaveData.BootPasswd);

    printf("----- dump config -----\n");
    printf("配置个数:%d 当前配置:%d\r\n", HeatingConfig.heatingConfig.size(), HeatingConfig.curConfigIndex);
    for (int i = 0; i < HeatingConfig.heatingConfig.size(); i++) {
        printf("配置名:%s 类型:%d\r\n", HeatingConfig.heatingConfig[i].name.c_str(), HeatingConfig.heatingConfig[i].type);
        printf("PID %f %f %f, %f %f %f\r\n", HeatingConfig.heatingConfig[i].PID[0][0], HeatingConfig.heatingConfig[i].PID[0][1], HeatingConfig.heatingConfig[i].PID[0][2], HeatingConfig.heatingConfig[i].PID[1][0], HeatingConfig.heatingConfig[i].PID[1][1], HeatingConfig.heatingConfig[i].PID[1][2]);
        printf("升温斜率 %f %f %f %f %f %f %f\r\n", HeatingConfig.heatingConfig[i].PTemp[0], HeatingConfig.heatingConfig[i].PTemp[1], HeatingConfig.heatingConfig[i].PTemp[2], HeatingConfig.heatingConfig[i].PTemp[3], HeatingConfig.heatingConfig[i].PTemp[4], HeatingConfig.heatingConfig[i].PTemp[5], HeatingConfig.heatingConfig[i].PTemp[6]);
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

    // 刷新加热台配置
    flushHeatConfig();

    // 初始化shell命令
    register_cmd("dumpmenu", "打印出所有菜单内容", NULL, do_dumpmenu_cmd, NULL);
}
