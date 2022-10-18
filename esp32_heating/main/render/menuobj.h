#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*
    @brief 开关控件 用于存储开关控件的值
    @注册
        -0->平滑动画
        -1->单选框测试
        -2->选项条固定长度
*/
enum Switch_space_Obj {
    SwitchSpace_SmoothAnimation = 0, // 编码器滑动菜单 动画标志位
    SwitchSpace_OptionStripFixedLength,

    SwitchSpace_PIDMode,
    SwitchSpace_KFP,
    SwitchSpace_PanelSettings, // 面板设置
    SwitchSpace_ScreenFlip,
    SwitchSpace_Volume,
    // SwitchSpace_RotaryDirection,
    SwitchSpace_HandleTrigger,
    SwitchSpace_Language,
    SwitchSpace_TipID,

    SwitchSpace_BLE_State,
    SwitchSpace_MenuListMode,
};

/*
    @brief 滑动条控件 用于存储滑动条控件的值
    @param -

    @注册
        -0->屏幕亮度设定值
        -1->自适应菜单滚动范围
    @变量
        int   x    值
        int   min  最小值
        int   max  最大值
        int   step 步进
*/
enum Slide_space_Obj {
    Slide_space_ScreenBrightness, // 屏幕亮度
    Slide_space_Scroll, // 文本渲染模式下使用 每页显示4个条目 中的第几条

    Slide_space_BootTemp, // 启动温度
    Slide_space_SleepTemp, // 休眠温度
    // Slide_space_BoostTemp,

    Slide_space_ShutdownTime, // 停机触发(分)
    Slide_space_SleepTime, // 休眠触发(分)
    // Slide_space_BoostTime,
    Slide_space_ScreenProtectorTime, // 屏保触发(秒)

    Slide_space_UndervoltageAlert, // 欠压提醒

    Slide_space_PID_AP, // 比例P
    Slide_space_PID_AI, // 积分I
    Slide_space_PID_AD, // 微分D
    Slide_space_PID_CP, // 比例P
    Slide_space_PID_CI, // 积分I
    Slide_space_PID_CD, // 微分D

    Slide_space_KFP_Q, // 过程噪声协方差
    Slide_space_KFP_R, // 观察噪声协方差

    // Slide_space_SamplingRatioWork,
    Slide_space_ADC_PID_Cycle_List_0, // 温差>150
    Slide_space_ADC_PID_Cycle_List_1, // 温差>50
    Slide_space_ADC_PID_Cycle_List_2, // 温差≤50

    Slide_space_PTemp_0, // 升温斜率
    Slide_space_PTemp_1, // 预热区温度
    Slide_space_PTemp_2, // 预热区时间
    Slide_space_PTemp_3, // 升温斜率
    Slide_space_PTemp_4, // 回流区温度
    Slide_space_PTemp_5, // 回流区时间
    Slide_space_PTemp_6, // 降温斜率
};

// 滑动菜单结构体
struct SlideBar {
    float* val; //值
    float min; // 最小值
    float max; // 最大值
    float step; // 步进
};

/**
 * @brief 菜单滑动动画结构体
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

struct MenuLevelSystem {
    int8_t id; // 当前层id
    int8_t index; // 对应的图标id
    int8_t min; // 最小个数
    int8_t max; // 最大个数
    _RenderType RenderType; // 0:无动作 1:开启图标化菜单
};

// 菜单类型定义
typedef enum {
    Type_GotoMenu = 0, // 跳转到菜单
    Type_RunFunction = 1, // 执行函数
    Type_MenuName = 2, // 菜单名 默认都是返回上一层菜单
    Type_Switch = 3, // 开关控件
    Type_Slider = 4, // 滑动条
    Type_CheckBox = 5, // 单选框
    Type_NULL = 6, // 无类型
} _MenuType;

struct MenuSystem {
    uint8_t lid; //层id
    uint8_t id; //选项id
    _MenuType type; // 菜单类型
    const char* name; //选项名称
    uint8_t* icon;
    uint8_t ParamA; //附加参数A (Type_GotoMenu-打开对应的lid) (Type_Switch-开关id) (Type_Slider-滑动条id)
    uint8_t ParamB; //附加参数B (Type_GotoMenu-打开对应的图标) (Type_Slider-滑动条：true?执行函数:无操作)
    void (*function)();
};

//最大烙铁头配置数量
#define MaxTipConfig 10

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

enum PANELSET {
    PANELSET_Simple = 0,
    PANELSET_Detailed,
};

enum HANDLETRIGGER {
    HANDLETRIGGER_VibrationSwitch = 0,
    HANDLETRIGGER_ReedSwitch,
};

extern uint8_t* SwitchControls[];
extern struct SlideBar SlideControls[];
extern struct MenuSmoothAnimation menuSmoothAnimation[];
extern struct MenuLevelSystem MenuLevel[];
extern struct MenuSystem Menu[];

extern const int sizeMenuLevel;
extern const int sizeMenu;
extern const int sizeMenuSmoothAnimatioin;

extern uint8_t MenuListMode;

#ifdef __cplusplus
}
#endif // __cplusplus