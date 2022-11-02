#include "heating.h"
#include <driver/ledc.h>
#include <math.h>
#include "argtable3/argtable3.h"

#define DUTYRESOLUTION LEDC_TIMER_12_BIT // 分辨率 最大12位

static const ledc_timer_config_t pwm_timer = {
    .speed_mode = LEDC_HIGH_SPEED_MODE, // timer mode
    .duty_resolution = DUTYRESOLUTION, // resolution of PWM duty
    .timer_num = LEDC_TIMER_0, // timer index
    .freq_hz = 10 * 1000, // frequency of PWM signal
    .clk_cfg = LEDC_AUTO_CLK, // Auto select the source clock
};

static const ledc_channel_config_t pwm_channel[_TYPE_MAX] = {
    // PWM T12
    [_TYPE_T12] = {
        .gpio_num = PWM_T12,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .flags = {
            .output_invert = 0,
        },
    },
    // PWM 风扇
    [_TYPE_FAN] = {
        .gpio_num = PWM_FAN,
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
        .gpio_num = PWM_HEAT,
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
        .gpio_num = PWM_BEEP,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_3,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
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
 * @brief 设置PWM输出百分比
 *
 * @param type 输出通道
 * @param value 百分比
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

    uint32_t duty = (uint32_t)((float)value / 100.0f * (float)(pow(2, (int)DUTYRESOLUTION) - 1));

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
 * @brief 初始化PWM
 *
 */
void pwmInit(void)
{
    ledc_timer_config(&pwm_timer);
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
}
