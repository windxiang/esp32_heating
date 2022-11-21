#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

enum CALCULATORMUSICTONE {
    CMT_NULL = 255,
    CMT_1 = 0,
    CMT_2 = 2,
    CMT_3 = 3,
    CMT_4 = 5,
    CMT_5 = 7,
    CMT_6 = 9,
    CMT_7 = 11,
    CMT_8 = 12,
    CMT_9 = 14,
    CMT_P = 15,
    CMT_S = 17,
    CMT_M = 18,
    CMT_D = 21,
    CMT_E = 22,
};

typedef enum {
    NOTE_C,
    NOTE_Cs,
    NOTE_D,
    NOTE_Eb,
    NOTE_E,
    NOTE_F,
    NOTE_Fs,
    NOTE_G,
    NOTE_Gs,
    NOTE_A,
    NOTE_Bb,
    NOTE_B,
    NOTE_MAX
} note_t;

struct TONE {
    note_t note;
    CALCULATORMUSICTONE rp;
    TickType_t delay;
};

extern TONE BeepSoundAlarm[];
extern TONE BeepSoundBoot[];
extern TONE TipInstall[];
extern TONE TipRemove[];
extern TONE BeepSoundDI[];
extern TONE BeepSoundRotaryAB[];
extern TONE BeepSoundClick[];
extern TONE BeepSoundDoubleClick[];
extern TONE BeepSoundLongClick[];

void beepInit(void);
void SetSound(TONE* pSound, bool isISR);
void BeepSingTianKongZhiCheng(void);

#ifdef __cplusplus
}
#endif // __cplusplus
