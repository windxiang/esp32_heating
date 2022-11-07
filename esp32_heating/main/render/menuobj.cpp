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

    ScreenProtectorTime : 60.0f, // 屏保在休眠后的触发时间 (秒)
    ScreenBrightness : 128.0f, // 屏幕亮度
    UndervoltageAlert : 18.0f, // 系统电压 欠压警告阈值 (单位V)

    BLEName : { 'H', 'e', 'a', 't', 'i', 'n', 'g' }, // 蓝牙设备名称
    BootPasswd : {}, // 开机密码
};

static float MenuScroll = 0; // 当前菜单的位置

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 各模块相关变量

float T12ShutdownTime = 0; // 烙铁关机提醒 (秒)

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

// 卡尔曼滤波配置
_KalmanInfo KalmanInfo[adc_last_max] = {
    [adc_HeatingTemp] = {
        .filter = { 0.02, 0, 0, 0 },
        .parm = { 1, 100, 0, 0.1f, 0.1f },
    },
    [adc_T12Temp] = {
        .filter = { 0.02, 0, 0, 0 },
        .parm = { 1, 100, 0, 0.1f, 0.1f },
    },
    [adc_T12Cur] = {
        .filter = { 0.02, 0, 0, 0 },
        .parm = { 1, 100, 0, 0.1f, 0.1f },
    },
    [adc_T12NTC] = {
        .filter = { 0.02, 0, 0, 0 },
        .parm = { 1, 100, 0, 0.1f, 0.1f },
    },
    [adc_SystemVol] = {
        .filter = { 0.02, 0, 0, 0 },
        .parm = { 1, 100, 0, 0.1f, 0.1f },
    },
    [adc_RoomTemp] = {
        .filter = { 0.02, 0, 0, 0 },
        .parm = { 1, 100, 0, 0.1f, 0.1f },
    },
};

// T12 震动开关检测配置
uint8_t HandleTrigger = HANDLETRIGGER_VibrationSwitch;

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
    (uint8_t*)&KalmanInfo[adc_RoomTemp].parm.UseKalman, // 卡尔曼滤波开关

};

// 滑动组建菜单
struct SlideBar SlideControls[] = {
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    { (float*)&SystemMenuSaveData.ScreenBrightness, 0, 255, 5 }, // 屏幕亮度
    { (float*)&SystemMenuSaveData.UndervoltageAlert, 0, 36, 0.25 }, // 欠压提醒
    { (float*)&MenuScroll, 0, OLED_SCREEN_HEIGHT / 16, 1 }, // 当前菜单显示的位置（每页只显示4个，所以范围在0~3） 高度为16像素

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    { (float*)&T12ShutdownTime, 0, 600, 1 }, // 烙铁停机时间
    { (float*)&SystemMenuSaveData.ScreenProtectorTime, 0, 600, 1 }, // 屏保在休眠后的触发时间 (秒)

    { (float*)&HeatingConfig.curConfig.targetTemp, HeatMinTemp, HeatMaxTemp, 1 }, // 恒温模式目标输出温度(°C)

    // 温度曲线
    { (float*)&HeatingConfig.curConfig.PTemp[0], 0.5, 5, 0.5 }, // 升温斜率
    { (float*)&HeatingConfig.curConfig.PTemp[1], 0, HeatMaxTemp, 5 }, // 预热区温度(摄氏度)
    { (float*)&HeatingConfig.curConfig.PTemp[2], 0, 600, 5 }, // 预热区时间(秒)
    { (float*)&HeatingConfig.curConfig.PTemp[3], 0.5, 5, 0.5 }, // 升温斜率
    { (float*)&HeatingConfig.curConfig.PTemp[4], 0, HeatMaxTemp, 5 }, // 回流区温度(摄氏度)
    { (float*)&HeatingConfig.curConfig.PTemp[5], 0, 600, 5 }, // 回流区时间(秒)
    { (float*)&HeatingConfig.curConfig.PTemp[6], 0.5, 5, 0.5 }, // 降温斜率

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
    { (float*)&KalmanInfo[adc_HeatingTemp].parm.Cycle, 10, 1000, 1 },
    { (float*)&KalmanInfo[adc_HeatingTemp].parm.TempCompenstation, 0, 100, 1 },
    { (float*)&KalmanInfo[adc_HeatingTemp].parm.KalmanQ, 0, 10, 0.01 },
    { (float*)&KalmanInfo[adc_HeatingTemp].parm.KalmanR, 0, 10, 0.01 },

    { (float*)&KalmanInfo[adc_T12Temp].parm.Cycle, 10, 1000, 1 },
    { (float*)&KalmanInfo[adc_T12Temp].parm.TempCompenstation, 0, 100, 1 },
    { (float*)&KalmanInfo[adc_T12Temp].parm.KalmanQ, 0, 10, 0.01 },
    { (float*)&KalmanInfo[adc_T12Temp].parm.KalmanR, 0, 10, 0.01 },

    { (float*)&KalmanInfo[adc_T12Cur].parm.Cycle, 10, 1000, 1 },
    { (float*)&KalmanInfo[adc_T12Cur].parm.TempCompenstation, 0, 100, 1 },
    { (float*)&KalmanInfo[adc_T12Cur].parm.KalmanQ, 0, 10, 0.01 },
    { (float*)&KalmanInfo[adc_T12Cur].parm.KalmanR, 0, 10, 0.01 },

    { (float*)&KalmanInfo[adc_T12NTC].parm.Cycle, 10, 1000, 1 },
    { (float*)&KalmanInfo[adc_T12NTC].parm.TempCompenstation, 0, 100, 1 },
    { (float*)&KalmanInfo[adc_T12NTC].parm.KalmanQ, 0, 10, 0.01 },
    { (float*)&KalmanInfo[adc_T12NTC].parm.KalmanR, 0, 10, 0.01 },

    { (float*)&KalmanInfo[adc_SystemVol].parm.Cycle, 10, 1000, 1 },
    { (float*)&KalmanInfo[adc_SystemVol].parm.TempCompenstation, 0, 100, 1 },
    { (float*)&KalmanInfo[adc_SystemVol].parm.KalmanQ, 0, 10, 0.01 },
    { (float*)&KalmanInfo[adc_SystemVol].parm.KalmanR, 0, 10, 0.01 },

    { (float*)&KalmanInfo[adc_RoomTemp].parm.Cycle, 10, 1000, 1 },
    { (float*)&KalmanInfo[adc_RoomTemp].parm.TempCompenstation, 0, 100, 1 },
    { (float*)&KalmanInfo[adc_RoomTemp].parm.KalmanQ, 0, 10, 0.01 },
    { (float*)&KalmanInfo[adc_RoomTemp].parm.KalmanR, 0, 10, 0.01 },
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

/**
 * @brief 加载加热台当前配置
 *
 * @return true
 * @return false
 */
bool loadHeatDefConfig(int8_t index)
{
    if (0 <= index && index < HeatingConfig.heatingConfig.size()) {
        HeatingConfig.curConfigIndex = index;
        memcpy((void*)&HeatingConfig.curConfig, (void*)&HeatingConfig.heatingConfig[index], sizeof(HeatingConfig.curConfig));
        return true;
    }
    return false;
}

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
            ParamA : SwitchComponents_CurConfigIndex,
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
}

/**
 * @brief 新建一个配置
 *
 * @param pName 配置名称
 */
static void _newHeatConfig(const char* pName)
{
    _HeatingConfig config = {};
    strncpy(config.name, pName, sizeof(config.name));

    // 模式配置
    config.type = TYPE_HEATING_CONSTANT;

    // 回流焊配置
    config.PTemp[0] = 2.0f; // 升温斜率
    config.PTemp[1] = 120.0f; // 预热区温度(摄氏度)
    config.PTemp[2] = 100.0f; // 预热区时间(秒)
    config.PTemp[3] = 1.0f; // 升温斜率
    config.PTemp[4] = 250.0f; // 回流区温度(摄氏度)
    config.PTemp[5] = 60.0f; // 回流区时间(秒)
    config.PTemp[6] = 3.0f; // 降温斜率

    // 恒温模式目标输出温度(°C)
    config.targetTemp = 280.0f;

    // PID采样时间
    config.PIDSample = 100.0f;

    // PID切换温度
    config.PIDTemp = 10.0f; // 相差10度时开始切换

    // 爬升期 PID
    config.PID[0][0] = 2.0f;
    config.PID[0][1] = 0.2f;
    config.PID[0][2] = 1.0f;
    // 接近期 PID
    config.PID[1][0] = 1.0f;
    config.PID[1][1] = 0.05f;
    config.PID[1][2] = 0.25f;

    ////////////////////////
    // 添加
    HeatingConfig.heatingConfig.push_back(config);

    // 选择新建的配置
    HeatingConfig.curConfigIndex = HeatingConfig.heatingConfig.size() - 1;
    if (loadHeatDefConfig(HeatingConfig.curConfigIndex)) {
        printf("初始化默认配置成功 %d %s\n", HeatingConfig.curConfigIndex, HeatingConfig.curConfig.name);
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
            _newHeatConfig(name);

            if (loadHeatDefConfig(HeatingConfig.curConfigIndex)) {
                PopWindows("新建成功");
            }
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
    char name[128];
    if (loadHeatDefConfig(HeatingConfig.curConfigIndex)) {
        // 提示
        snprintf(name, sizeof(name), "载入[%s]成功", HeatingConfig.curConfig.name);
        PopWindows(name);
    } else {
        snprintf(name, sizeof(name), "载入失败");
        PopWindows(name);
    }
}

/**
 * @brief 保存当前配置数据到原始位置
 *
 */
void saveCurrentHeatData(void)
{
    if (-1 != HeatingConfig.curConfigIndex && HeatingConfig.curConfigIndex < HeatingConfig.heatingConfig.size()) {
        memcpy((void*)&HeatingConfig.heatingConfig[HeatingConfig.curConfigIndex], (void*)&HeatingConfig.curConfig, sizeof(HeatingConfig.curConfig));
    }
}

/**
 * @brief 重命名当前的配置
 *
 */
void renameHeatConfig(void)
{
    if (-1 != HeatingConfig.curConfigIndex && HeatingConfig.curConfigIndex < HeatingConfig.heatingConfig.size()) {
        char name[20] = { 0 };
        strncpy(name, HeatingConfig.heatingConfig[HeatingConfig.curConfigIndex].name, sizeof(name));
        TextEditor("[重命名配置]", name, sizeof(name));

        if (strlen(name) > 0) {
            strncpy(HeatingConfig.heatingConfig[HeatingConfig.curConfigIndex].name, name, sizeof(HeatingConfig.heatingConfig[HeatingConfig.curConfigIndex].name));
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

        PopWindows("删除成功");

        // 选择第0个配置
        HeatingConfig.curConfigIndex = 0;

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
                                         { Type_Slider, "欠压提醒(V)", Set6, SlideComponents_UndervoltageAlert, 0, NULL },
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

    /////////////////////////////////////////////////////
    // 三级菜单
    { 110, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "显示效果", NULL, 101, 1, NULL },
                                         { Type_GotoMenu, "面板设置", Set0, 112, 0, NULL },
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
                                         { Type_MenuName, "面板设置", NULL, 110, 1, NULL },
                                         { Type_CheckBox, "简约", Set17, SwitchComponents_PanelSettings, 0, *JumpWithTitle },
                                         { Type_CheckBox, "详细", Set18, SwitchComponents_PanelSettings, 1, *JumpWithTitle },
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
                                         { Type_GotoMenu, "加热台管理", IMG_Tip, 201, 0, NULL },
                                         { Type_GotoMenu, "卡尔曼滤波器", kalmanImg, 202, 0, NULL },
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

                                        // 环境温度
                                        { Type_Switch, "环境温度-状态", NULL, SwitchComponents_KFP6, 0, NULL },
                                        { Type_Slider, "采样周期(ms)", NULL, SlideComponents_ADC_Cycle6, 0, NULL },
                                        { Type_Slider, "温度补偿", NULL, SlideComponents_TempComp_6, 0, NULL },
                                        { Type_Slider, "过程噪声协方差", NULL, SlideComponents_KFP_Q6, 0, NULL },
                                        { Type_Slider, "观察噪声协方差", NULL, SlideComponents_KFP_R6, 0, NULL },
                                        { Type_MenuName, "----------", NULL, 200, 2, NULL },
                                        { Type_ReturnMenu, "返回", NULL, 200, 2, NULL },
                                    } },

    { 203, 0, 0, 0, MenuRenderImage, {
                                         { Type_MenuName, "定时场景", NULL, 200, 3, NULL },
                                         { Type_Slider, "T12停机(s)", Set13, SlideComponents_ShutdownTime, 0, NULL }, // 烙铁停机时间
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
                                        { Type_Slider, "升温斜率", NULL, SlideComponents_PTemp_0, 0, NULL },
                                        { Type_Slider, "预热区温度", NULL, SlideComponents_PTemp_1, 0, NULL },
                                        { Type_Slider, "预热区时间", NULL, SlideComponents_PTemp_2, 0, NULL },
                                        { Type_Slider, "升温斜率", NULL, SlideComponents_PTemp_3, 0, NULL },
                                        { Type_Slider, "回流区温度", NULL, SlideComponents_PTemp_4, 0, NULL },
                                        { Type_Slider, "回流区时间", NULL, SlideComponents_PTemp_5, 0, NULL },
                                        { Type_Slider, "降温斜率", NULL, SlideComponents_PTemp_6, 0, NULL },
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
                                        { Type_CheckBox, "T12焊笔", NULL, SwitchComponents_HeatMoe, 2, NULL },
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
    printf("音量:%d 面板设置:%d 固定方式:%d 屏幕亮度:%1.1f 欠压提醒:%1.1f\n", SystemMenuSaveData.Volume, SystemMenuSaveData.PanelSettings, SystemMenuSaveData.OptionStripFixedLength_Flag, SystemMenuSaveData.ScreenBrightness, SystemMenuSaveData.UndervoltageAlert);
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

    // 只有一个配置的话 就新建
    if (0 == HeatingConfig.heatingConfig.size()) {
        _newHeatConfig("defConfig");
        settings_write_all();
    } else {
        loadHeatDefConfig(HeatingConfig.curConfigIndex);
    }

    // 初始化shell命令
    register_cmd("dumpmenu", "打印出所有菜单内容", NULL, do_dumpmenu_cmd, NULL);
}

/**
 * @brief 获得当前的加热台配置
 *
 * @return _HeatingConfig
 */
_HeatingConfig* getCurrentHeatingConfig(void)
{
    return &HeatingConfig.curConfig;
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
 * @brief 获取设置欠压设置
 *
 * @return float
 */
float getSystemVoltage(void)
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
