#include "heating.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

static const char* TAG = "nvs";
static nvs_handle SettingsHandle = 0;

/**
 * @brief 保存
 *
 * @return int32_t
 */
static int32_t settings_commit(void)
{
    if (0 != SettingsHandle) {
        ESP_LOGI(TAG, "保存配置成功\r\n");
        return nvs_commit(SettingsHandle);
    }

    ESP_LOGE(TAG, "保存配置失败\r\n");
    return -1;
}

static int do_readnvs_cmd(int argc, char** argv)
{
    settings_read_all();
    return 0;
}

static int do_writenvs_cmd(int argc, char** argv)
{
    settings_write_all();
    return 0;
}

// 设置存储 初始化
esp_err_t settings_storage_init(void)
{
    esp_err_t err;

    // 初始化 NVS
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS分区被截断，需要擦除
        err = nvs_flash_erase();
        if (err != ESP_OK)
            return err;

        // 重试 nvs_flash_init
        err = nvs_flash_init();
        if (err != ESP_OK)
            return err;

        settings_write_all();
    }

    register_cmd("readnvs", "读取nvs", NULL, do_readnvs_cmd, NULL);
    register_cmd("writenvs", "写入nvs", NULL, do_writenvs_cmd, NULL);
    return err;
}

/**
 * @brief 读取所有配置
 *
 * @return int
 */
int settings_read_all(void)
{
    if (0 == SettingsHandle) {
        if (nvs_open("settings", NVS_READWRITE, &SettingsHandle) != ESP_OK)
            return -1;
    }

    size_t size;

    // 读取菜单配置
    size = sizeof(SystemMenuSaveData);
    nvs_get_blob(SettingsHandle, "m_sys", &SystemMenuSaveData, &size);

    // 读取加热台T12配置参数
    int8_t count = 0;
    nvs_get_i8(SettingsHandle, "c_curMax", &count); // 全部配置个数
    nvs_get_i8(SettingsHandle, "c_curIndex", &HeatingConfig.curConfigIndex); // 当前配置

    HeatingConfig.heatingConfig.clear();

    for (int i = 0; i < count; i++) {
        char key[32] = { 0 };
        // char name[20] = { 0 };

        _HeatingConfig config = {};

        sprintf(key, "c_con-%d", i);
        size = sizeof(config);
        nvs_get_blob(SettingsHandle, key, &config, &size);

        // 添加
        HeatingConfig.heatingConfig.push_back(config);
    }

    // 读取卡尔曼滤波参数
    for (int i = 0; i < adc_last_max; i++) {
        char key[32] = { 0 };
        sprintf(key, "c_kalman-%d", i);
        size = sizeof(KalmanInfo[i].parm);
        nvs_get_blob(SettingsHandle, key, &KalmanInfo[i].parm, &size);
    }

    return 0;
}

// 写入所有配置
int settings_write_all(void)
{
    if (0 == SettingsHandle) {
        if (nvs_open("settings", NVS_READWRITE, &SettingsHandle) != ESP_OK)
            return -1;
    }

    nvs_set_blob(SettingsHandle, "m_sys", &SystemMenuSaveData, sizeof(SystemMenuSaveData));

    // 写入加热台T12配置参数
    nvs_set_i8(SettingsHandle, "c_curMax", HeatingConfig.heatingConfig.size());
    nvs_set_i8(SettingsHandle, "c_curIndex", HeatingConfig.curConfigIndex);
    for (int i = 0; i < HeatingConfig.heatingConfig.size(); i++) {
        char key[32] = { 0 };
        sprintf(key, "c_con-%d", i);
        nvs_set_blob(SettingsHandle, key, &HeatingConfig.heatingConfig[i], sizeof(HeatingConfig.heatingConfig[i]));
    }

    // 写入卡尔曼滤波参数
    for (int i = 0; i < adc_last_max; i++) {
        char key[32] = { 0 };
        sprintf(key, "c_kalman-%d", i);
        nvs_set_blob(SettingsHandle, key, &KalmanInfo[i].parm, sizeof(KalmanInfo[i].parm));
    }

    return settings_commit();
}
