#include "ntc.h"
#include <math.h>
#include <stdio.h>

NTC_Thermistor::NTC_Thermistor(
    const float referenceResistance,
    const float nominalResistance,
    const float nominalTemperatureCelsius,
    const float bValue,
    const float systemVoltage)
{
    this->referenceResistance = referenceResistance;
    this->nominalResistance = nominalResistance;
    this->nominalTemperature = celsiusToKelvins(nominalTemperatureCelsius);
    this->bValue = bValue;
    this->systemVoltage = systemVoltage;
}

/**
 * @brief 读取开尔文温度，转换为摄氏温度并返回。
 *
 * @param voltage
 * @return float 返回摄氏温度
 */
float NTC_Thermistor::readCelsius(float voltage)
{
    return kelvinsToCelsius(readKelvin(voltage));
}

/**
 * @brief 读取开尔文温度，转换为华氏度并返回。
 *
 * @param voltage 当前ADC测量值
 * @return float 返回华氏温度
 */
float NTC_Thermistor::readFahrenheit(float voltage)
{
    return kelvinsToFahrenheit(readKelvin(voltage));
}

/**
 * @brief 读取热敏电阻电阻，转换为开尔文并将其返回。
 *
 * @param voltage 当前ADC测量值
 * @return float 返回开尔文温度
 */
float NTC_Thermistor::readKelvin(float voltage)
{
    return resistanceToKelvins(readResistance(voltage));
}

inline float NTC_Thermistor::resistanceToKelvins(const float resistance)
{
    const float inverseKelvin = 1.0f / this->nominalTemperature + log(resistance / this->nominalResistance) / this->bValue;
    return (1.0f / inverseKelvin);
}

/**
 * @brief 根据ADC电压值 计算NTC阻值
 *
 * @param voltage ADC采集的电压
 * @return float
 */
inline float NTC_Thermistor::readResistance(float voltage)
{
    float Rntc = (voltage * this->referenceResistance) / (this->systemVoltage - voltage);
    // 反过来计算验证正确性
    // float sysVol = Rntc / (this->referenceResistance + Rntc) * this->systemVoltage;
    // printf("adc %f ntc %f sysVol %f\n", voltage, Rntc, sysVol);
    return Rntc;
}

/**
 * @brief 摄氏度到开尔文的转换
 *
 * @param celsius
 * @return float
 */
inline float NTC_Thermistor::celsiusToKelvins(const float celsius)
{
    return (celsius + 273.15);
}

/**
 * @brief 开尔文到摄氏度的转换
 *
 * @param kelvins
 * @return float
 */
inline float NTC_Thermistor::kelvinsToCelsius(const float kelvins)
{
    return (kelvins - 273.15);
}

/**
 * @brief 摄氏度到华氏度的转换
 *
 * @param celsius
 * @return float
 */
inline float NTC_Thermistor::celsiusToFahrenheit(const float celsius)
{
    return (celsius * 1.8 + 32);
}

/**
    开尔文到华氏度的转换：
    F = (K - 273.15) * 1.8 + 32
    Where C = (K - 273.15) is Kelvins To Celsius conversion.
    Then F = C * 1.8 + 32 is Celsius to Fahrenheit conversion.
    => Kelvin convert to Celsius, then Celsius to Fahrenheit.
*/
inline float NTC_Thermistor::kelvinsToFahrenheit(const float kelvins)
{
    return celsiusToFahrenheit(kelvinsToCelsius(kelvins));
}
