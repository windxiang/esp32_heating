#include "heating.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

static const char* TAG = "nvs";
static nvs_handle SettingsHandle = 0;

typedef enum {
    _int8,
    _uint8,
    _int16,
    _uint16,
    _int32,
    _uint32,
    _int64,
    _uint64,
    _str,
    _float32
} eType;

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
    nvs_get_u8(SettingsHandle, "m_Language", &SystemMenuSaveData.Language);
    nvs_get_u8(SettingsHandle, "m_BLEState", &SystemMenuSaveData.BLEState);
    nvs_get_u8(SettingsHandle, "m_ScreenFlip", &SystemMenuSaveData.ScreenFlip);
    nvs_get_u8(SettingsHandle, "m_MenuListMode", &SystemMenuSaveData.MenuListMode);
    nvs_get_u8(SettingsHandle, "m_SmoothAni", &SystemMenuSaveData.SmoothAnimationFlag);
    nvs_get_u8(SettingsHandle, "m_Volume", &SystemMenuSaveData.Volume);
    nvs_get_u8(SettingsHandle, "m_PanelSettings", &SystemMenuSaveData.PanelSettings);
    nvs_get_u8(SettingsHandle, "m_OptionWidth", &SystemMenuSaveData.OptionStripFixedLength_Flag);
    size = sizeof(SystemMenuSaveData.ScreenBrightness);
    nvs_get_blob(SettingsHandle, "m_OLEDLight", &SystemMenuSaveData.ScreenBrightness, &size);
    size = sizeof(SystemMenuSaveData.UndervoltageAlert);
    nvs_get_blob(SettingsHandle, "m_UnderVoltage", &SystemMenuSaveData.UndervoltageAlert, &size);
    size = sizeof(SystemMenuSaveData.BLEName);
    nvs_get_str(SettingsHandle, "m_BLEName", SystemMenuSaveData.BLEName, &size);
    size = sizeof(SystemMenuSaveData.BootPasswd);
    nvs_get_str(SettingsHandle, "m_BootPasswd", SystemMenuSaveData.BootPasswd, &size);

    // 读取加热台T12配置参数
    int8_t count = 0;
    nvs_get_i8(SettingsHandle, "c_curMax", &count);
    nvs_get_i8(SettingsHandle, "c_curIndex", &HeatingConfig.curConfigIndex);
    HeatingConfig.heatingConfig.clear();
    for (int i = 0; i < count; i++) {
        char key[32] = { 0 };
        char name[20] = { 0 };

        _HeatingConfig heatingConfig = {};

        sprintf(key, "c_name-%d", i);
        size = sizeof(name);
        nvs_get_str(SettingsHandle, key, name, &size);
        heatingConfig.name = name;

        sprintf(key, "c_pid-%d", i);
        size = sizeof(heatingConfig.PID);
        nvs_get_blob(SettingsHandle, key, &heatingConfig.PID, &size);

        sprintf(key, "c_temp-%d", i);
        size = sizeof(heatingConfig.PTemp);
        nvs_get_blob(SettingsHandle, key, &heatingConfig.PTemp, &size);

        HeatingConfig.heatingConfig.push_back(heatingConfig);
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

    nvs_set_u8(SettingsHandle, "m_Language", SystemMenuSaveData.Language);
    nvs_set_u8(SettingsHandle, "m_BLEState", SystemMenuSaveData.BLEState);
    nvs_set_u8(SettingsHandle, "m_ScreenFlip", SystemMenuSaveData.ScreenFlip);
    nvs_set_u8(SettingsHandle, "m_MenuListMode", SystemMenuSaveData.MenuListMode);
    nvs_set_u8(SettingsHandle, "m_SmoothAni", SystemMenuSaveData.SmoothAnimationFlag);
    nvs_set_u8(SettingsHandle, "m_Volume", SystemMenuSaveData.Volume);
    nvs_set_u8(SettingsHandle, "m_PanelSettings", SystemMenuSaveData.PanelSettings);
    nvs_set_u8(SettingsHandle, "m_OptionWidth", SystemMenuSaveData.OptionStripFixedLength_Flag);
    nvs_set_blob(SettingsHandle, "m_OLEDLight", (void*)&SystemMenuSaveData.ScreenBrightness, sizeof(SystemMenuSaveData.ScreenBrightness));
    nvs_set_blob(SettingsHandle, "m_UnderVoltage", (void*)&SystemMenuSaveData.UndervoltageAlert, sizeof(SystemMenuSaveData.UndervoltageAlert));
    nvs_set_str(SettingsHandle, "m_BLEName", SystemMenuSaveData.BLEName);
    nvs_set_str(SettingsHandle, "m_BootPasswd", SystemMenuSaveData.BootPasswd);

    // 写入加热台T12配置参数
    nvs_set_i8(SettingsHandle, "c_curMax", HeatingConfig.heatingConfig.size());
    nvs_set_i8(SettingsHandle, "c_curIndex", HeatingConfig.curConfigIndex);
    for (int i = 0; i < HeatingConfig.heatingConfig.size(); i++) {
        char key[32] = { 0 };

        sprintf(key, "c_name-%d", i);
        nvs_set_str(SettingsHandle, key, HeatingConfig.heatingConfig[i].name.c_str());

        sprintf(key, "c_pid-%d", i);
        nvs_set_blob(SettingsHandle, key, &HeatingConfig.heatingConfig[i].PID, sizeof(HeatingConfig.heatingConfig[i].PID));

        sprintf(key, "c_temp-%d", i);
        nvs_set_blob(SettingsHandle, key, &HeatingConfig.heatingConfig[i].PTemp, sizeof(HeatingConfig.heatingConfig[i].PTemp));
    }

    return settings_commit();
}
