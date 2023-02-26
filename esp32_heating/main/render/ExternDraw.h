#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "stdint.h"

void ClearOLEDBuffer(void);
void Display(void);

void Blur(int sx, int sy, int ex, int ey, int f, int delay);
void DrawScrollBar(int x, int y, int w, int h, int iTotal, int iCurPos);
void Draw_Num_Bar(float i, float a, float b, int x, int y, int w, int h, int c);
void Draw_Pixel_Resize(int x, int y, int ox, int oy, int w, int h);
void Draw_Slow_Bitmap(int x, int y, const unsigned char* bitmap, unsigned char w, unsigned char h);
void Draw_Slow_Bitmap_Resize(int x, int y, uint8_t* bitmap, int w1, int h1, int w2, int h2);

void DrawMsgBox(const char* s);
void DrawHighLightText(int x, int y, const char* s);

void TextEditor(const char* title, char* text, int32_t textSize);

uint32_t Get_UTF8_Ascii_Pix_Len(uint8_t size, const char* s);

void DrawPageFootnotes(int curPage, int TotalPage);
void Draw_APP(int x, int y, uint8_t* bitmap);
void DrawTempCurve(void);
void PopWindows(const char* s);

void RenderScreenSavers(void);

#ifdef __cplusplus
}
#endif // __cplusplus