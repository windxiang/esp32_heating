#include "heating.h"
#include "OneButton.h"
#include "driver/gpio.h"

// static const char* TAG = "rotary";

typedef struct {
    float EncoderPosition; // 编码器当前位置 必须在最小最大中间
    float EncoderMinPosition; // 编码器最小位置
    float EncoderMaxPosition; // 编码器最大位置
    float EncoderStepPosition; // 编码器每次步进
    bool EncoderLOCKFlag; // 锁定标志位,设置为Ture进行锁定,锁定后无法修改EncoderPosition值
    int a; // 中断使用
    int b; // 中断使用
    int ab; // 中断使用
    QueueHandle_t buttonQueue; // 按钮队列
    QueueHandle_t encoderQueue; // 编码器队列
} _RotaryData;

static _RotaryData RotaryData = {};

static OneButton RotaryButton(PIN_BUTTON, true); // 编码器按键逻辑处理
static OneButton NormalButton1(PIN_KEY1, true); // 普通按键
static OneButton NormalButton2(PIN_KEY2, true); // 普通按键

#define ROTARY_FREQDIV 2.0f // 编码器分频

////////////////////////////////////////////////////////////////////////////////////////////////
// 发送按下事件
static void sendButtonEvent(_logic_event_type event)
{
    if (NULL != pHandleEventGroup)
        xEventGroupSetBits(pHandleEventGroup, event);
}

////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 编码器中断
 *
 */
static void IRAM_ATTR rotary_handler(void* arg)
{
    // 若编码器被锁定，则不允许数值操作
    if (RotaryData.EncoderLOCKFlag == true)
        return;

    BaseType_t xHigherPriorityTaskWoken = pdTRUE;

    // 得到AB相状态
    int a = gpio_get_level(PIN_ROTARY_A);
    int b = gpio_get_level(PIN_ROTARY_B);

    if (a != RotaryData.a) {
        RotaryData.a = a;

        if (b != RotaryData.b) {
            RotaryData.b = b;

            uint8_t f = a == b;
            xQueueSendFromISR(RotaryData.encoderQueue, &f, &xHigherPriorityTaskWoken);

            if ((a == b) != RotaryData.ab) {
                uint8_t f = a == b;
                xQueueSendFromISR(RotaryData.encoderQueue, &f, &xHigherPriorityTaskWoken);
            }
            RotaryData.ab = (a == b);

            SetSound(BeepSoundRotaryAB, true);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 编码器按键-单击回调函数
 *
 */
static void Callback_ButtonClick(uint32_t parameter)
{
    if (NULL != RotaryData.buttonQueue) {
        ROTARY_BUTTON_TYPE f = (ROTARY_BUTTON_TYPE)parameter;
        xQueueSend(RotaryData.buttonQueue, &f, 0);
        sendButtonEvent(EVENT_LOGIC_KEYUP);
        SetSound(BeepSoundClick, false);
    }
}

/**
 * @brief 编码器按键-长按回调函数
 *
 */
static void Callback_ButtonLongClick(uint32_t parameter)
{
    if (NULL != RotaryData.buttonQueue) {
        ROTARY_BUTTON_TYPE f = (ROTARY_BUTTON_TYPE)parameter;
        xQueueSend(RotaryData.buttonQueue, &f, 0);
        sendButtonEvent(EVENT_LOGIC_KEYUP);
        SetSound(BeepSoundLongClick, false);
    }
}

/**
 * @brief 编码器按键-双击回调函数
 *
 */
static void Callback_ButtonDoubleClick(uint32_t parameter)
{
    if (NULL != RotaryData.buttonQueue) {
        ROTARY_BUTTON_TYPE f = (ROTARY_BUTTON_TYPE)parameter;
        xQueueSend(RotaryData.buttonQueue, &f, 0);
        sendButtonEvent(EVENT_LOGIC_KEYUP);
        SetSound(BeepSoundDoubleClick, false);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 设置计数器参数
 *
 * @param min 最小值
 * @param max 最大值
 * @param step 步进值
 * @param position 当前位置
 */
void RotarySet(float min, float max, float step, float position)
{
    RotaryData.EncoderMinPosition = min * ROTARY_FREQDIV;
    RotaryData.EncoderMaxPosition = max * ROTARY_FREQDIV;
    RotaryData.EncoderStepPosition = step;
    RotaryData.EncoderPosition = constrain(position * ROTARY_FREQDIV, RotaryData.EncoderMinPosition, RotaryData.EncoderMaxPosition);

    // ESP_LOGI(TAG, "设置编码器 -> 最小值=%f 最大值=%f 步进=%f 传入计数=%f 当前计数=%f\r\n", min, max, step, position, RotaryData.EncoderPosition);
}

/**
 * @brief 设置当前编码器位置
 *
 * @param position
 */
void RotarySetPositon(float position)
{
    RotaryData.EncoderPosition = constrain(position * ROTARY_FREQDIV, RotaryData.EncoderMinPosition, RotaryData.EncoderMaxPosition);
    // ESP_LOGI(TAG, "更新编码器 当前计数=%f\r\n", position);
}

/**
 * @brief 编码器数值
 *
 * @return float
 */
float GetRotaryPositon(void)
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

////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 获取编码器按键状态
 *
 * @return ROTARY_BUTTON_TYPE
 */
ROTARY_BUTTON_TYPE getRotaryButton(void)
{
    if (NULL != RotaryData.buttonQueue) {
        ROTARY_BUTTON_TYPE f = BUTTON_NULL;
        xQueueReceive(RotaryData.buttonQueue, &f, 0);
        return f;
    }
    return BUTTON_NULL;
}

/**
 * @brief 清空队列
 *
 */
void rotaryResetQueue(void)
{
    if (NULL != RotaryData.buttonQueue) {
        xQueueReset(RotaryData.buttonQueue);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 编码器线程初始化
 *
 * @param arg
 */
static void rotary_task(void* arg)
{
    // 设置中断函数
    // gpio_isr_handler_add(PIN_ROTARY_A, rotary_handler, NULL);

    uint8_t f;
    while (1) {
        // 编码器旋转
        if (xQueueReceive(RotaryData.encoderQueue, &f, 10 / portTICK_PERIOD_MS) == pdPASS) {
            RotaryData.EncoderPosition = constrain(RotaryData.EncoderPosition + (f ? RotaryData.EncoderStepPosition : -RotaryData.EncoderStepPosition), RotaryData.EncoderMinPosition, RotaryData.EncoderMaxPosition);
            sendButtonEvent(EVENT_LOGIC_KEYUP);
        }

        // 编码器按键
        RotaryButton.tick();

        // 普通按键
        NormalButton1.tick();
        NormalButton2.tick();
    }
}

void rotaryInit(void)
{
    RotaryData.buttonQueue = xQueueCreate(10, sizeof(ROTARY_BUTTON_TYPE));
    RotaryData.encoderQueue = xQueueCreate(10, sizeof(uint8_t));

    // 初始化编码器
    gpio_config_t io_conf = {};
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

    // 初始化编码器按键
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = BIT64(PIN_BUTTON);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    // 初始化普通按键1
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = BIT64(PIN_KEY1);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    // 初始化普通按键2
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = BIT64(PIN_KEY2);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    RotaryButton.attachClick(Callback_ButtonClick, RotaryButton_Click);
    RotaryButton.attachDoubleClick(Callback_ButtonDoubleClick, RotaryButton_DoubleClick);
    RotaryButton.attachLongPressStart(Callback_ButtonLongClick, RotaryButton_LongClick);
    RotaryButton.setDebounceTicks(30 / portTICK_PERIOD_MS);
    RotaryButton.setClickTicks(200 / portTICK_PERIOD_MS);
    RotaryButton.setPressTicks(300 / portTICK_PERIOD_MS);

    NormalButton1.attachClick(Callback_ButtonClick, NormalButton1_Click);
    NormalButton1.attachDoubleClick(Callback_ButtonDoubleClick, NormalButton1_DoubleClick);
    NormalButton1.attachLongPressStart(Callback_ButtonLongClick, NormalButton1_LongClick);
    NormalButton1.setDebounceTicks(30 / portTICK_PERIOD_MS);
    NormalButton1.setClickTicks(200 / portTICK_PERIOD_MS);
    NormalButton1.setPressTicks(300 / portTICK_PERIOD_MS);

    NormalButton2.attachClick(Callback_ButtonClick, NormalButton2_Click);
    NormalButton2.attachDoubleClick(Callback_ButtonDoubleClick, NormalButton2_DoubleClick);
    NormalButton2.attachLongPressStart(Callback_ButtonLongClick, NormalButton2_LongClick);
    NormalButton2.setDebounceTicks(30 / portTICK_PERIOD_MS);
    NormalButton2.setClickTicks(200 / portTICK_PERIOD_MS);
    NormalButton2.setPressTicks(300 / portTICK_PERIOD_MS);

    RotarySet(0.0f, 100.f, 1.f, 1.f);

    // 设置中断函数
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_ROTARY_A, rotary_handler, NULL);

    xTaskCreatePinnedToCore(rotary_task, "rotary", 1024 * 5, NULL, 5, NULL, tskNO_AFFINITY);
}
