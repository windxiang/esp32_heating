#include "heating.h"
#include <stdio.h>
#include <esp_chip_info.h>
#include <esp_spi_flash.h>
#include "PIDv1.h"

double TipTemperature1 = 0; // PID输入值 (当前温度)
double PID_Output1 = 0; // PID输出值 要输出PWM宽度
double PID_Setpoint1 = 0; // PID目标值 (设定温度值)
double aggKp1 = 0.0, aggKi1 = 0.0, aggKd1 = 0.0;
PID MyPID(&TipTemperature1, &PID_Output1, &PID_Setpoint1, aggKp1, aggKi1, aggKd1, DIRECT);

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

    // 初始化OLED
    // oled_init(OLED_I2C_PIN_SDA, OLED_I2C_PIN_SCL, OLED_FREQ, I2C_MODE_MASTER);

    // 初始化shell
    shell_init();

    // 初始化PWM
    pwmInit();

    // 初始化系统保存参数
    if (ESP_OK == settings_storage_init()) {
        // 读取所有配置
        settings_read_all();
    } else {
        // 初始化错误
        ESP_LOGE(TAG, "初始化NVS失败\r\n");
    }

    // 初始化编码器 编码器工作线程
    // xTaskCreatePinnedToCore(rotary_task, "rotary", 1024 * 5, NULL, 5, NULL, tskNO_AFFINITY);

    // 初始化逻辑控制前期
    // xTaskCreatePinnedToCore(logic_task, "logic", 1024 * 20, NULL, 5, NULL, tskNO_AFFINITY);

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

        // vTaskDelay(100 / portTICK_PERIOD_MS);

        vTaskDelay(24 * 60 * 60 / portTICK_PERIOD_MS);
    }
}

#ifdef __cplusplus
}
#endif // __cplusplus
