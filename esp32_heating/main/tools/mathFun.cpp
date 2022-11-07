#include "heating.h"
#include "ExternDraw.h"
#include <math.h>

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
 * @brief
 *
 * @param x
 * @param in_min
 * @param in_max
 * @param out_min
 * @param out_max
 * @return long
 */
long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    const long dividend = out_max - out_min;
    const long divisor = in_max - in_min;
    const long delta = x - in_min;
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
float CalculateTemp(float T, float P[], float* pAllTime)
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
