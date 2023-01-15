#pragma once

#include <vector>
#include <string>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 加热台 相关
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// 卡尔曼滤波计算
typedef struct
{
    float LastP; // 上次估算协方差 初始化值为0.02 可以随便设置  但是不能为0（为0的话卡尔曼滤波器就认为已经是最优滤波器了）
    float Now_P; // 当前估算协方差 初始化值为0
    float out; // 卡尔曼滤波器输出 初始化值为0
    float Kg; // 卡尔曼增益 初始化值为0
} _KalmanFilter;

// 卡尔曼滤波参数 可以在菜单里面进行设置
typedef struct {
    uint8_t UseKalman; // 使用卡尔曼
    float Cycle; // ADC采样周期(ms)
    float calibrationVal; // 校准补偿
    float KalmanQ; // 过程噪声协方差  Q增大，动态响应变快(越跟随传感器)，收敛稳定性变坏
    float KalmanR; // 观测噪声协方差  R增大，动态响应变慢(滞后性变大)，收敛稳定性变好
    // Q, R 讲白了就是(买的破温度计有多破，以及你的超人力有多强)
    // Q参数 调整滤波后的 曲线平滑程度，Q越小越平滑、 Q值越大跟随传感器速度越快 曲线越不平滑
    // R参数 调整滤波后的 曲线与实测曲线的相近程度：R越小越接近、R越大滞后性越明显并且曲线越平滑
} _KalmanParm;

typedef struct {
    _KalmanFilter filter;
    _KalmanParm parm;
} _KalmanInfo;

extern _KalmanInfo KalmanInfo[];

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 加热台的系统配置
 *
 */
struct _HeatSystemConfig {
    float HeatMinTemp; // 加热台最小温度
    float HeatMaxTemp; // 加热台最大温度
    float T12MinTemp; // T12最小温度
    float T12MaxTemp; // T12最大温度
    float T12IdleTime; // T12降温时间(s)
    float T12IdleTemp; // T12降温温度
    float T12StopTime; // T12停机时间(s)
};

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
    char name[20]; // 名称
    HEATINGTYPE type; // 类型
    float PTemp[7]; // 温度曲线
    float PIDSample; // PID采样时间
    float PIDTemp; // 远近PID切换温度
    float PID[2][3]; // PID系数[远PID，近PID]
    float targetTemp; // 恒温模式目标输出温度(°C)
};

/**
 * @brief 加热台 T12烙铁配置
 *
 */
struct _HeatingSystem {
    int8_t maxConfig; // 最大配置
    int8_t curConfigIndex; // 当前使用配置索引
    _HeatSystemConfig systemConfig; // 系统配置
    _HeatingConfig curConfig; // 当前使用的配置参数
    std::vector<_HeatingConfig> heatingConfig; // 子配置
};
extern _HeatingSystem HeatingConfig;

/**
 * @brief 回流焊曲线定义
 *
 */
enum _ReflowSolder {
    WarmUpRampRate = 0, // 预热区 升温斜率
    WarmUpTemp, // 预热区 温度(摄氏度)
    WarmUpTime, // 预热区 时间(s)
    RefloatRampRate, // 回流区 升温斜率
    RefloatTemp, // 回流区 温度(摄氏度)
    RefloatTime, // 回流区 时间(s)
    CoolDownTime, // 降温斜率
};

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
    float ScreenProtectorTime; // 屏保在休眠后的触发时间 (秒)
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
enum SwitchComponents_Obj {
    ///////////////////////////////////////////////
    SwitchComponents_Language, // 语言设置
    SwitchComponents_BLE_State, // 蓝牙开关
    SwitchComponents_ScreenFlip, // 翻转OLED
    SwitchComponents_MenuListMode, // 图标模式 列表模式
    SwitchComponents_SmoothAnimation, // 过度动画标志位
    SwitchComponents_Volume, // 声音设置
    SwitchComponents_PanelSettings, // 面板设置 详细面板 简约面板
    SwitchComponents_OptionWidth, // 选项条固定 自适应
    // SwitchComponents_RotaryDirection, // 编码器反向

    ///////////////////////////////////////////////
    SwitchComponents_CurConfigIndex, // 当前加热台配置索引
    SwitchComponents_HeatMoe, // 恒温加热台 回流焊加热台 T12加热台
    SwitchComponents_T12VibrationDetection, // T12 震动开关检测配置

    // 卡尔曼滤波开关
    SwitchComponents_KFP1, // 卡尔曼滤波开关
    SwitchComponents_KFP2, // 卡尔曼滤波开关
    SwitchComponents_KFP3, // 卡尔曼滤波开关
    SwitchComponents_KFP4, // 卡尔曼滤波开关
    SwitchComponents_KFP5, // 卡尔曼滤波开关
    SwitchComponents_KFP6, // 卡尔曼滤波开关
    SwitchComponents_KFP7, // 卡尔曼滤波开关
};

/**
 * @brief 滑动条控件 用于存储滑动条控件的值
 *
 */
enum SlideComponents_Obj {
    ///////////////////////////////////////////////
    // 系统配置
    SlideComponents_ScreenBrightness, // 屏幕亮度
    SlideComponents_UndervoltageAlert, // 欠压提醒(V)
    SlideComponents_Scroll, // 文本渲染模式下使用 每页显示4个条目 中的第几条
    SlideComponents_ScreenProtectorTime, // 屏保触发(秒)
    ///////////////////////////////////////////////

    // 加热台 T12 配置

    // 其他设置
    SlideComponents_TargetTemp, // 恒温模式目标输出温度(°C)

    // 温度曲线
    SlideComponents_PTemp_WarmUpRampRate, // 预热区 升温斜率
    SlideComponents_PTemp_WarmUpTemp, // 预热区 温度(摄氏度)
    SlideComponents_PTemp_WarmUpTime, // 预热区 维持时间(秒)
    SlideComponents_PTemp_RefloatRampRate, // 回流区 升温斜率
    SlideComponents_PTemp_RefloatTemp, // 回流区 温度(摄氏度)
    SlideComponents_PTemp_RefloatTime, // 回流区 维持时间(秒)
    SlideComponents_PTemp_CoolDownTime, // 降温斜率

    // PID参数
    SlideComponents_PIDSample, // PID采样时间
    SlideComponents_SwitchTemp, // PID温度切换度数
    SlideComponents_PID_AP, // 比例P(爬升期)
    SlideComponents_PID_AI, // 积分I
    SlideComponents_PID_AD, // 微分D
    SlideComponents_PID_CP, // 比例P(接近期)
    SlideComponents_PID_CI, // 积分I
    SlideComponents_PID_CD, // 微分D

    // 卡尔曼滤波
    SlideComponents_ADC_Cycle1, // 热电偶采样周期(ms)
    SlideComponents_TempComp_1, // 温度补偿
    SlideComponents_KFP_Q1, // 过程噪声协方差
    SlideComponents_KFP_R1, // 观察噪声协方差

    SlideComponents_ADC_Cycle2, // T12温度
    SlideComponents_TempComp_2, // 温度补偿
    SlideComponents_KFP_Q2, // 过程噪声协方差
    SlideComponents_KFP_R2, // 观察噪声协方差

    SlideComponents_ADC_Cycle3, // T12电流
    SlideComponents_TempComp_3, // 温度补偿
    SlideComponents_KFP_Q3, // 过程噪声协方差
    SlideComponents_KFP_R3, // 观察噪声协方差

    SlideComponents_ADC_Cycle4, // T12 NTC
    SlideComponents_TempComp_4, // 温度补偿
    SlideComponents_KFP_Q4, // 过程噪声协方差
    SlideComponents_KFP_R4, // 观察噪声协方差

    SlideComponents_ADC_Cycle5, // 系统输入电压
    SlideComponents_TempComp_5, // 温度补偿
    SlideComponents_KFP_Q5, // 过程噪声协方差
    SlideComponents_KFP_R5, // 观察噪声协方差

    SlideComponents_ADC_Cycle6, // 5V电压
    SlideComponents_TempComp_6, // 温度补偿
    SlideComponents_KFP_Q6, // 过程噪声协方差
    SlideComponents_KFP_R6, // 观察噪声协方差

    SlideComponents_ADC_Cycle7, // 室温
    SlideComponents_TempComp_7, // 温度补偿
    SlideComponents_KFP_Q7, // 过程噪声协方差
    SlideComponents_KFP_R7, // 观察噪声协方差

    // 最小 最大温度值
    SlideComponents_HeatMinTemp, // 加热台最小最大温度
    SlideComponents_HeatMaxTemp,
    SlideComponents_T12MinTemp, // T12最小最大温度
    SlideComponents_T12MaxTemp,
    // T12保护设置
    SlideComponents_IdleTime, // T12降温时间(s)
    SlideComponents_IdleTemp, // T12降温温度
    SlideComponents_StopTime, // T12停机时间(s)
};

// 滑动菜单结构体
struct SlideBar {
    float* val; // 值
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
    PANELSET_Trend, // 趋势图
};

// T12 震动开关
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
    uint8_t ParamA; // 附加参数A (Type_GotoMenu-打开对应的lid) (Type_Switch-开关id) (Type_Slider-滑动条id)
    uint8_t ParamB; // 附加参数B (Type_GotoMenu-打开对应的图标) (Type_Slider-滑动条：true?执行函数:无操作)
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

float getScreenProtectorTime(void);
float getSystemUndervoltageAlert(void);
uint8_t getBlueToolsStatus(void);
uint8_t getVolume(void);
void JumpWithTitle(void);

#ifdef __cplusplus
}
#endif // __cplusplus
