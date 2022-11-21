#include "heating.h"
#include "driver/gpio.h"

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

u8g2_t u8g2 = {};

static const char* TAG = "u8g2_hal";
static i2c_cmd_handle_t handle_i2c = NULL; // I2C handle.

uint8_t u8g2_esp32_i2c_byte_cb(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr)
{
    switch (msg) {
    case U8X8_MSG_BYTE_SET_DC: {
        break;
    }

    case U8X8_MSG_BYTE_INIT: {
        // 初始化
        break;
    }

    case U8X8_MSG_BYTE_SEND: {
        uint8_t* data_ptr = (uint8_t*)arg_ptr;
        ESP_LOG_BUFFER_HEXDUMP(TAG, data_ptr, arg_int, ESP_LOG_VERBOSE);

        while (arg_int > 0) {
            i2c_master_write_byte(handle_i2c, *data_ptr, ACK_CHECK_EN);
            data_ptr++;
            arg_int--;
        }
        break;
    }

    case U8X8_MSG_BYTE_START_TRANSFER: {
        uint8_t i2c_address = u8x8_GetI2CAddress(u8x8);
        handle_i2c = i2c_cmd_link_create();
        i2c_master_start(handle_i2c);
        i2c_master_write_byte(handle_i2c, i2c_address | I2C_MASTER_WRITE, ACK_CHECK_EN);
        break;
    }

    case U8X8_MSG_BYTE_END_TRANSFER: {
        i2c_master_stop(handle_i2c);
        i2c_master_cmd_begin(I2C_NUM, handle_i2c, I2C_TIMEOUT_MS / portTICK_RATE_MS);
        i2c_cmd_link_delete(handle_i2c);
        break;
    }
    }
    return 0;
}

uint8_t u8g2_esp32_gpio_and_delay_cb(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr)
{
    switch (msg) {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
        // Initialize the GPIO and DELAY HAL functions.  If the pins for DC and
        // RESET have been specified then we define those pins as GPIO outputs.
        break;

    case U8X8_MSG_GPIO_RESET:
        // Set the GPIO reset pin to the value passed in through arg_int.
        break;

    case U8X8_MSG_GPIO_CS:
        // Set the GPIO client select pin to the value passed in through arg_int.
        break;

    case U8X8_MSG_GPIO_I2C_CLOCK:
        // Set the Software I²C pin to the value passed in through arg_int.
        break;

    case U8X8_MSG_GPIO_I2C_DATA:
        // Set the Software I²C pin to the value passed in through arg_int.
        break;

    case U8X8_MSG_DELAY_MILLI:
        // Delay for the number of milliseconds passed in through arg_int.
        delay(arg_int);
        break;
    }
    return 0;
}

void oled_init(int sda_io_num, int scl_io_num, uint32_t freqHZ, i2c_mode_t mode)
{
    // 初始化IIC
    i2c_config_t conf = {};
    conf.mode = mode;
    conf.sda_io_num = sda_io_num,
    conf.scl_io_num = scl_io_num,
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE,
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE,
    conf.master.clk_speed = freqHZ;
    i2c_param_config(I2C_NUM, &conf);
    i2c_driver_install(I2C_NUM, conf.mode, I2C_RX_BUF_DISABLE, I2C_TX_BUF_DISABLE, 0);

    // 初始化u8g2 oled libaray
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8g2_esp32_i2c_byte_cb, u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
    u8x8_SetI2CAddress(&u8g2.u8x8, 0x78);

    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearBuffer(&u8g2);

    u8g2_SetFontPosTop(&u8g2); // 设置字体基线为Top

    u8g2_SetFont(&u8g2, u8g2_font_sticker_mel_tr);
    u8g2_DrawStr(&u8g2, 20, 25, "HelloWorld");

    u8g2_SetFont(&u8g2, u8g2_font_wqy12_t_gb2312); // 设置一个默认的字体
    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_SetFontMode(&u8g2, 1);

    // u8g2_SetFontDirection(&u8g2, 1);

    // u8g2_DrawBox(&u8g2, 0, 0, 10, 15);
    // u8g2_DrawFrame(&u8g2, 0, 26, 100, 6);
    // u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
    // u8g2_DrawStr(&u8g2, 0, 0, "Hi World!");
    // u8g2_DrawUTF8(&u8g2, 0, 1, "abc");
    u8g2_SendBuffer(&u8g2);

    delay(1000);
}

/**
 * @brief 反转屏幕
 *
 */
void Update_OLED_Flip(uint8_t isFlip)
{
    u8g2_SetFlipMode(&u8g2, isFlip);
}

/**
 * @brief 更新屏幕亮度
 *
 */
void Update_OLED_Light_Level(uint8_t light)
{
    u8g2_SendF(&u8g2, "c", 0x81); //向SSD1306发送指令：设置内部电阻微调
    u8g2_SendF(&u8g2, "c", light); //微调范围（0-255）
}