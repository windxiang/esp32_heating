#pragma once

#include <vector>
#include <string>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 加热台 相关
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//卡尔曼滤波
typedef struct
{
    float LastP; //上次估算协方差 初始化值为0.02
    float Now_P; //当前估算协方差 初始化值为0
    float out; //卡尔曼滤波器输出 初始化值为0
    float Kg; //卡尔曼增益 初始化值为0
    float Q; //过程噪声协方差 初始化值为0.001
    float R; //观测噪声协方差 初始化值为0.543
} KFP; // Kalman Filter parameter

/**
 * @brief 加热台 T12类型配置
 *
 */
enum HEATINGTYPE {
    TYPE_HEATING_CONSTANT, // 加热台 恒温模式
    TYPE_HEATING_VARIABLE, // 加热台 回流焊
    TYPE_T12, // T12
};

/**
 * @brief 加热台 T12 配置参数
 *
 */
struct _HeatingConfig {
    std::string name; // 名称
    HEATINGTYPE type; // 类型
    float PTemp[7]; // 温度曲线
    float PID[2][3]; // PID系数{远PID，近PID}
};

/**
 * @brief 加热台 T12烙铁配置
 *
 */
struct _HeatingSystem {
    int8_t maxConfig; // 最大配置
    int8_t curConfigIndex; // 当前使用配置索引
    _HeatingConfig curConfig; // 当前使用的配置参数
    std::vector<_HeatingConfig> heatingConfig; // 子配置
};
extern _HeatingSystem HeatingConfig;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 系统菜单
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 系统菜单数据
 *
 */
struct _SystemMenuSaveData {
    uint8_t Language; // 系统语言
    uint8_t BLEState; // 蓝牙开关
    uint8_t ScreenFlip; // 屏幕翻转
    uint8_t MenuListMode; // 1:菜单使用文本渲染方式 不使用图标、 0:混合显示模式
    uint8_t SmoothAnimationFlag; // 菜单动画标志位
    uint8_t Volume; // 编码器声音设置
    uint8_t PanelSettings; // 面板设置 详细面板 简约面板
    uint8_t OptionStripFixedLength_Flag; // 选项条固定 自适应
    float ScreenBrightness; // 屏幕亮度
    float UndervoltageAlert; // 系统电压 欠压警告阈值 (单位V)
    char BLEName[20]; // 蓝牙设备名称
    char BootPasswd[20]; // 开机密码
};
extern _SystemMenuSaveData SystemMenuSaveData;

/**
 * @brief 开关控件 用于存储开关控件的值
 *
 */
enum Switch_space_Obj {
    ///////////////////////////////////////////////
    SwitchSpace_Language, // 语言设置
    SwitchSpace_BLE_State, // 蓝牙开关
    SwitchSpace_ScreenFlip, // 翻转OLED
    SwitchSpace_MenuListMode, // 图标模式 列表模式
    SwitchSpace_SmoothAnimation, // 过度动画标志位
    SwitchSpace_Volume, // 声音设置
    SwitchSpace_PanelSettings, // 面板设置 详细面板 简约面板
    SwitchSpace_OptionWidth, // 选项条固定 自适应
    // SwitchSpace_RotaryDirection, // 编码器反向

    ///////////////////////////////////////////////
    SwitchSpace_PIDMode,
    SwitchSpace_KFP,
    SwitchSpace_HandleTrigger,
    SwitchSpace_CurConfigIndex, // 当前加热台配置索引
};

/**
 * @brief 滑动条控件 用于存储滑动条控件的值
 *
 */
enum Slide_space_Obj {
    ///////////////////////////////////////////////
    Slide_space_ScreenBrightness, // 屏幕亮度
    Slide_space_UndervoltageAlert, // 欠压提醒
    Slide_space_Scroll, // 文本渲染模式下使用 每页显示4个条目 中的第几条
    ///////////////////////////////////////////////

    Slide_space_BootTemp, // 启动温度
    Slide_space_SleepTemp, // 休眠温度
    // Slide_space_BoostTemp,

    Slide_space_ShutdownTime, // 停机触发(分)
    Slide_space_SleepTime, // 休眠触发(分)
    // Slide_space_BoostTime,
    Slide_space_ScreenProtectorTime, // 屏保触发(秒)

    Slide_space_KFP_Q, // 过程噪声协方差
    Slide_space_KFP_R, // 观察噪声协方差

    // Slide_space_SamplingRatioWork,
    Slide_space_ADC_PID_Cycle_List_0, // 温差>150
    Slide_space_ADC_PID_Cycle_List_1, // 温差>50
    Slide_space_ADC_PID_Cycle_List_2, // 温差≤50

    // 温度曲线
    Slide_space_PTemp_0, // 升温斜率
    Slide_space_PTemp_1, // 预热区温度(摄氏度)
    Slide_space_PTemp_2, // 预热区时间(秒)
    Slide_space_PTemp_3, // 升温斜率
    Slide_space_PTemp_4, // 回流区温度(摄氏度)
    Slide_space_PTemp_5, // 回流区时间(秒)
    Slide_space_PTemp_6, // 降温斜率

    // PID参数
    Slide_space_PID_AP, // 比例P(爬升期)
    Slide_space_PID_AI, // 积分I
    Slide_space_PID_AD, // 微分D
    Slide_space_PID_CP, // 比例P(接近期)
    Slide_space_PID_CI, // 积分I
    Slide_space_PID_CD, // 微分D
};

// 滑动菜单结构体
struct SlideBar {
    float* val; //值
    float min; // 最小值
    float max; // 最大值
    float step; // 步进
};

/**
 * @brief 菜单动画结构体
 *
 */
struct MenuSmoothAnimation {
    float result; // 动画计算结果
    float last; // 上次的值
    float val; // 目标值
    float SlidingWeights; // 平滑权重
    uint8_t isTotalization; // 是否允许累加
};

// 渲染类型
typedef enum {
    MenuRenderText = 0, // 文本渲染
    MenuRenderImage, // 图表化渲染
} _RenderType;

/**
 * @brief 首页面板显示模式
 *
 */
enum PANELSET {
    PANELSET_Simple = 0, // 简约模式
    PANELSET_Detailed, // 详细模式
};

enum HANDLETRIGGER {
    HANDLETRIGGER_VibrationSwitch = 0,
    HANDLETRIGGER_ReedSwitch,
};

// 菜单类型定义
typedef enum {
    Type_GotoMenu, // 跳转到菜单
    Type_RunFunction, // 执行函数
    Type_MenuName, // 菜单名 默认都是返回上一层菜单
    Type_Switch, // 开关控件
    Type_Slider, // 滑动条
    Type_CheckBox, // 单选框
    Type_ReturnMenu, // 返回上一级菜单
    Type_NULL, // 无类型
} _MenuType;

// 子菜单定义
struct subMenu {
    _MenuType type; // 菜单类型
    std::string name;
    uint8_t* icon;
    uint8_t ParamA; //附加参数A (Type_GotoMenu-打开对应的lid) (Type_Switch-开关id) (Type_Slider-滑动条id)
    uint8_t ParamB; //附加参数B (Type_GotoMenu-打开对应的图标) (Type_Slider-滑动条：true?执行函数:无操作)
    void (*function)();
};

/**
 * @brief 定义菜单的结构体
 *
 */
struct menuSystem {
    int16_t menuID; // 当前菜单ID (设置此变量原因, 可以不连续)
    int8_t index; //  当前显示哪个图标
    int8_t minMenuSize; // 最小从哪个菜单开始渲染显示
    int8_t maxMenuSize; // 最大菜单个数
    _RenderType RenderType; // 渲染类型
    std::vector<subMenu> subSystem; // 子菜单
};

extern uint8_t* SwitchControls[];
extern struct SlideBar SlideControls[];
extern struct MenuSmoothAnimation menuSmoothAnimation[];
extern const int sizeMenuSmoothAnimatioin;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void initMenuSystem(void);
menuSystem* getCurRenderMenu(int menuID);
subMenu* getSubMenu(menuSystem* pMenuRoot, int id);

#ifdef __cplusplus
}
#endif // __cplusplus