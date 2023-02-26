#pragma once

#include <stdbool.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>

#include "esp_log.h"

// 驱动
#include "rotary.h" // 编码器
#include "oled.h" // OLED

// 渲染
#include "u8g2.h"
#include "menuobj.h"
#include "ExternDraw.h"
#include "menuexpand.h"

// 工具
#include "mathFun.h"

// shell
#include "shell.h"

// nvs
#include "nvs.h"

// pwm
#include "pwm.h"

// ws2812 led
#include "ws2812.h"

// 逻辑控制
#include "logic.h"

// max6675
#include "max6675.h"

// ADC
#include "adc.h"

// 蜂鸣器
#include "beep.h"

// T12
#include "t12.h"

#ifndef constrain
#define constrain(amt, low, high) ((amt) <= (low) ? (low) : ((amt) >= (high) ? (high) : (amt)))
#endif // constrain

#ifndef delay
#define delay(time) vTaskDelay(time / portTICK_PERIOD_MS);
#endif // delay

#ifndef min
#define min(amt, low) ((amt) <= (low) ? (amt) : (low))
#endif

#ifndef max
#define max(x, y) ((x) > (y) ? (x) : (y))
#endif

// 按键和编码器
#define PIN_ROTARY_A GPIO_NUM_27 // 编码器
#define PIN_ROTARY_B GPIO_NUM_14 // 编码器
#define PIN_BUTTON GPIO_NUM_33 // 编码器按键
#define PIN_KEY1 GPIO_NUM_4 // 按键1
#define PIN_KEY2 GPIO_NUM_15 // 按键2

// PWM 输出GPIO配置
#define PIN_PWM_T12 GPIO_NUM_2
#define PIN_PWM_FAN 12
#define PIN_PWM_HEAT 26
#define PIN_PWM_BEEP 25

// WS2812 RGB
#define PIN_WS2812RGB GPIO_NUM_32
#define STRIP_LED_NUMBER 1 // WS2812 LED 个数

// MAX6675 SPI
#define PIN_MAX6675_SPI_CS GPIO_NUM_18
#define PIN_MAX6675_SPI_CLK GPIO_NUM_19
#define PIN_MAX6675_SPI_MISO GPIO_NUM_5

// T12
#define PIN_T12_SLEEP GPIO_NUM_13 // GPIO T12休眠检测

// OLED相关
#define OLED_SCREEN_WIDTH 128 // OLED 宽度
#define OLED_SCREEN_HEIGHT 64 // OLED 高度
#define SCREEN_PAGE_NUM 8
#define SCREEN_FONT_ROW 4
#define CNSize 12

#define I2C_NUM I2C_NUM_0 // IIC使用接口
#define OLED_FREQ (1000 * 1000) // IIC频率
#define OLED_I2C_PIN_SCL GPIO_NUM_21 // IIC SCL IO
#define OLED_I2C_PIN_SDA GPIO_NUM_22 // IIC SDA IO
#define I2C_TIMEOUT_MS (100)

// ADC类型定义
enum _enumADCTYPE {
    adc_HeatingTemp = 0, // 加热台温度 SPI读取芯片
    adc_T12Temp, // T12 烙铁头温度 在T12 IO 输出低电平时候采集温度
    adc_T12Cur, // T12 烙铁电流 在T12 IO 输出高电平时候采集电流
    adc_T12PCBNTC, // T12 NTC温度
    adc_SystemVol, // 系统输入电压
    adc_SystemRef, // 系统5V电压
    adc_RoomTemp, // PCB温度
    adc_last_max // 放在末尾
};

// 时间通知
extern EventGroupHandle_t pHandleEventGroup;

// 定义各种事件
enum _logic_event_type {
    EVENT_LOGIC_MENUEXIT = 1 << 0, // 退出菜单
    EVENT_LOGIC_MENUENTER = 1 << 1, // 进入菜单
    EVENT_LOGIC_KEYUP = 1 << 2, // 按键事件
    EVENT_LOGIC_HEATING = 1 << 3, // 事件 开始加热 停止加热
};

// 定义模式
const char heatingModeStr[][32] = {
    { "恒温焊台" },
    { "回流焊" },
    { "T12" },
};
