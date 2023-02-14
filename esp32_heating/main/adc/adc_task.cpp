#include "heating.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"
#include "stdlib.h"
#include <math.h>

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

struct _strADCInfo {
    float originalRam; // ADC 原始数据
    float ram; // adc 滤波后的数据
    adc1_channel_t channel; // adc 采集通道
    TickType_t lastSampleTime; // 最后一次采样时间
    adc_atten_t atten;
    esp_adc_cal_characteristics_t characteristics_t;
};

// 出厂是否校准过
static bool cali_enable = false;

static _strADCInfo strADCInfo[adc_last_max] = {
    [adc_HeatingTemp] = { 0, 0, ADC1_CHANNEL_MAX, 0, ADC_ATTEN_DB_11, {} }, // 加热台K型热电偶 这一路不属于ADC范围里  是使用MAX6675芯片进行采集的
    [adc_T12Temp] = { 0, 0, ADC1_CHANNEL_0, 0, ADC_ATTEN_DB_11, {} }, // T12 烙铁头温度
    [adc_T12Cur] = { 0, 0, ADC1_CHANNEL_1, 0, ADC_ATTEN_DB_11, {} }, // T12 电流
    [adc_T12NTC] = { 0, 0, ADC1_CHANNEL_6, 0, ADC_ATTEN_DB_11, {} }, // T12 NTC
    [adc_SystemVol] = { 0, 0, ADC1_CHANNEL_7, 0, ADC_ATTEN_DB_11, {} }, // 系统输入电压
    [adc_SystemRef] = { 0, 0, ADC1_CHANNEL_3, 0, ADC_ATTEN_DB_11, {} }, // 系统5V电压
    [adc_RoomTemp] = { 0, 0, ADC1_CHANNEL_2, 0, ADC_ATTEN_DB_11, {} }, // PCB NTC 温度
};

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
 * @brief 获取T12烙铁头温度 运放采集
 *
 * @return float
 */
float adcGetT12Temp(void)
{
    const float calibrationVal = KalmanInfo[adc_T12Temp].parm.calibrationVal;
    return (float)strADCInfo[adc_T12Temp].ram + calibrationVal;
}

/**
 * @brief 获取T12电流 运放采集
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
    const float adcVoltage = strADCInfo[adc_T12NTC].ram; // ADC 采集的电压值mV
    const float systemVolage = adcGetSystem5VVol();
    return convertNTCTemp(adcVoltage, systemVolage) + calibrationVal;
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
 * @brief 获取系统5V电压(mV)
 *
 * @return float
 */
float adcGetSystem5VVol(void)
{
    const float up = 5.0f * 1000.0f;
    const float down = 5.0f * 1000.0f;
    const float calibrationVal = KalmanInfo[adc_SystemRef].parm.calibrationVal;

    uint32_t voltage = strADCInfo[adc_SystemRef].ram * (down + up) / down;
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
    const float adcVoltage = strADCInfo[adc_RoomTemp].ram; // ADC 采集的电压值mV
    const float systemVolage = adcGetSystem5VVol();
    return convertNTCTemp(adcVoltage, systemVolage) + calibrationVal;
}

// 检查 TP两点校准值、Vref参考电压值 是否被刻录到eFuse中
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

/**
 * @brief 得到ADC的校准值
 *
 * @param width
 * @return true
 * @return false
 */
static void adc_calibration_init(_strADCInfo* pADCInfo, adc_bits_width_t width)
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

        esp_adc_cal_characterize(ADC_UNIT_1, pADCInfo->atten, width, 0, &pADCInfo->characteristics_t);
        // esp_adc_cal_characterize(ADC_UNIT_2, atten, width, 0, &adc2_chars);
    } else {
        ESP_LOGE(TAG, "Invalid arg");
    }
}

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
void adc1_init_with_calibrate(adc_bits_width_t width)
{
    // 检查 TP两点校准值、Vref，是否被刻录到eFuse中（一般出厂后eFuse中仅有Vref）
    check_efuse();

    // 配置 ADC1 精度
    adc1_config_width(width);

    // 配置 ADC1的端口和衰减
    for (int i = 1; i < adc_last_max; i++) {
        adc1_config_channel_atten(strADCInfo[i].channel, strADCInfo[i].atten);
        adc_calibration_init(&strADCInfo[i], width);
    }
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
uint32_t adc1_cal_get_voltage_mul(_strADCInfo* pADCInfo, uint32_t mul_num)
{
    uint32_t raw = 0;
    for (uint32_t i = 0; i < mul_num; i++) {
        raw += adc1_get_raw(pADCInfo->channel);
    }
    raw /= mul_num;

    if (cali_enable) {
        return esp_adc_cal_raw_to_voltage(raw, &pADCInfo->characteristics_t);
    }
    return raw;
}

/**
 * @brief 处理ADC数据
 *
 * @param pKalmanInfo
 */
void adcProcess(int adcType, TickType_t now)
{
    _KalmanInfo* pKalmanInfo = &KalmanInfo[adcType]; // 获取卡尔曼周期
    _strADCInfo* pAdcInfo = &strADCInfo[adcType]; // 获取ADC配置

    // 进行采样
    float val = (float)adc1_cal_get_voltage_mul(pAdcInfo, 10);

    // 原始数据
    pAdcInfo->originalRam = val;

    // 卡尔曼滤波
    if (pKalmanInfo->parm.UseKalman) {
        val = kalmanFilter(pKalmanInfo, val);
    }

    pAdcInfo->ram = val;
    pAdcInfo->lastSampleTime = now;

    if (adcType == adc_RoomTemp) {
        // printf("%1.1f\n", pAdcInfo->ram);
    } else if (adcType == adc_SystemVol) {
        // printf("%1.1f\n", pAdcInfo->ram);
    } else if (adcType == adc_T12Temp) {
        // printf("4:%1.0f, %1.1f, %1.1f, %1.1f\n", pAdcInfo->originalRam, pAdcInfo->ram, adcGetHeatingTemp(), ConverNThermocoupleInterpolation(pAdcInfo->ram));
    } else if (adcType == adc_SystemRef) {
        // printf("%1.1f\n", pAdcInfo->ram);
    }
}

/**
 * @brief 读取ESP32 ADC数据
 *
 */
static void _processADC(void)
{
    // adc_T12Temp 不在这里进行处理 因为这个温度要根据PWM低电平时候才可以读取
    for (int i = adc_T12Cur; i < adc_last_max; i++) {
        const _KalmanInfo* pKalmanInfo = &KalmanInfo[i]; // 获取卡尔曼周期
        const _strADCInfo* pAdcInfo = &strADCInfo[i]; // 获取ADC配置

        const TickType_t now = xTaskGetTickCount();
        const TickType_t timeChange = (now - pAdcInfo->lastSampleTime);
        if (timeChange >= pKalmanInfo->parm.Cycle) {
            adcProcess(i, now);
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
        float temp = 0;
        if (MAX6675ReadCelsius(&temp)) {
            // 原始数据
            pADCInfo->originalRam = temp;

            // 卡尔曼滤波
            if (pKalmanInfo->parm.UseKalman) {
                temp = kalmanFilter(pKalmanInfo, temp);
            }

            pADCInfo->ram = temp;
            pADCInfo->lastSampleTime = now;

            // printf("%1.1f\n", pADCInfo->ram);
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
    delay(300);

    while (1) {
        // 读取SPI数据
        processMAX6675();

        // 读取ADC数据
        _processADC();

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
    ESP_LOGI(TAG, "加热台:%1.1f℃ 室温:%1.1f℃ 输入电压:%1.1fmV 5V电压:%1.1fmV\n", adcGetHeatingTemp(), adcGetRoomNTCTemp(), adcGetSystemVol(), adcGetSystem5VVol());
    ESP_LOGI(TAG, "T12 NTC:%1.1f 手柄状态:%s\n", adcGetT12NTC(), t12_isPutDown() == true ? "放下" : "手上");
    ESP_LOGI(TAG, "烙铁头温度:%1.1f 电流:%1.1f\n", adcGetT12Temp(), adcGetT12Cur());
    return 0;
}

/**
 * @brief 初始化ADC
 *
 */
void adcInit(void)
{
    adc1_init_with_calibrate(ADC_WIDTH_BIT_12);
    xTaskCreatePinnedToCore(adc_task, "adc", 1024 * 5, NULL, 5, NULL, tskNO_AFFINITY);

    register_cmd("dumpadc", "打印出所有ADC数据", NULL, do_dumpadc_cmd, NULL);
}
