#include "heating.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"
#include "stdlib.h"
#include <math.h>
#include "ntc.h"

static const char* TAG = "ADC";

/*  ESP32 ADC1 channel 与 Pin 对照表
    +----------+------------+-------+-----------------+--------------+
    |          |   channel  |  Pin  |      Name       |   Function   |
    +==========+============+=======+=================+==============+
    |          |  ADC1_CH0  |   5   |    SENSOR_VP    |    GPIO36    |
    |          +------------+-------+-----------------+--------------+
    |          |  ADC1_CH1  |   6   |    SENSOR_CAPP  |    GPIO37    |
    |          +------------+-------+-----------------+--------------+
    |          |  ADC1_CH2  |   7   |    SENSOR_CAPN  |    GPIO38    |
    |          +------------+-------+-----------------+--------------+
    |   ESP32  |  ADC1_CH3  |   8   |    SENSOR_VN    |    GPIO39    |
    |   SAR    +------------+-------+-----------------+--------------+
    |   ADC1   |  ADC1_CH4  |   12  |    32K_XP       |    GPIO32    |
    |          +------------+-------+-----------------+--------------+
    |          |  ADC1_CH5  |   13  |    32K_XN       |    GPIO33    |
    |          +------------+-------+-----------------+--------------+
    |          |  ADC1_CH6  |   10  |    VDET_1       |    GPIO34    |
    |          +------------+-------+-----------------+--------------+
    |          |  ADC1_CH7  |   11  |    VDET_2       |    GPIO35    |
    +----------+------------+-------+-----------------+--------------+
*/

struct _strADCInfo {
    float originalRam; // ADC 原始数据
    float ram; // adc 滤波后的数据
    adc1_channel_t channel; // adc 采集通道
    TickType_t lastSampleTime; // 最后一次采样时间
};

static _strADCInfo strADCInfo[adc_last_max] = {
    [adc_HeatingTemp] = { 0, 0, ADC1_CHANNEL_MAX, 0 },
    [adc_T12Temp] = { 0, 0, ADC1_CHANNEL_0, 0 },
    [adc_T12Cur] = { 0, 0, ADC1_CHANNEL_3, 0 },
    [adc_T12NTC] = { 0, 0, ADC1_CHANNEL_6, 0 },
    [adc_SystemVol] = { 0, 0, ADC1_CHANNEL_1, 0 },
    [adc_RoomTemp] = { 0, 0, ADC1_CHANNEL_7, 0 },
};

#define DEFAULT_VREF 1100

/**
 * @brief 获取 加热台温度
 *
 * @return float
 */
float adcGetHeatingTemp(void)
{
    const float calibrationVal = KalmanInfo[adc_HeatingTemp].parm.calibrationVal;
    return (float)strADCInfo[adc_HeatingTemp].ram + calibrationVal;
}

/**
 * @brief 获取T12温度
 *
 * @return float
 */
float adcGetT12Temp(void)
{
    const float calibrationVal = KalmanInfo[adc_T12Temp].parm.calibrationVal;
    return (float)strADCInfo[adc_T12Temp].ram + calibrationVal;
}

/**
 * @brief 获取T12电流
 *
 * @return float
 */
float adcGetT12Cur(void)
{
    const float calibrationVal = KalmanInfo[adc_T12Cur].parm.calibrationVal;
    return (float)strADCInfo[adc_T12Cur].ram + calibrationVal;
}

/**
 * @brief 获取T12 NTC温度
 *
 * @return float
 */
float adcGetT12NTC(void)
{
    const float calibrationVal = KalmanInfo[adc_T12NTC].parm.calibrationVal;
    return (float)strADCInfo[adc_T12NTC].ram + calibrationVal;
}

/**
 * @brief 获取系统输入电压(mV)
 *
 * @return float
 */
float adcGetSystemVol(void)
{
    const float up = 10.0f * 1000.0f;
    const float down = 1.0f * 1000.0f;
    const float calibrationVal = KalmanInfo[adc_SystemVol].parm.calibrationVal;

    uint32_t voltage = strADCInfo[adc_SystemVol].ram * (down + up) / down;
    return voltage + calibrationVal;
}

/**
 * @brief 获取NTC室温
 *
 * @return float
 */
float adcGetRoomNTCTemp(void)
{
    const float calibrationVal = KalmanInfo[adc_RoomTemp].parm.calibrationVal;
    const float R0 = 10 * 1000; // R0电阻阻值
    const float Rn = 10 * 1000; // Rntc电阻阻值
    const float Tn = 25.0f; // Tn - nominal temperature in Celsius.
    const float B = 3950.0f; // B - b-value of a thermistor.

    const float temp = strADCInfo[adc_RoomTemp].ram; // ADC 采集的电压值mV

    // printf("采集 %f %f \n", temp, calibrationVal);

    NTC_Thermistor thermistor(R0, Rn, Tn, B, 4.98f * 1000.0f);
    const double celsius = thermistor.readCelsius(temp);
    return celsius + calibrationVal;
}

//检查 TP两点校准值、Vref参考电压值 是否被刻录到eFuse中
static void check_efuse(void)
{
    // 检查 TP两点校准值 是否被刻录到eFuse中（TP两点校准值是用户自己测量，并刻录到eFuse中）
    esp_err_t eFuse_TP = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP);

    // 检查 Vref参考电压值 是否被刻录到eFuse中（eFuse Vref由工厂生产时刻录，一般默认用这种方式校准）
    esp_err_t eFuse_Vref = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF);

    if (eFuse_TP == ESP_OK) {
        ESP_LOGI(TAG, "eFuse Two Point: Supported\n");
    } else {
        ESP_LOGI(TAG, "eFuse Two Point: NOT supported\n");
    }

    if (eFuse_Vref == ESP_OK) {
        ESP_LOGI(TAG, "eFuse Vref: Supported\n");
    } else {
        ESP_LOGI(TAG, "eFuse Vref: NOT supported\n");
    }
}

static esp_adc_cal_characteristics_t adc1_chars;
// static esp_adc_cal_characteristics_t adc2_chars; 此通道目前没用
static bool cali_enable = false;

/**
 * @brief 得到ADC的校准值
 *
 * @param width
 * @return true
 * @return false
 */
static void adc_calibration_init(adc_atten_t atten, adc_bits_width_t width)
{
    cali_enable = false;

    esp_err_t ret = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF);
    if (ret == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGI(TAG, "Calibration scheme not supported, skip software calibration");

    } else if (ret == ESP_ERR_INVALID_VERSION) {
        ESP_LOGI(TAG, "eFuse not burnt, skip software calibration");

    } else if (ret == ESP_OK) {
        ESP_LOGI(TAG, "eFuse Vref: Supported\n");

        cali_enable = true;
        esp_adc_cal_characterize(ADC_UNIT_1, atten, width, 0, &adc1_chars);
        // esp_adc_cal_characterize(ADC_UNIT_2, atten, width, 0, &adc2_chars);
    } else {
        ESP_LOGE(TAG, "Invalid arg");
    }
}

/*
    ESP32 ADC 衰减系数 与 量程 对照表
    +----------+------------+--------------------------+------------------+
    |   SoC    | attenuation|   suggested range (mV)   |  full range (V)  |
    +==========+============+==========================+==================+
    |          |  0         |     100 ~ 950            |     0 ~ 1.1      |
    |          +------------+--------------------------+------------------+
    |          |  2.5       |     100 ~ 1250           |     0 ~ 1.5      |
    |   ESP32  +------------+--------------------------+------------------+
    |          |  6         |     150 ~ 1750           |     0 ~ 2.2      |
    |          +------------+--------------------------+------------------+
    |          |  11        |     150 ~ 2450           |     0 ~ 3.9      |
    +----------+------------+--------------------------+------------------+
    |          |  0         |     100 ~ 800            |     0 ~ 1.1      |
    |          +------------+--------------------------+------------------+
    |          |  2.5       |     100 ~ 1100           |     0 ~ 1.5      |
    | ESP32-S2 +------------+--------------------------+------------------+
    |          |  6         |     150 ~ 1350           |     0 ~ 2.2      |
    |          +------------+--------------------------+------------------+
    |          |  11        |     150 ~ 2600           |     0 ~ 3.9      |
    +----------+------------+--------------------------+------------------+
*/

/**
 * @brief  ADC1及输入通道初始化（在特定衰减下表征ADC的特性，并生成ADC电压曲线）
 *      - 支持函数重载，支持输入不定数目的通道参数，ESP32-ADC1通道数目最大值为8，总参数数目为 3~10。
 *      - 由于ADC2不能与WIFI共用，所以尽量优先使用ADC1，且ADC2的读取方式与ADC1不同，也就没有在esayIO中提供ADC2的初始化和读取函数
 *      - 默认设置ADC转换数据宽度为12Bit，0~4095。（几乎不需要更改，需要更改的情况一般是改为8Bit来缩小RAM占用。但ESP32的ADC不支持8Bit，最低9Bit）
 *
 * @param  atten    输入通道衰减系数（决定了输入电压量程）。有 0、2.5、6、11DB 可选，详见上表
 * @param  ch_num   总ADC1输入通道 的数量。channel 与 Pin 对照关系，详见上表
 * @param  (...)    ADC1输入通道列表。支持不定数目参数，数目为 1~8。值为 ADC_CHANNEL_0, ADC_CHANNEL_1... ADC_CHANNEL_7。
 *
 * @return
 *      - none
 *
 */
void adc1_init_with_calibrate(adc_atten_t atten, adc_bits_width_t width)
{
    // 检查 TP两点校准值、Vref，是否被刻录到eFuse中（一般出厂后eFuse中仅有Vref）
    check_efuse();

    // 配置 ADC1 精度
    adc1_config_width(width);

    // 配置 ADC1的端口和衰减
    for (int i = 1; i < adc_last_max; i++) {
        // 配置 ADC1的端口和衰减
        adc1_config_channel_atten(strADCInfo[i].channel, atten);
    }

    adc_calibration_init(atten, width);
}

/**
 * @brief  获取ADC1通道x，经多重采样平均后，并校准补偿后的转换电压，单位mV
 *      - 注意：使用该函数前，一定要用 adc1_init_with_calibrate 对ADC1进行校准并初始化，否则会报错
 *
 * @param  channel 要读取的ADC1输入通道
 * @param  mul_num 多重采样的次数
 *
 * @return
 *     - 电压值，单位mV。uint32_t类型
 *
 */
uint32_t adc1_cal_get_voltage_mul(adc1_channel_t channel, uint32_t mul_num)
{
    uint32_t adc_reading = 0;
    for (uint32_t i = 0; i < mul_num; i++) {
        adc_reading += adc1_get_raw(channel);
    }
    adc_reading /= mul_num;

    if (cali_enable) {
        return esp_adc_cal_raw_to_voltage(adc_reading, &adc1_chars);
    }
    return adc_reading;
}

/**
 * @brief 读取ESP32 ADC数据
 *
 */
static void processADC()
{
    // 读取ADC数据
    for (int i = 1; i < adc_last_max; i++) {
        _KalmanInfo* pKalmanInfo = &KalmanInfo[i];

        TickType_t now = xTaskGetTickCount();
        TickType_t timeChange = (now - strADCInfo[i].lastSampleTime);

        if (timeChange >= pKalmanInfo->parm.Cycle) {
            // 进行采样
            float val = (float)adc1_cal_get_voltage_mul(strADCInfo[i].channel, 10);

            // 原始数据
            strADCInfo[i].originalRam = val;

            // 卡尔曼滤波
            if (pKalmanInfo->parm.UseKalman) {
                val = kalmanFilter(pKalmanInfo, val);
            }

            strADCInfo[i].ram = val;
            strADCInfo[i].lastSampleTime = timeChange;
        }
    }
}

/**
 * @brief 读取SPI MAX6675数据
 *
 * @param now
 */
static void processMAX6675(void)
{
    TickType_t now = xTaskGetTickCount();
    _KalmanInfo* pKalmanInfo = &KalmanInfo[0];
    _strADCInfo* pADCInfo = &strADCInfo[0];

    TickType_t timeChange = (now - pADCInfo->lastSampleTime);

    if (timeChange >= pKalmanInfo->parm.Cycle) {
        // 进行采样
        // float val = MAX6675ReadCelsius();
        float val = 0;
        if (val != -100.0f) {
            // 原始数据
            pADCInfo->originalRam = val;

            // 卡尔曼滤波
            if (pKalmanInfo->parm.UseKalman) {
                val = kalmanFilter(pKalmanInfo, val);
            }

            pADCInfo->ram = val;
            pADCInfo->lastSampleTime = timeChange;
        }
    }
}

/**
 * @brief ADC线程采集任务
 *
 * @param arg
 */
static void adc_task(void* arg)
{
    while (1) {
        // 读取SPI数据
        processMAX6675();

        // 读取ADC数据
        processADC();

        delay(1);
    }
}

/**
 * @brief 打印出所有ADC内容
 *
 * @param argc
 * @param argv
 * @return int
 */
static int do_dumpadc_cmd(int argc, char** argv)
{
    printf("加热台:%1.1f℃ 室温:%1.1f℃ 电压:%1.1fmV\n", adcGetHeatingTemp(), adcGetRoomNTCTemp(), adcGetSystemVol());
    return 0;
}

/**
 * @brief 初始化ADC
 *
 */
void adcInit(void)
{
    adc1_init_with_calibrate(ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12);
    xTaskCreatePinnedToCore(adc_task, "adc", 1024 * 5, NULL, 5, NULL, tskNO_AFFINITY);

    register_cmd("dumpadc", "打印出所有ADC数据", NULL, do_dumpadc_cmd, NULL);
}
