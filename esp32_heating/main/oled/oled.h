#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "u8g2.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

#define I2C_TX_BUF_DISABLE 0 /*!< I2C master do not need buffer */
#define I2C_RX_BUF_DISABLE 0 /*!< I2C master do not need buffer */

#define WRITE_BIT I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ /*!< I2C master read */

#define ACK_CHECK_EN 1 /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0 /*!< I2C master will not check ack from slave */

#define ACK_VAL (i2c_ack_type_t)0 /*!< I2C ack value */
#define NACK_VAL (i2c_ack_type_t)1 /*!< I2C nack value */

extern u8g2_t u8g2;

void oled_init(int sda_io_num, int scl_io_num, uint32_t freqHZ, i2c_mode_t mode);
void Update_OLED_Flip(uint8_t isFlip);
void Update_OLED_Light_Level(uint8_t light);

uint8_t u8g2_esp32_i2c_byte_cb(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);
uint8_t u8g2_esp32_gpio_and_delay_cb(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);

#ifdef __cplusplus
}
#endif // __cplusplus