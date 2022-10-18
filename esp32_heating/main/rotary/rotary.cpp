#include "heating.h"
#include "OneButton.h"

#include "driver/gpio.h"

static const char* TAG = "rotary";

typedef struct {
    int32_t EncoderPosition; // 编码器当前位置 必须在最小最大中间
    int32_t EncoderMinPosition; // 编码器最小位置
    int32_t EncoderMaxPosition; // 编码器最大位置
    int32_t EncoderStepPosition; // 编码器每次步进
    bool EncoderLOCKFlag; // 锁定标志位,设置为Ture进行锁定,锁定后无法修改EncoderPosition值
    int a; // 中断使用
    int b; // 中断使用
    int ab; // 中断使用
    QueueHandle_t queue; // 队列
} _RotaryData;

static _RotaryData RotaryData = {};

static OneButton RotaryButton(PIN_BUTTON, true); // 编码器按键逻辑处理

#define ROTARY_FREQDIV 2 // 编码器分频

/**
 * @brief 编码器中断
 *
 */
static void IRAM_ATTR rotary_handler(void* arg)
{
    TimerUpdateEvent();

    // 若编码器被锁定，则不允许数值操作
    if (RotaryData.EncoderLOCKFlag == true)
        return;

    // 得到AB相状态
    int a = gpio_get_level(PIN_ROTARY_A);
    int b = gpio_get_level(PIN_ROTARY_B);

    if (a != RotaryData.a) {
        RotaryData.a = a;

        if (b != RotaryData.b) {
            RotaryData.b = b;

            RotaryData.EncoderPosition = constrain(RotaryData.EncoderPosition + ((a == b) ? RotaryData.EncoderStepPosition : -RotaryData.EncoderStepPosition), RotaryData.EncoderMinPosition, RotaryData.EncoderMaxPosition);

            if ((a == b) != RotaryData.ab) {
                RotaryData.EncoderPosition = constrain(RotaryData.EncoderPosition + ((a == b) ? RotaryData.EncoderStepPosition : -RotaryData.EncoderStepPosition), RotaryData.EncoderMinPosition, RotaryData.EncoderMaxPosition);
            }
            RotaryData.ab = (a == b);
        }
    }
}

/***
 * @description: 按键单击回调函数
 * @param {*}
 * @return {*}
 */
static void RotaryButtonClick(void)
{
    if (NULL != RotaryData.queue) {
        // SetSound(Beep1);
        TimerUpdateEvent();
        ROTARY_BUTTON_TYPE f = BUTTON_CLICK;
        xQueueSend(RotaryData.queue, &f, 0);
    }
}

/***
 * @description: 按键长按回调函数
 * @param {*}
 * @return {*}
 */
static void RotaryButtonLongClick(void)
{
    if (NULL != RotaryData.queue) {
        // SetSound(Beep1);
        TimerUpdateEvent();
        ROTARY_BUTTON_TYPE f = BUTTON_LONGCLICK;
        xQueueSend(RotaryData.queue, &f, 0);
    }
}

/***
 * @description: 按键双击回调函数
 * @param {*}
 * @return {*}
 */
static void RotaryButtonDoubleClick(void)
{
    if (NULL != RotaryData.queue) {
        // SetSound(Beep1);
        TimerUpdateEvent();
        ROTARY_BUTTON_TYPE f = BUTTON_DOUBLECLICK;
        xQueueSend(RotaryData.queue, &f, 0);
    }
}

/**
 * @brief 设置计数器参数
 * @param {double} c       计数器初始值
 * @param {double} min     计数器最小值
 * @param {double} max     计数器最大值
 * @param {double} step    计数器增量步进
 * @return {*}
 */
void RotarySet(int32_t min, int32_t max, int32_t step, int32_t position)
{
    RotaryData.EncoderMinPosition = min * ROTARY_FREQDIV;
    RotaryData.EncoderMaxPosition = max * ROTARY_FREQDIV;
    RotaryData.EncoderStepPosition = step;
    RotaryData.EncoderPosition = constrain(position * ROTARY_FREQDIV, RotaryData.EncoderMinPosition, RotaryData.EncoderMaxPosition);

    ESP_LOGI(TAG, "设置编码器 -> 最小值=%d 最大值=%d 步进=%d 传入计数=%d 当前计数=%d\r\n", min, max, step, position, RotaryData.EncoderPosition);
}

/**
 * @brief 设置当前编码器位置
 *
 * @param position
 */
void RotarySetPositon(int32_t position)
{
    RotaryData.EncoderPosition = constrain(position * ROTARY_FREQDIV, RotaryData.EncoderMinPosition, RotaryData.EncoderMaxPosition);
    ESP_LOGI(TAG, "更新编码器 当前计数=%d\r\n", position);
}

/**
 * @brief 编码器数值
 *
 * @return int32_t
 */
int32_t GetRotaryPositon(void)
{
    return RotaryData.EncoderPosition / ROTARY_FREQDIV;
}

/**
 * @brief 编码器是否要锁定 锁定后无法进行旋转操作
 *
 * @param isLock
 */
void setRotaryLock(bool isLock)
{
    RotaryData.EncoderLOCKFlag = isLock;
}

/**
 * @brief 获取编码器按键状态
 *
 * @return ROTARY_BUTTON_TYPE
 */
ROTARY_BUTTON_TYPE getRotaryButton(void)
{
    if (NULL != RotaryData.queue) {
        ROTARY_BUTTON_TYPE f = BUTTON_NULL;
        xQueueReceive(RotaryData.queue, &f, 0);
        return f;
    }
    return BUTTON_NULL;
}

/**
 * @brief 编码器线程初始化
 *
 * @param arg
 */
void rotary_task(void* arg)
{
    RotaryData.queue = xQueueCreate(10, sizeof(ROTARY_BUTTON_TYPE));

    gpio_install_isr_service(0);

    // 初始化编码器
    gpio_config_t io_conf;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = BIT64(PIN_ROTARY_A);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    gpio_config(&io_conf);

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = BIT64(PIN_ROTARY_B);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    // 初始化按键
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = BIT64(PIN_BUTTON);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    RotaryButton.attachClick(RotaryButtonClick);
    RotaryButton.attachDoubleClick(RotaryButtonDoubleClick);
    RotaryButton.attachLongPressStart(RotaryButtonLongClick);
    RotaryButton.setDebounceTicks(30 / portTICK_PERIOD_MS);
    RotaryButton.setClickTicks(200 / portTICK_PERIOD_MS);
    RotaryButton.setPressTicks(300 / portTICK_PERIOD_MS);

    RotarySet(0, 100, 1, 1);

    // 设置中断函数
    gpio_isr_handler_add(PIN_ROTARY_A, rotary_handler, NULL);

    while (1) {
        RotaryButton.tick();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}