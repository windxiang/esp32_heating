#include "heating.h"
#include <math.h>
#include "ntc.h"

/**
 * @brief 获取数值长度
 *
 * @param x
 * @return int
 */
int GetNumberLength(int x)
{
    int i = 0;

    do {
        i++;
        x /= 10;
    } while (x != 0);
    return i;
}

/**
 * @brief 将数据映射到屏幕指定的坐标位置上
 *
 * @param in_cur 当前数据值
 * @param in_min 最小数据值
 * @param in_max 最大数据值
 * @param out_min 屏幕上坐标起点
 * @param out_max 屏幕上坐标终点
 * @return long
 */
long map(long in_cur, long in_min, long in_max, long out_min, long out_max)
{
    const long dividend = out_max - out_min; // 屏幕坐标相差
    const long divisor = in_max - in_min; // 数值相差
    const long delta = in_cur - in_min; // 当前数值 - 最小数值

    if (divisor == 0) {
        return -1;
    }

    return (delta * dividend + (divisor / 2)) / divisor + out_min;
}

/**
 * @brief UTF8混合字符串计算水平居中坐标计算
 *
 * @param x 起始坐标
 * @param w 总宽度
 * @param size 字符串长度(没有用)
 * @param s 字符串
 * @return uint32_t
 */
uint32_t UTF8_HMiddle(uint32_t x, uint32_t w, uint8_t size, const char* s)
{
    return x + (w - Get_UTF8_Ascii_Pix_Len(size, s)) / 2;
}

/**
 * @brief 温度曲线计算
 *
 * @param T 时间差(ms)
 * @param P 设置的参数
 * @param pAllTime 输出总时间长度(ms)
 * @return float
 */
float CalculateTemp(float T, const float P[], float* pAllTime)
{
    float Temperature = 0; // 目标输出温度
    float Time1 = P[1] / P[0]; // 预热区升温时间 = 预热区温度 /  升温斜率
    float Time2 = fabsf(P[4] - P[1]) / P[3]; // 回流区升温时间 = 绝对值(回流区温度 - 预热区温度) / 升温斜率
    float Time3 = P[4] / P[6]; // 回流区温度 / 降温斜率

    if (T <= 0) {
        Temperature = 0;
    } else if (T <= Time1) {
        Temperature = P[0] * T;
    } else if (T <= (Time1 + P[2])) {
        Temperature = P[1];
    } else if (T <= (Time1 + P[2] + Time2)) {
        Temperature = P[1] + P[3] * (T - (Time1 + P[2])) * (P[4] > P[1] ? 1 : -1);
    } else if (T <= (Time1 + P[2] + Time2 + P[5])) {
        Temperature = P[4];
    } else if (T <= (Time1 + P[2] + Time2 + P[5] + Time3)) {
        Temperature = P[4] - P[6] * (T - (Time1 + P[2] + Time2 + P[5]));
    } else {
        Temperature = 0;
    }

    if (pAllTime) {
        *pAllTime = Time1 + P[2] + Time2 + P[5] + Time3; // 整段曲线时间长度 = Time1 + 预热区时间 + Time2 + 回流区时间 + Time3
    }
    return Temperature;
}

/**
 * @brief 随机数生成
 *
 * @param min 生成范围
 * @param max 生成范围
 * @return int32_t
 */
int32_t myRand(int32_t min, int32_t max)
{
    int32_t x = min + rand() % (max - min + 1);
    return x;
}

/**
 * @brief 卡尔曼滤波器
 *
 * @param kfp
 * @param input
 * @return float
 */
float kalmanFilter(_KalmanInfo* kfp, float input)
{
    // 预测协方差方程：k时刻系统估算协方差 = k-1时刻的系统协方差 + 过程噪声协方差
    kfp->filter.Now_P = kfp->filter.LastP + kfp->parm.KalmanQ;

    // 卡尔曼增益方程：卡尔曼增益 = k时刻系统估算协方差 / （k时刻系统估算协方差 + 观测噪声协方差）
    kfp->filter.Kg = kfp->filter.Now_P / (kfp->filter.Now_P + kfp->parm.KalmanR);

    // 更新最优值方程：k时刻状态变量的最优值 = 状态变量的预测值 + 卡尔曼增益 * （测量值 - 状态变量的预测值）
    kfp->filter.out = kfp->filter.out + kfp->filter.Kg * (input - kfp->filter.out); //因为这一次的预测值就是上一次的输出值

    // 更新协方差方程: 本次的系统协方差付给 kfp->LastP 威下一次运算准备。
    kfp->filter.LastP = (1 - kfp->filter.Kg) * kfp->filter.Now_P;

    return kfp->filter.out;
}

/**
 * @brief 转换NTC到温度值
 *
 * @param adcVoltage 原始ADC采集的电压
 * @param adcVoltage 系统参考电压(5000mV)
 * @return float 转换成摄氏度
 */
float convertNTCTemp(float adcVoltage, float systemReferenceVoltage)
{
    const float R0 = 10.0f * 1000.0f; // R0电阻阻值 (10K)
    const float Rn = 10.0f * 1000.0f; // Rntc电阻阻值 (10K)
    const float Tn = 25.0f; // Tn - nominal temperature in Celsius.
    const float B = 3950.0f; // B - b-value of a thermistor.

    // 调用 NTC_Thermistor 类来进行转换
    NTC_Thermistor thermistor(R0, Rn, Tn, B, systemReferenceVoltage);
    const float celsius = (float)thermistor.readCelsius(adcVoltage);
    return celsius;
}

/**
 * @brief 根据NTC当前电阻 计算出NTC对应的温度
 * 当前电阻计算: float Rntc = (adc * 10000) / (供电电压 - adc);
 * GND 串 10K电阻 串 ntc 串vcc
 *
 * @param NTC_Res 当前电阻值
 * @return float
 */
float Get_Temp(uint16_t NTC_Res)
{
    // Rt = R *EXP(B*(1/T1-1/T2))
    const float Rp = 10000.0f; // 10K
    const float T2 = 273.15f + 25.0f; // T2
    const float Bx = 3950.0f; // B
    const float Ka = 273.15f;
    const float Rt = NTC_Res;

    float temp;
    temp = Rt / Rp;
    temp = log(temp); // ln(Rt/Rp)
    temp /= Bx; // ln(Rt/Rp)/B
    temp += (1 / T2);
    temp = 1 / (temp);
    temp -= Ka;
    return temp;
}

/**
 * @brief 未验证
 * 根据ADC原始数据  求出对应NTC温度
 * 10K电阻串ntc，ntc接gnd端
 *
 * @param adc ADC原始数据
 * @return float
 */
float Get_Tempture(float adc)
{
    const float B = 3950.0f; //温度系数
    const float TN = 298.15f; //额定温度(绝对温度加常温:273.15+25)
    const float R = 10000.0f; // 固定电阻阻值(欧)
    const float RN = 10000.0f; // NTC额定阻值(绝对温度时的电阻值10k)(欧)
    const float BaseVol = 4980.0f; // ADC基准电压(mV)
    float RV, RT, Tmp;
    RV = BaseVol / 4096.0f * (float)adc; // 4096为ADC的采集精度  求出NTC两端电压
    RT = RV * R / (BaseVol - RV); //求出当前温度阻值
    Tmp = 1 / (1 / TN + (log(RT / RN) / B)) - 273.15; //%RT = RN exp*B(1/T-1/TN)%
    return Tmp;
}

/**
 * @brief 解一元一次方程   给出两点坐标和第三点的x值或y值  得出第三点y值或x值
 *
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 * @param Unkown_x
 * @param Unkown_y
 * @return float
 */
static float OneDimensionalEquation(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t Unkown_x, uint16_t Unkown_y)
{
    float k = 0;
    float b = 0;

    k = (((int32_t)y1 - (int32_t)y2) / ((int32_t)x1 - (int32_t)x2));
    b = y1 - x1 * k;

    if (0 == Unkown_x) //如果unknown_x设为0 就是求x的值 否则求y值
        return ((float)Unkown_y - b) / k;
    else
        return ((float)Unkown_x * k + b);
}

/**
 * @brief 查表法  根据当前NTC电阻阻值 计算出温度
 * 当前电阻计算: float Rntc = (adc * 10000) / (供电电压 - adc);
 *
 * @param NTC_Res 采集到的NTC电阻值
 * @return float 计算后的摄氏度
 */
float Res_To_Temperature(uint16_t NTC_Res)
{
    /*10K NTC 温度与阻值对应表 X轴代表温度  Y轴代表阻值*/
    const uint16_t X_Temp[17] = { 0, 6, 12, 18, 25, 31, 37, 45, 50, 57, 63, 70, 76, 83, 89, 94, 100 };
    const uint16_t Y_Res[17] = { 31908, 23698, 17794, 13498, 9900, 7625, 5925, 4288, 3529, 2709, 2177, 1701, 1386, 1101, 909, 778, 649 };
    uint8_t Loop = 0;
    uint8_t StartPoint = 0;
    float RealTemp = 0;

    /*查找阻值所对应的区间*/
    for (Loop = 0; Loop < 17; Loop++) {
        if (NTC_Res > Y_Res[Loop + 1]) {
            StartPoint = Loop;
            break;
        }
    }

    RealTemp = (float)OneDimensionalEquation(X_Temp[StartPoint], Y_Res[StartPoint], X_Temp[StartPoint + 1], Y_Res[StartPoint + 1], 0, NTC_Res);
    return RealTemp;
}

#if 0
/**
 * @brief 转换NTC到温度值 使用查表的方法
 *
 * @param adcVoltage 原始ADC采集的电压
 * @return float
 */
float convertNTCTempByTable(float adcVoltage)
{
    // NTC 3950 10K 1% MF52
    const static uint16_t ntc_table[] = {
        3901, 3888, 3875, 3861, 3846, 3831, 3814, 3797, 3779, 3761,
        3741, 3721, 3699, 3677, 3654, 3630, 3605, 3579, 3552, 3525,
        3496, 3466, 3435, 3404, 3371, 3338, 3303, 3268, 3232, 3195,
        3157, 3118, 3079, 3038, 2998, 2956, 2914, 2871, 2827, 2783,
        2739, 2694, 2648, 2603, 2557, 2511, 2464, 2418, 2371, 2325,
        2278, 2232, 2186, 2140, 2094, 2048, 2003, 1958, 1913, 1869,
        1825, 1782, 1739, 1697, 1655, 1614, 1574, 1534, 1495, 1457,
        1419, 1382, 1346, 1310, 1275, 1241, 1208, 1175, 1143, 1112,
        1082, 1052, 1023, 995, 967, 940, 914, 888, 863, 839,
        816, 793, 770, 749, 728, 707, 687, 668, 649, 631,
        613, 596, 579, 563, 547, 532, 517, 502, 488, 475,
        462, 449, 436, 424, 413, 401, 390, 380, 369, 359,
        350, 340, 331, 322, 314, 305, 297, 289, 282, 274,
        267, 260, 253, 247, 240, 234, 228, 222, 217, 211,
        206, 201, 196, 191, 186, 181, 177, 173, 168, 164,
        160, 156, 153, 149, 145, 142, 139, 135, 132, 129,
        126, 123, 120, 117, 115, 112, 110, 107, 105, 102,
        100, 98, 96, 94, 91, 89, 88, 86, 84, 82,
        80
    };

    uint16_t i, j = 0;
    uint16_t interpolation_temp;
    uint16_t aa, bb;
    float sub_temp;
    float temp;

    for (i = 0; i < sizeof(ntc_table) / sizeof(ntc_table[1]); i++) {
        if (adcVoltage >= ntc_table[i]) {
            j = i;
            break;
        }
    }

    aa = ntc_table[j];
    bb = ntc_table[j + 1];

    interpolation_temp = aa - bb;
    interpolation_temp = interpolation_temp * 10;
    interpolation_temp = interpolation_temp / 10;

    sub_temp = adcVoltage - aa;
    sub_temp = sub_temp * 10;

    temp = (j * 10) + (10 - (sub_temp / interpolation_temp));
    return temp;
}
#endif

/**
 * @brief 使用插值算法来将 ADC 数据转换为温度 (ADC 采集N型热电偶)
 *
 * @param ad_data ADC采集的电压数据 单位(mV) 放大倍数:100倍
 * @return float 转换后的温度值
 */
float ConverNThermocoupleInterpolation(uint16_t ad_data)
{

    // N型热电偶转温度
    const uint16_t Thermocouple_table[60] = {
        0, 26, 52, 79, 106,
        134, 161, 190, 218, 248,
        277, 307, 337, 368, 398,
        430, 461, 493, 525, 558,
        591, 624, 657, 691, 725,
        759, 794, 828, 863, 898,
        934, 969, 1005, 1041, 1077,
        1113, 1150, 1186, 1223, 1260,
        1297, 1334, 1371, 1409, 1446,
        1484, 1522, 1560, 1598, 1633,
        1674, 1713, 1751, 1790, 1828,
        1867, 1905, 1944, 1983, 2022
    };

    for (int i = 0; i < sizeof(Thermocouple_table) / sizeof(Thermocouple_table[0]); i++) {
        if (ad_data < Thermocouple_table[i]) {
            float interpolation_temp = Thermocouple_table[i] - Thermocouple_table[i - 1];
            float sub_temp = (ad_data - Thermocouple_table[i - 1]) * 10.0f;
            float temp = (i * 10.0f) + ((float)sub_temp / (float)interpolation_temp);
            return temp;
        }
    }

    // 转换错误
    return -1000.0f;
}
