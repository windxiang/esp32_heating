#pragma once

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

// 驱动
#include "rotary.h" // 编码器
#include "oled.h" // OLED

// 渲染
#include "u8g2.h"
#include "render.h"
#include "menuobj.h"

// 工具
#include "mathFun.h"

// 事件
#include "event.h"

// shell
#include "shell.h"

// nvs
#include "nvs.h"

// pwm
#include "pwm.h"

// 逻辑控制
#include "logic.h"

#ifndef constrain
#define constrain(amt, low, high) ((amt) <= (low) ? (low) : ((amt) >= (high) ? (high) : (amt)))
#endif // constrain

#ifndef delay
#define delay(time) vTaskDelay(time / portTICK_PERIOD_MS);
#endif // delay

#ifndef min
#define min(amt, low) ((amt) <= (low) ? (amt) : (low))
#endif

// 一些硬件上的定义
#define PIN_ROTARY_A GPIO_NUM_27 // 编码器
#define PIN_ROTARY_B GPIO_NUM_14 // 编码器
#define PIN_BUTTON GPIO_NUM_33 // 编码器按键

// OLED相关
#define OLED_SCREEN_WIDTH 128 // OLED 宽度
#define OLED_SCREEN_HEIGHT 64 // OLED 高度
#define SCREEN_PAGE_NUM 8
#define SCREEN_FONT_ROW 4
#define CNSize 12

//温度限制
#define HeatMaxTemp 300 // 最大温度值
#define HeatMinTemp 0 // 最小温度值

// PWM 输出GPIO配置
#define PWM_T12 15
#define PWM_FAN 12
#define PWM_HEAT 26
#define PWM_BEEP 25
