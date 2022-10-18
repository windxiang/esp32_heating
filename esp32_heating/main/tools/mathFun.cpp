#include "heating.h"
#include "ExternDraw.h"

/**
 * @brief 获取数值长度
 *
 * @param x  数值
 * @return int 数值长度
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

/*
    @作用 UTF8混合字符串计算水平居中
    @输入：UTF8字符串
    @输出：居中位置
*/
uint32_t UTF8_HMiddle(uint32_t x, uint32_t w, uint8_t size, const char* s)
{
    return x + (w - Get_UTF8_Ascii_Pix_Len(size, s)) / 2;
}