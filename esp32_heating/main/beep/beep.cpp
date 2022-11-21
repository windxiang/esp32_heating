#include "heating.h"
#include <math.h>
#include <string.h>

#define EVENT_BEEP_PLAY (1 << 0) // 播放声音事件

struct _strPlay {
    TONE* MySound;
    uint16_t playSchedule;
    bool isMute;
    EventGroupHandle_t pBeepHandleEvent;
};

static _strPlay currentPlay = {};

// 3声警报声音
TONE BeepSoundAlarm[] = {
    { NOTE_D, CMT_9, 250 },
    { NOTE_D, CMT_7, 250 },
    { NOTE_D, CMT_5, 250 },
    { NOTE_D, CMT_9, 250 },
    { NOTE_D, CMT_8, 250 },
    { NOTE_D, CMT_7, 250 },
    { NOTE_D, CMT_NULL, 250 },

    { NOTE_D, CMT_7, 250 },
    { NOTE_D, CMT_5, 250 },
    { NOTE_D, CMT_9, 250 },
    { NOTE_D, CMT_4, 250 },
    { NOTE_D, CMT_5, 250 },
    { NOTE_D, CMT_NULL, 250 },

    { NOTE_D, CMT_7, 250 },
    { NOTE_D, CMT_5, 250 },
    { NOTE_D, CMT_9, 250 },
    { NOTE_D, CMT_7, 250 },
    { NOTE_D, CMT_M, 250 },
    { NOTE_D, CMT_NULL, 250 },

    { NOTE_MAX, CMT_NULL, 0 },
};

// 启动声音
TONE BeepSoundBoot[] = {
    { NOTE_D, CMT_5, 230 },
    { NOTE_D, CMT_7, 230 },
    { NOTE_D, CMT_9, 215 },
    { NOTE_D, CMT_M, 215 },
    { NOTE_MAX, CMT_NULL, 0 },
};

TONE TipInstall[] = {
    { NOTE_D, CMT_7, 250 },
    { NOTE_D, CMT_M, 250 },
    { NOTE_MAX, CMT_NULL, 0 },
};

TONE TipRemove[] = {
    { NOTE_D, CMT_9, 250 },
    { NOTE_D, CMT_5, 250 },
    { NOTE_MAX, CMT_NULL, 0 },
};

// 滴 的声音
TONE BeepSoundDI[] = {
    { NOTE_B, CMT_E, 20 },
    { NOTE_MAX, CMT_NULL, 0 },
};

// 编码器旋转声音
TONE BeepSoundRotaryAB[] = {
    { NOTE_D, CMT_E, 20 },
    { NOTE_MAX, CMT_NULL, 0 },
};

// 按键按下声音
TONE BeepSoundClick[] = {
    { NOTE_D, CMT_8, 50 },
    { NOTE_MAX, CMT_NULL, 0 },
};

// 按键双击声音
TONE BeepSoundDoubleClick[] = {
    { NOTE_D, CMT_M, 50 },
    { NOTE_D, CMT_NULL, 50 },
    { NOTE_D, CMT_M, 50 },
    { NOTE_MAX, CMT_NULL, 0 },
};

// 按键长按声音
TONE BeepSoundLongClick[] = {
    { NOTE_D, CMT_7, 50 },
    { NOTE_D, CMT_9, 50 },
    { NOTE_D, CMT_M, 50 },
    { NOTE_MAX, CMT_NULL, 0 },
};

/**
 * @brief 设置PWM输出蜂鸣器
 *
 * @param freq
 */
static void SetTone(uint32_t freq)
{
    beepOutput(freq);
}

static float GetNote(note_t note, uint8_t rp)
{
    const uint16_t noteFrequencyBase[] = {
        // C   C#    D     Eb    E     F     F#    G     G#    A     Bb    B
        4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902
    };
    const float FreqRatio = 1.059463094;

    float noteFreq = ((float)noteFrequencyBase[note] / (float)(1 << 4)) * pow(FreqRatio, rp);
    return noteFreq;
}

static bool PlayTones(void)
{
    TONE* pTONE = &currentPlay.MySound[currentPlay.playSchedule];

    if (NULL == pTONE || NOTE_MAX == pTONE->note) {
        SetTone(0);
        return false;
    }

    // 播放音符
    if (pTONE->rp == CMT_NULL) {
        SetTone(0);
    } else {
        SetTone((uint32_t)GetNote(pTONE->note, pTONE->rp));
    }

    // 延时
    delay(pTONE->delay);

    // 下一个音符
    currentPlay.playSchedule++;
    return true;
}

/**
 * @brief 播放蜂鸣器声音
 *
 * @param sound
 */
void SetSound(TONE* pSound, bool isISR)
{
    if (getVolume() && NULL != currentPlay.pBeepHandleEvent && !currentPlay.isMute) {
        currentPlay.MySound = pSound;
        currentPlay.playSchedule = 0;
        if (isISR) {
            BaseType_t xHigherPriorityTaskWoken = pdTRUE;
            xEventGroupSetBitsFromISR(currentPlay.pBeepHandleEvent, EVENT_BEEP_PLAY, &xHigherPriorityTaskWoken);
        } else {
            xEventGroupSetBits(currentPlay.pBeepHandleEvent, EVENT_BEEP_PLAY);
        }
    }
}

static void beep_task(void* arg)
{
    while (1) {
        EventBits_t bits = xEventGroupWaitBits(currentPlay.pBeepHandleEvent, EVENT_BEEP_PLAY, pdTRUE, pdFALSE, portMAX_DELAY);
        xEventGroupClearBits(pHandleEventGroup, bits);

        if (bits & EVENT_BEEP_PLAY) {
            while (PlayTones()) { }
        }
    }
}

/**
 * @brief 初始化蜂鸣器
 *
 */
void beepInit(void)
{
    currentPlay.pBeepHandleEvent = xEventGroupCreate();

    xTaskCreatePinnedToCore(beep_task, "beep", 1024 * 5, NULL, 5, NULL, tskNO_AFFINITY);
}

#if 0
#define NOTE_D0 -1
#define NOTE_D1 294
#define NOTE_D2 330
#define NOTE_D3 350
#define NOTE_D4 393
#define NOTE_D5 441
#define NOTE_D6 495
#define NOTE_D7 556

#define NOTE_DL1 147
#define NOTE_DL2 165
#define NOTE_DL3 175
#define NOTE_DL4 196
#define NOTE_DL5 221
#define NOTE_DL6 248
#define NOTE_DL7 278

#define NOTE_DH1 589
#define NOTE_DH2 661
#define NOTE_DH3 700
#define NOTE_DH4 786
#define NOTE_DH5 882
#define NOTE_DH6 990
#define NOTE_DH7 112
//以上部分是定义是把每个音符和频率值对应起来，其实不用打这么多，但是都打上了，后面可以随意编写

#define WHOLE 1
#define HALF 0.5
#define QUARTER 0.25
#define EIGHTH 0.25
#define SIXTEENTH 0.625
//这部分是用英文对应了拍子，这样后面也比较好看

static int tune[] = {
    NOTE_D0, NOTE_D0, NOTE_D0, NOTE_D6, NOTE_D7, NOTE_DH1, NOTE_D7, NOTE_DH1, NOTE_DH3, NOTE_D7, NOTE_D7, NOTE_D7, NOTE_D3, NOTE_D3,
    NOTE_D6, NOTE_D5, NOTE_D6, NOTE_DH1, NOTE_D5, NOTE_D5, NOTE_D5, NOTE_D3, NOTE_D4, NOTE_D3, NOTE_D4, NOTE_DH1,
    NOTE_D3, NOTE_D3, NOTE_D0, NOTE_DH1, NOTE_DH1, NOTE_DH1, NOTE_D7, NOTE_D4, NOTE_D4, NOTE_D7, NOTE_D7, NOTE_D7, NOTE_D0, NOTE_D6, NOTE_D7,
    NOTE_DH1, NOTE_D7, NOTE_DH1, NOTE_DH3, NOTE_D7, NOTE_D7, NOTE_D7, NOTE_D3, NOTE_D3, NOTE_D6, NOTE_D5, NOTE_D6, NOTE_DH1,
    NOTE_D5, NOTE_D5, NOTE_D5, NOTE_D2, NOTE_D3, NOTE_D4, NOTE_DH1, NOTE_D7, NOTE_D7, NOTE_DH1, NOTE_DH1, NOTE_DH2, NOTE_DH2, NOTE_DH3, NOTE_DH1, NOTE_DH1, NOTE_DH1,
    NOTE_DH1, NOTE_D7, NOTE_D6, NOTE_D6, NOTE_D7, NOTE_D5, NOTE_D6, NOTE_D6, NOTE_D6, NOTE_DH1, NOTE_DH2, NOTE_DH3, NOTE_DH2, NOTE_DH3, NOTE_DH5,
    NOTE_DH2, NOTE_DH2, NOTE_DH2, NOTE_D5, NOTE_D5, NOTE_DH1, NOTE_D7, NOTE_DH1, NOTE_DH3, NOTE_DH3, NOTE_DH3, NOTE_DH3, NOTE_DH3,
    NOTE_D6, NOTE_D7, NOTE_DH1, NOTE_D7, NOTE_DH2, NOTE_DH2, NOTE_DH1, NOTE_D5, NOTE_D5, NOTE_D5, NOTE_DH4, NOTE_DH3, NOTE_DH2, NOTE_DH1,
    NOTE_DH3, NOTE_DH3, NOTE_DH3, NOTE_DH3, NOTE_DH6, NOTE_DH6, NOTE_DH5, NOTE_DH5, NOTE_DH3, NOTE_DH2, NOTE_DH1, NOTE_DH1, NOTE_D0, NOTE_DH1,
    NOTE_DH2, NOTE_DH1, NOTE_DH2, NOTE_DH2, NOTE_DH5, NOTE_DH3, NOTE_DH3, NOTE_DH3, NOTE_DH3, NOTE_DH6, NOTE_DH6, NOTE_DH5, NOTE_DH5,
    NOTE_DH3, NOTE_DH2, NOTE_DH1, NOTE_DH1, NOTE_D0, NOTE_DH1, NOTE_DH2, NOTE_DH1, NOTE_DH2, NOTE_DH2, NOTE_D7, NOTE_D6, NOTE_D6, NOTE_D6, NOTE_D6, NOTE_D7
}; //这部分就是整首曲子的音符部分，用了一个序列定义为tune，整数

static float duration[] = {
    1, 1, 1, 0.5, 0.5, 1 + 0.5, 0.5, 1, 1, 1, 1, 1, 0.5, 0.5,
    1 + 0.5, 0.5, 1, 1, 1, 1, 1, 1, 1 + 0.5, 0.5, 1, 1,
    1, 1, 0.5, 0.5, 0.5, 0.5, 1 + 0.5, 0.5, 1, 1, 1, 1, 1, 0.5, 0.5,
    1 + 0.5, 0.5, 1, 1, 1, 1, 1, 0.5, 0.5, 1 + 0.5, 0.5, 1, 1,
    1, 1, 1, 0.5, 0.5, 1, 0.5, 0.25, 0.25, 0.25, 0.5, 0.5, 0.5, 0.5, 0.25, 0.5, 1,
    0.5, 0.5, 0.5, 0.5, 1, 1, 1, 1, 1, 0.5, 0.5, 1 + 0.5, 0.5, 1, 1,
    1, 1, 1, 0.5, 0.5, 1.5, 0.5, 1, 1, 1, 1, 1, 1,
    0.5, 0.5, 1, 1, 0.5, 0.5, 1.5, 0.25, 0.5, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 0.5, 0.5, 1, 1, 0.5, 0.5,
    1, 0.5, 0.5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0.5, 0.5, 1, 1, 0.5, 0.5, 1, 0.5, 0.25, 0.5, 1, 1, 1, 1, 0.5, 0.5
}; //这部分是整首曲子的节拍部分，也定义个序列duration，浮点（数组的个数和前面音符的个数是一样的，一一对应么）

/**
 * @brief 唱歌 天空之城
 * 唱到末尾还有问题..还要继续调试改BUG
 *
 */
void BeepSingTianKongZhiCheng(void)
{
    for (int x = 0; x < sizeof(tune) / sizeof(tune[0]); x++) {
        SetTone(tune[x]); //此函数依次播放tune序列里的数组，即每个 音符
        delay(400 * duration[x]); //每个音符持续的时间，即节拍duration，是调整时间的越大，曲子速度越慢，越小曲子速度越快，自己掌握吧
    }
}
#endif