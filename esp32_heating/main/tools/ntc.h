#pragma once

class NTC_Thermistor {

private:
    float referenceResistance;
    float nominalResistance;
    float nominalTemperature; // in Celsius.
    float bValue;
    float systemVoltage;

public:
    /**
      Constructor
      @param referenceResistance - 上电阻
      @param nominalResistance - NTC电阻标称值
      @param nominalTemperature - nominal temperature in Celsius
      @param bValue - b-value of a thermistor
      @param systemVoltage - 电压
    */
    NTC_Thermistor(
        float referenceResistance,
        float nominalResistance,
        float nominalTemperatureCelsius,
        float bValue,
        float systemVoltage);

    /**
      Reads a temperature in Celsius from the thermistor.

      @return temperature in degree Celsius
    */
    float readCelsius(float voltage);

    /**
      Reads a temperature in Kelvin from the thermistor.

      @return temperature in degree Kelvin
    */
    float readKelvin(float voltage);

    /**
      Reads a temperature in Fahrenheit from the thermistor.

      @return temperature in degree Fahrenheit
    */
    float readFahrenheit(float voltage);

private:
    /**
      Resistance to Kelvin conversion:
      1/K = 1/K0 + ln(R/R0)/B;
      K = 1 / (1/K0 + ln(R/R0)/B);
      Where
      K0 - nominal temperature,
      R0 - nominal resistance at a nominal temperature,
      R - the input resistance,
      B - b-value of a thermistor.

      @param resistance - resistance value to convert
      @return temperature in degree Kelvin
    */
    inline float resistanceToKelvins(float resistance);

    /**
      计算热敏电阻的电阻:
      Converts a value of the thermistor sensor into a resistance.
      R = R0 / (ADC / V - 1);
      Where
      R0 - 标称温度下的标称电阻
      R - NTC阻值
      ADC - ADC采集分辨率
      V - 当前ADC采集电压.

      @return resistance of the thermistor sensor.
    */
    inline float readResistance(float voltage);

    /**
      摄氏度到开尔文的转换：
      K = C + 273.15

      @param celsius - temperature in degree Celsius to convert
      @return temperature in degree Kelvin
    */
    inline float celsiusToKelvins(float celsius);

    /**
      开尔文到摄氏度的转换：
      C = K - 273.15

      @param kelvins - temperature in degree Kelvin to convert
      @return temperature in degree Celsius
    */
    inline float kelvinsToCelsius(float kelvins);

    /**
      摄氏度到华氏度的转换：
      F = C * 1.8 + 32

      @param celsius - temperature in degree Celsius to convert
      @return temperature in degree Fahrenheit
    */
    inline float celsiusToFahrenheit(float celsius);

    /**
      开尔文到华氏度的转换：
      F = (K - 273.15) * 1.8 + 32

      @param kelvins - temperature in degree Kelvin to convert
      @return temperature in degree Fahrenheit
    */
    inline float kelvinsToFahrenheit(float kelvins);
};
