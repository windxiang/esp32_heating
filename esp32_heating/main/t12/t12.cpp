/*
2. 在使用模拟IO的方式来测试
    2.测试占空比 是否正确
3.T12 插入 拔出的检测
3.停止加热后 要多久时间ADC采集才正确 电烙铁温度
4.开始加热后 要多久时间ADC采集才正确 电流
*/
#include "heating.h"
#include "argtable3/argtable3.h"

#define PERIOD_HZ (100) // 周期时间100ms  10Hz 刚好选择占空比范围为0~100

/**
 * @brief PWM 命令结构体
 *
 */
static struct {
    struct arg_int* val;
    struct arg_int* count;
    struct arg_end* end;
} t12pwm_cmd_args = {};

enum _T12Status {
    _T12_INIT = 0, // T12 初始化阶段
    _T12_START, // T12开始输出
};

struct _strT12Info {
    QueueHandle_t queue;
    _T12Status status;
    TickType_t tickStart; // 开始输出时间
    uint8_t CurDutyCycle; // 本次输出占空比
    uint8_t nextDutyCycle; // 下一个占空比
};

static _strT12Info t12Info = {};

/**
 * @brief T12手柄是否插入 TODO
 *
 * @return true 插入
 * @return false 未插入
 */
bool t12_isInsert(void)
{
    return true;
}

/**
 * @brief T12手柄 是否放下
 *
 * @return true 放下
 * @return false 拿在手上
 */
bool t12_isPutDown(void)
{
    return gpio_get_level(PIN_T12_SLEEP) == false;
}

/**
 * @brief T12 加热输出控制
 *
 * @param level
 */
void setT12IOLevel(uint32_t level)
{
    gpio_set_level(PIN_PWM_T12, level);
}

/**
 * @brief 获取T12 加热输出IO状态
 *
 * @return true 处于高电平 加热中
 * @return false 处于低电平 不在加热状态中
 */
static bool getT12PWMLevel(void)
{
    return gpio_get_level(PIN_PWM_T12) == true;
}

/**
 * @brief 设置 T12 PWM 输出
 *
 * @param dutyCycle 输出周期 百分比 （0~100）
 * @return true
 * @return false
 */
void t12PWMOutput(uint8_t dutyCycle)
{
    uint8_t q = dutyCycle;
    if (t12Info.queue) {
        xQueueSend(t12Info.queue, &q, 0);
    }
}

/**
 * @brief T12 状态机处理
 * 这里频率是10HZ 100ms一个周期 方便计算 也不能使用太高频率 这个频率刚刚好
 * 要一个频率输出完整后 才可以切换到最后一个设定的占空比上
 *
 * @param dutyCycle 占空比 范围 0~100 如果不需要输出了则要发送一个0进来
 */
static bool T12OutputStateMachine(uint8_t dutyCycle)
{
    // 判断范围在100以内
    if (100 < dutyCycle) {
        dutyCycle = 100;
    }

    // 状态机
    switch (t12Info.status) {
    case _T12_INIT: {
        if (0 < dutyCycle) {
            // 开始输出
            t12Info.tickStart = xTaskGetTickCount();
            t12Info.CurDutyCycle = dutyCycle;
            setT12IOLevel(1);
            t12Info.status = _T12_START;

        } else {
            // 等待开始
            t12Info.CurDutyCycle = 0;
            t12Info.status = _T12_INIT;
            setT12IOLevel(0);
        }
    } break;

    case _T12_START: {
        // 已经开始输出状态下
        TickType_t targetTick = t12Info.tickStart + t12Info.CurDutyCycle; // 高电平时间
        TickType_t now = xTaskGetTickCount();

        if (targetTick > now) {
            setT12IOLevel(1);

        } else {
            setT12IOLevel(0);
            delay(1);
            adcProcess(adc_T12Temp, now);
        }

        if ((PERIOD_HZ + t12Info.tickStart) <= now) {
            // 输出完一个周期 切换到下一个周期进行输出
            t12Info.status = _T12_INIT;
            t12Info.CurDutyCycle = t12Info.nextDutyCycle;
            // delay(1);
            // adcProcess(adc_T12Temp, now);
        }
    } break;

    default:
        break;
    }

    return true;
}

/**
 * @brief T12PWM 测试命令
 *
 * @param argc
 * @param argv
 * @return int
 */
static int do_t12pwm_cmd(int argc, char** argv)
{
    int nerrors = arg_parse(argc, argv, (void**)&t12pwm_cmd_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, t12pwm_cmd_args.end, argv[0]);
        return 1;
    }

    printf("\r\n\r\n");
    for (int i = 0; i < *t12pwm_cmd_args.count->ival; i++) {
        t12PWMOutput(*t12pwm_cmd_args.val->ival);
        delay(5);
    }
    return 0;
}

/**
 * @brief T12 PWM 任务
 *
 * @param arg
 */
static void t12pwm_task(void* arg)
{
    // 停止输出
    T12OutputStateMachine(0);

    uint8_t q;
    while (1) {
        if (xQueueReceive(t12Info.queue, &q, 1 / portTICK_PERIOD_MS) == pdPASS) {
            // 得到新的占空比
            t12Info.nextDutyCycle = q;
            if (t12Info.status == _T12_INIT) {
                t12Info.CurDutyCycle = t12Info.nextDutyCycle;
            }
        }

        T12OutputStateMachine(t12Info.CurDutyCycle);
    }
}

/**
 * @brief 初始化T12
 *
 */
void t12Init(void)
{
    // 创建队列
    t12Info.queue = xQueueCreate(10, sizeof(uint8_t));

    // Sleep IO
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = BIT64(PIN_T12_SLEEP);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE; // GPIO_INTR_ANYEDGE;
    gpio_config(&io_conf);

    // PWM IO
    // io_conf.mode = GPIO_MODE_OUTPUT;
    // io_conf.pin_bit_mask = BIT64(PIN_PWM_T12);
    // io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    // io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    // io_conf.intr_type = GPIO_INTR_DISABLE;
    // gpio_config(&io_conf);

    // setT12IOLevel(0);

    // xTaskCreatePinnedToCore(t12pwm_task, "t12pwm", 1024 * 5, NULL, 5, NULL, tskNO_AFFINITY);

    // 测试命令
    t12pwm_cmd_args.val = arg_intn("v", "val", "<n>", 1, 1, "t12 output value");
    t12pwm_cmd_args.count = arg_intn("c", "count", "<n>", 1, 1, "t12 test count");
    t12pwm_cmd_args.end = arg_end(20);
    register_cmd("t12pwm", "测试T12PWM", NULL, do_t12pwm_cmd, &t12pwm_cmd_args);
}
