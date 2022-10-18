#include <stdio.h>

#include "heating.h"

#include <esp_chip_info.h>
#include <esp_spi_flash.h>

static const char* TAG = "app_main";

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void app_main(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "ESP32 chip model %d revision %d (%d CPU cores, WiFi%s%s)\n", chip_info.model, chip_info.revision, chip_info.cores, (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "", (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    ESP_LOGI(TAG, "%d MB %s SPI FLASH\n", spi_flash_get_chip_size() / (1024 * 1024), (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    // 初始化编码器 编码器工作线程
    xTaskCreatePinnedToCore(rotary_task, "rotary", 1024 * 2, NULL, 5, NULL, tskNO_AFFINITY);

    // 初始化OLED
    oled_init(OLED_I2C_PIN_SDA, OLED_I2C_PIN_SCL, OLED_FREQ, I2C_MODE_MASTER);

    // 初始化Render
    xTaskCreatePinnedToCore(render_task, "render", 1024 * 10, NULL, 5, NULL, tskNO_AFFINITY);

    // uint32_t pos = 0;
    while (1) {
        // uint32_t _t = GetRotaryPositon();
        // if (pos != _t) {
        //     pos = _t;
        //     ESP_LOGI(TAG, "enco %d\r\n", pos);
        // }

        // ROTARY_BUTTON_TYPE f = getRotaryButton();
        // if (f != BUTTON_NULL) {
        //     ESP_LOGI(TAG, "button %d\r\n", f);
        // }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

#ifdef __cplusplus
}
#endif // __cplusplus

void SYSReboot(void)
{
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}