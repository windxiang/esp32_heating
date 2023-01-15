#include "heating.h"
#include <string.h>
#include "argtable3/argtable3.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Shell 命令参数
/**
 * @brief PWM 命令结构体
 *
 */
static struct {
    struct arg_int* channel;
    struct arg_dbl* q;
    struct arg_dbl* r;
    struct arg_end* end;
} kalman_cmd_args = {};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 各模块相关变量
_HeatingSystem HeatingConfig = {
    maxConfig : 10, // 最大配置数量
    curConfigIndex : -1, // 当前使用配置索引
    systemConfig : {
        HeatMinTemp : 0.0f,
        HeatMaxTemp : 300.0f,
        T12MinTemp : 0.0f,
        T12MaxTemp : 300.0f,
        T12IdleTime : 30.0f,
        T12IdleTemp : 150.0f,
        T12StopTime : 120.0f,
    }, // 系统配置
    curConfig : {}, // 当前使用的配置参数
    heatingConfig : {},
};

// 卡尔曼滤波配置
_KalmanInfo KalmanInfo[adc_last_max] = {
    // 加热台
    [adc_HeatingTemp] = {
        .filter = { 0.02, 0, 0, 0 },
        .parm = { 1, 200, 0, 0.1f, 0.1f },
    },
    // T12 烙铁头温度
    [adc_T12Temp] = {
        .filter = { 0.02, 0, 0, 0 },
        .parm = { 1, 100, 0, 0.1f, 0.1f },
    },
    // T12 电流
    [adc_T12Cur] = {
        .filter = { 0.02, 0, 0, 0 },
        .parm = { 1, 100, 0, 0.1f, 0.1f },
    },
    // T12 NTC
    [adc_T12NTC] = {
        .filter = { 0.02, 0, 0, 0 },
        .parm = { 1, 100, 0, 0.1f, 0.1f },
    },
    // 系统电压
    [adc_SystemVol] = {
        .filter = { 0.02, 0, 0, 0 },
        .parm = { 1, 100, 0, 0.1f, 0.1f },
    },
    // 5V电压
    [adc_SystemRef] = {
        .filter = { 0.02, 0, 0, 0 },
        .parm = { 1, 100, 0, 0.1f, 0.1f },
    },
    // 室温
    [adc_RoomTemp] = {
        .filter = { 0.02, 0, 0, 0 },
        .parm = { 1, 100, 0, 0.1f, 0.1f },
    },
};

// T12 震动开关检测配置
uint8_t HandleTrigger = HANDLETRIGGER_VibrationSwitch;

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
void _newHeatConfig(const char* pName)
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
 * @brief 初始化最小最大温度值
 *
 */
void flushMaxTemp(void)
{
    if (HeatingConfig.curConfig.type == TYPE_HEATING_VARIABLE || HeatingConfig.curConfig.type == TYPE_HEATING_CONSTANT) {
        // 加热台 回流焊
        SlideControls[SlideComponents_TargetTemp].max = HeatingConfig.systemConfig.HeatMaxTemp;
        SlideControls[SlideComponents_PTemp_WarmUpTemp].max = HeatingConfig.systemConfig.HeatMaxTemp;
        SlideControls[SlideComponents_PTemp_RefloatTemp].max = HeatingConfig.systemConfig.HeatMaxTemp;
    } else {
        // T12
        SlideControls[SlideComponents_TargetTemp].max = HeatingConfig.systemConfig.T12MaxTemp;
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
        flushMaxTemp();
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

/**
 * @brief 卡尔曼滤波参数修改
 *
 * @param argc
 * @param argv
 * @return int
 */
static int do_kalman_cmd(int argc, char** argv)
{
    int nerrors = arg_parse(argc, argv, (void**)&kalman_cmd_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, kalman_cmd_args.end, argv[0]);
        return 1;
    }

    if (0 <= *kalman_cmd_args.channel->ival && *kalman_cmd_args.channel->ival < adc_last_max) {
        _KalmanInfo* pInfo = &KalmanInfo[*kalman_cmd_args.channel->ival];
        pInfo->parm.KalmanQ = *kalman_cmd_args.q->dval;
        pInfo->parm.KalmanR = *kalman_cmd_args.r->dval;
        printf("通道%d 修改成功 Q:%f R:%f\r\n", *kalman_cmd_args.channel->ival, pInfo->parm.KalmanQ, pInfo->parm.KalmanR);

    } else {
        printf("\r\n 修改错误 不存在的通道\r\n");
    }
    return 0;
}

/**
 * @brief 初始化扩展菜单
 *
 */
void initMenuExpand(void)
{
    // 只有一个配置的话 就新建
    if (0 == HeatingConfig.heatingConfig.size()) {
        _newHeatConfig("defConfig");
        settings_write_all();
    } else {
        loadHeatDefConfig(HeatingConfig.curConfigIndex);
    }

    kalman_cmd_args.channel = arg_intn("c", "channel", "<n>", 1, 1, "modify channel");
    kalman_cmd_args.q = arg_dbln("q", "q", "<n>", 1, 1, "modify q");
    kalman_cmd_args.r = arg_dbln("r", "r", "<n>", 1, 1, "modify r");
    kalman_cmd_args.end = arg_end(20);
    register_cmd("kalman", "kalaman参数配置", NULL, do_kalman_cmd, &kalman_cmd_args);
}

/**
 * @brief 获取加热台温度配置
 *
 * @return _HeatingSystem
 */
_HeatSystemConfig* getHeatingSystemConfig(void)
{
    return &HeatingConfig.systemConfig;
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
