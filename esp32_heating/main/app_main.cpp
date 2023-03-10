/**
 * 继续参考的项目:
 * https://oshwhub.com/z_star/pian-xie-mi-ni-jia-re-tai-V2.0
 * https://oshwhub.com/LittleOAndLittleQ/87e303d7f7624710a0cddce77ea6bd42
 * https://oshwhub.com/littleoandlittleq/bian-xie-jia-re-tai
 * 加入调试输出串口
 * WIFI WebServer等功能
 */

#include "heating.h"
#include <stdio.h>
#include <esp_chip_info.h>
#include <esp_spi_flash.h>

static const char* TAG = "app_main";

EventGroupHandle_t pHandleEventGroup = NULL;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void app_main(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "ESP32 chip model %d revision %d (%d CPU cores, WiFi%s%s)\n", chip_info.model, chip_info.revision, chip_info.cores, (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "", (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    ESP_LOGI(TAG, "%d MB %s SPI FLASH\n", spi_flash_get_chip_size() / (1024 * 1024), (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    // 初始化shell
    shell_init();

    // 初始化系统保存参数
    if (ESP_OK == settings_storage_init()) {
        // 读取所有配置
        settings_read_all();
    } else {
        // 初始化错误
        ESP_LOGE(TAG, "初始化NVS失败\r\n");
    }

    // 初始化OLED
    oled_init(OLED_I2C_PIN_SDA, OLED_I2C_PIN_SCL, OLED_FREQ, I2C_MODE_MASTER);

    // 初始化WS2812
    WS2812init();

    // 初始化PWM
    pwmInit();

    // 蜂鸣器
    beepInit();

    // SPI初始化
    max6675Init();

    // T12 初始化
    t12Init();

    // 创建任务
    pHandleEventGroup = xEventGroupCreate();

    // 初始化编码器 编码器工作线程
    rotaryInit();

    // 初始化ADC采集
    adcInit();

    // 初始化逻辑控制前期
    logicInit();

    // uint32_t pos = 0;
    // while (1) {
    // uint32_t _t = GetRotaryPositon();
    // if (pos != _t) {
    //     pos = _t;
    //     ESP_LOGI(TAG, "enco %d\r\n", pos);
    // }

    // ROTARY_BUTTON_TYPE f = getRotaryButton();
    // if (f != BUTTON_NULL) {
    //     ESP_LOGI(TAG, "button %d\r\n", f);
    // }

    // delay(100);

    // delay(24 * 60 * 60);
    // }
}

#ifdef __cplusplus
}
#endif // __cplusplus
