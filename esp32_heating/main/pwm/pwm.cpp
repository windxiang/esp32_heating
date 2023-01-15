#include "heating.h"
#include <driver/ledc.h>
#include <math.h>
#include "argtable3/argtable3.h"

#define DUTYRESOLUTION1 LEDC_TIMER_12_BIT // 分辨率
#define DUTYRESOLUTION2 LEDC_TIMER_14_BIT

static const ledc_timer_config_t pwmTimer[] = {
    {
        .speed_mode = LEDC_HIGH_SPEED_MODE, // timer mode
        .duty_resolution = DUTYRESOLUTION1, // resolution of PWM duty
        .timer_num = LEDC_TIMER_0, // timer index
        .freq_hz = 10, // frequency of PWM signal
        .clk_cfg = LEDC_AUTO_CLK, // Auto select the source clock
    },
    // 用于BEEP 修改freq
    {
        .speed_mode = LEDC_HIGH_SPEED_MODE, // timer mode
        .duty_resolution = DUTYRESOLUTION2, // resolution of PWM duty
        .timer_num = LEDC_TIMER_1, // timer index
        .freq_hz = 1 * 1000, // frequency of PWM signal
        .clk_cfg = LEDC_AUTO_CLK, // Auto select the source clock
    },
};

static const ledc_channel_config_t pwm_channel[_TYPE_MAX] = {
    // PWM T12
    // [_TYPE_T12] = {
    //     .gpio_num = PIN_PWM_T12,
    //     .speed_mode = LEDC_HIGH_SPEED_MODE,
    //     .channel = LEDC_CHANNEL_0,
    //     .intr_type = LEDC_INTR_DISABLE,
    //     .timer_sel = LEDC_TIMER_0,
    //     .duty = 0,
    //     .hpoint = 0,
    //     .flags = {
    //         .output_invert = 0,
    //     },
    // },
    // PWM 风扇
    [_TYPE_FAN] = {
        .gpio_num = PIN_PWM_FAN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .flags = {
            .output_invert = 0,
        },
    },
    // PWM 加热台
    [_TYPE_HEAT] = {
        .gpio_num = PIN_PWM_HEAT,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_2,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .flags = {
            .output_invert = 0,
        },
    },
    // PWM 蜂鸣器
    [_TYPE_BEEP] = {
        .gpio_num = PIN_PWM_BEEP,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_3,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_1, // 通道1
        .duty = 0,
        .hpoint = 0,
        .flags = {
            .output_invert = 0,
        },
    }
};

/**
 * @brief PWM 命令结构体
 *
 */
static struct {
    struct arg_int* type;
    struct arg_int* val;
    struct arg_end* end;
} pwm_cmd_args = {};

/**
 * @brief BEEP 命令结构体
 *
 */
static struct {
    struct arg_int* freq;
    struct arg_end* end;
} freq_cmd_args = {};

/**
 * @brief 蜂鸣器输出,固定50%占空比
 *
 * @param freq 要输出的频率
 * @return true
 * @return false
 */
bool beepOutput(uint32_t freq)
{
    const ledc_channel_config_t* pPWMChannel = &pwm_channel[_TYPE_BEEP];
    const uint32_t duty = 8192; // (2 ^ 14 - 1) / 2

    if (freq > 0 && ESP_OK == ledc_set_freq(pPWMChannel->speed_mode, pPWMChannel->timer_sel, freq)) {
        ledc_set_duty(pPWMChannel->speed_mode, pPWMChannel->channel, duty);
        ledc_update_duty(pPWMChannel->speed_mode, pPWMChannel->channel);

    } else {
        ledc_set_duty(pPWMChannel->speed_mode, pPWMChannel->channel, 0);
        ledc_update_duty(pPWMChannel->speed_mode, pPWMChannel->channel);
        return false;
    }

    return true;
}

/**
 * @brief 设置PWM输出占空比
 *
 * @param type 输出通道
 * @param value 占空比
 * @return true 设置成功
 * @return false 设置失败
 */
bool pwmOutput(_PWMTYPE type, uint16_t value)
{
    if (type >= _TYPE_MAX) {
        return false;
    }

    const ledc_channel_config_t* pPWMChannel = &pwm_channel[type];

    if (value > 100)
        value = 100;

    // uint32_t duty = (uint32_t)((float)value / 100.0f * (float)(pow(2, (int)DUTYRESOLUTION1) - 1));
    uint32_t duty = (uint32_t)((float)value / 100.0f * 4095.0f); // 这里的 就是 4095 = pow(2, (int)DUTYRESOLUTION1) - 1;   因为是12位分辨率

    // 直接修改PWM
    ledc_set_duty(pPWMChannel->speed_mode, pPWMChannel->channel, duty);
    ledc_update_duty(pPWMChannel->speed_mode, pPWMChannel->channel);

    return true;
}

/**
 * @brief PWM 配置命令
 *
 * @param argc
 * @param argv
 * @return int
 */
static int do_pwm_cmd(int argc, char** argv)
{
    int nerrors = arg_parse(argc, argv, (void**)&pwm_cmd_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, pwm_cmd_args.end, argv[0]);
        return 1;
    }

    if (pwmOutput((_PWMTYPE)*pwm_cmd_args.type->ival, *pwm_cmd_args.val->ival)) {
        printf("通道:%d 输出:%d%% 设置成功\n", *pwm_cmd_args.type->ival, *pwm_cmd_args.val->ival);
    } else {
        printf("设置失败 通道号错误\n");
    }
    return 0;
}

/**
 * @brief BEEP 配置命令
 *
 * @param argc
 * @param argv
 * @return int
 */
static int do_beep_cmd(int argc, char** argv)
{
    int nerrors = arg_parse(argc, argv, (void**)&freq_cmd_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, freq_cmd_args.end, argv[0]);
        return 1;
    }

    if (beepOutput(*freq_cmd_args.freq->ival)) {
        printf("BEEP 输出Freq:%d\n", *freq_cmd_args.freq->ival);
    } else {
        printf("设置失败\n");
    }
    return 0;
}

/**
 * @brief 初始化PWM
 *
 */
void pwmInit(void)
{
    for (uint i = 0; i < sizeof(pwmTimer) / sizeof(ledc_timer_config_t); i++) {
        ledc_timer_config(&pwmTimer[i]);
    }

    for (uint i = 0; i < _TYPE_MAX; i++) {
        ledc_channel_config(&pwm_channel[i]);
        pwmOutput((_PWMTYPE)i, 0);
    }
    ledc_fade_func_install(0);

    // 注册命令
    pwm_cmd_args.type = arg_intn("t", "type", "<n>", 1, 1, "output channel");
    pwm_cmd_args.val = arg_intn("v", "val", "<n>", 1, 1, "output value");
    pwm_cmd_args.end = arg_end(20);
    register_cmd("pwm", "控制PWM输出", NULL, do_pwm_cmd, &pwm_cmd_args);

    freq_cmd_args.freq = arg_intn("f", "freq", "<n>", 1, 1, "output freq");
    freq_cmd_args.end = arg_end(20);
    register_cmd("beep", "控制beep输出", NULL, do_beep_cmd, &freq_cmd_args);
}
