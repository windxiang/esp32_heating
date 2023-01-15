#include "bitmap.h"

uint8_t QRC[] = { 14, 0x05, 0x80, 0x74, 0xb8, 0x57, 0xa8, 0x76, 0xb8, 0x05, 0x80, 0xf9, 0x7c, 0x46, 0x94, 0xaa, 0xa8, 0xf9, 0x7c, 0x06, 0x88, 0x74, 0xa8, 0x57, 0x8c, 0x75, 0x74, 0x06, 0x98 };
// 面板设置
uint8_t Set0[] = { 14, 0x88, 0x24, 0x08, 0x20, 0x38, 0x30, 0x38, 0x30, 0x38, 0x30, 0x38, 0x30, 0x38, 0x30, 0x38, 0x30, 0x39, 0x30, 0x3b, 0xb0, 0x3f, 0xf0, 0x3f, 0xf0, 0x00, 0x00, 0x80, 0x04 };
uint8_t Set1[] = { 14, 0xf8, 0x7c, 0xf3, 0x3c, 0xf4, 0xbc, 0xf6, 0xbc, 0xf4, 0xbc, 0xf6, 0xbc, 0xf4, 0xbc, 0xe4, 0x9c, 0xc8, 0x4c, 0xd0, 0x2c, 0xd0, 0x2c, 0xc8, 0x4c, 0xe7, 0x9c, 0xf0, 0x3c };
uint8_t Set2[] = { 14, 0x8f, 0xc4, 0x10, 0x20, 0x20, 0x10, 0x4c, 0xc8, 0x9f, 0xe4, 0x3d, 0xf0, 0x3d, 0xf0, 0x1d, 0xe0, 0x3e, 0xf0, 0x3f, 0x70, 0x9f, 0xe4, 0x8c, 0xc4, 0x00, 0x00, 0x30, 0x30 };
uint8_t Set3[] = { 14, 0xc3, 0xfc, 0x18, 0x00, 0x18, 0x00, 0xc3, 0xfc, 0xff, 0xfc, 0xff, 0x0c, 0x00, 0x60, 0x00, 0x60, 0xff, 0x0c, 0xff, 0xfc, 0xf0, 0xfc, 0x06, 0x00, 0x06, 0x00, 0xf0, 0xfc };
uint8_t Set4[] = { 14, 0x80, 0x04, 0x00, 0x00, 0x20, 0x00, 0x10, 0x00, 0x08, 0x00, 0x10, 0x00, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x04 };
uint8_t Set4F[] = { 14, 0x80, 0x04, 0x00, 0x00, 0x3f, 0xf0, 0x38, 0x30, 0x33, 0x10, 0x3f, 0x90, 0x38, 0x10, 0x33, 0x90, 0x27, 0x90, 0x26, 0x10, 0x30, 0x90, 0x3f, 0xf0, 0x00, 0x00, 0x80, 0x04 };
uint8_t Set4FF[] = { 14, 0x80, 0x04, 0x00, 0x00, 0x3f, 0xf0, 0x24, 0x30, 0x21, 0x90, 0x27, 0x90, 0x27, 0x30, 0x20, 0x70, 0x27, 0xf0, 0x23, 0x30, 0x30, 0x70, 0x3f, 0xf0, 0x00, 0x00, 0x80, 0x04 };
uint8_t Set5[] = { 14, 0xfc, 0xfc, 0xf8, 0x7c, 0xe0, 0x1c, 0xc0, 0x0c, 0xc0, 0x0c, 0xc0, 0x0c, 0xc0, 0x0c, 0xc0, 0x0c, 0xc0, 0x0c, 0x80, 0x04, 0x80, 0x04, 0xff, 0xfc, 0xf8, 0x7c, 0xfc, 0xfc };
uint8_t Set5_1[] = { 14, 0xfc, 0xf4, 0xf8, 0x60, 0xe0, 0x44, 0xc0, 0x8c, 0xc1, 0x1c, 0xc2, 0x2c, 0xc4, 0x4c, 0xc8, 0x8c, 0xd1, 0x0c, 0xa2, 0x04, 0xc4, 0x04, 0x8f, 0xfc, 0x18, 0x7c, 0xbc, 0xfc };
uint8_t Set6[] = { 14, 0xf8, 0x7c, 0xc0, 0x0c, 0xc0, 0x0c, 0xcf, 0xcc, 0xcf, 0xcc, 0xcf, 0xcc, 0xcf, 0xcc, 0xcf, 0xcc, 0xcf, 0xcc, 0xc8, 0x4c, 0xc8, 0x4c, 0xcf, 0xcc, 0xc0, 0x0c, 0xc0, 0x0c };
uint8_t Set_LANG[] = { 14, 0xf0, 0x3c, 0xce, 0x4c, 0xbc, 0x34, 0xb8, 0x14, 0x18, 0x80, 0x10, 0x80, 0x42, 0x30, 0x60, 0x78, 0x60, 0xf8, 0x44, 0xc0, 0x80, 0x04, 0xb8, 0x84, 0xcf, 0xcc, 0xf0, 0x3c };
uint8_t Set7[] = { 14, 0x80, 0x0c, 0x00, 0x1c, 0x3f, 0xf4, 0x3f, 0xe0, 0x3f, 0xc4, 0x37, 0x8c, 0x23, 0x18, 0x30, 0x30, 0x38, 0x70, 0x3c, 0xf0, 0x3f, 0xf0, 0x3f, 0xf0, 0x00, 0x00, 0x80, 0x04 };
uint8_t Set11[] = { 14, 0xf3, 0xfc, 0xe7, 0xfc, 0xce, 0x0c, 0x8e, 0x1c, 0x9f, 0x9c, 0x1f, 0x3c, 0x1f, 0x0c, 0x0e, 0x0c, 0x0f, 0xfc, 0x87, 0xf8, 0x81, 0xe4, 0xc0, 0x0c, 0xe0, 0x1c, 0xf8, 0x7c }; // 休眠触发(秒)
uint8_t Set8[] = { 14, 0x81, 0xdc, 0x55, 0x9c, 0x29, 0x04, 0x55, 0x00, 0x29, 0x90, 0x55, 0xd0, 0x03, 0xfc, 0xff, 0x00, 0x2e, 0x00, 0x26, 0x70, 0x02, 0x50, 0x82, 0x70, 0xe6, 0x00, 0xee, 0x04 };
uint8_t Set9[] = { 14, 0xf0, 0x3c, 0xce, 0x0c, 0xbf, 0x04, 0xb3, 0x04, 0x73, 0x00, 0x7f, 0x00, 0x7e, 0x00, 0x7c, 0x00, 0x7c, 0x00, 0x7c, 0x60, 0xbc, 0x64, 0xbe, 0x04, 0xcf, 0x0c, 0xf0, 0x3c };
uint8_t Set10[] = { 14, 0xf8, 0x7c, 0xf7, 0xbc, 0x00, 0x00, 0x7f, 0xf8, 0x00, 0x00, 0xbf, 0xf4, 0xab, 0x54, 0xab, 0x54, 0xab, 0x54, 0xab, 0x54, 0xab, 0x54, 0xab, 0x54, 0xbf, 0xf4, 0xc0, 0x0c };
uint8_t Set13[] = { 14, 0x80, 0x04, 0x00, 0x00, 0x3f, 0xf0, 0x3c, 0xf0, 0x34, 0xb0, 0x24, 0x90, 0x24, 0x90, 0x27, 0x90, 0x23, 0x10, 0x30, 0x30, 0x38, 0x70, 0x3f, 0xf0, 0x00, 0x00, 0x80, 0x04 };
uint8_t Set12[] = { 14, 0x80, 0x04, 0x00, 0x00, 0x3f, 0xf0, 0x3c, 0xf0, 0x3c, 0xf0, 0x3c, 0xf0, 0x20, 0x10, 0x20, 0x10, 0x3c, 0xf0, 0x3c, 0xf0, 0x3c, 0xf0, 0x3f, 0xf0, 0x00, 0x00, 0x80, 0x04 };
uint8_t Set14[] = { 14, 0xff, 0x7c, 0xfe, 0x7c, 0xfc, 0xec, 0xe8, 0xdc, 0xc8, 0x4c, 0x98, 0x4c, 0x88, 0x24, 0x08, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x04, 0xe0, 0x1c };
uint8_t Set15[] = { 14, 0x15, 0x54, 0x3f, 0xfc, 0x15, 0x54, 0x3f, 0xfc, 0x15, 0x54, 0x3c, 0x04, 0x14, 0x04, 0x3c, 0xfc, 0x14, 0x54, 0x3c, 0xfc, 0x00, 0x54, 0x00, 0xa8, 0x00, 0x00, 0x00, 0x00 };
uint8_t Set16[] = { 14, 0x15, 0x54, 0x3f, 0xf8, 0x15, 0x50, 0x3f, 0xf0, 0x15, 0x50, 0x38, 0x70, 0x10, 0x20, 0x31, 0x04, 0x13, 0x8c, 0x22, 0xa8, 0x17, 0xfc, 0x2a, 0xa8, 0x00, 0x00, 0x00, 0x00 };
uint8_t Set17[] = { 14, 0x80, 0x00, 0xbf, 0xf8, 0x1f, 0xf8, 0xb0, 0x08, 0x18, 0x08, 0xb9, 0xe8, 0x19, 0xb8, 0xb8, 0x38, 0x18, 0x38, 0xb9, 0xb8, 0xb9, 0xf8, 0xb0, 0xf8, 0xbf, 0xf8, 0x80, 0x00 };
uint8_t Set18[] = { 14, 0x80, 0x00, 0xbf, 0xf8, 0x1f, 0xf8, 0xb0, 0x28, 0x1f, 0xf8, 0xb4, 0x08, 0x1f, 0xf8, 0xb1, 0x08, 0x1f, 0xf8, 0xb0, 0x48, 0xbf, 0xf8, 0xb2, 0x88, 0xbf, 0xf8, 0x80, 0x00 };
uint8_t SetTrend[] = { 14, 0xdf, 0xfc, 0xdf, 0xfc, 0xdf, 0xfc, 0xdf, 0xfc, 0xdf, 0xfc, 0xdf, 0xfc, 0xdf, 0xf8, 0xdf, 0x74, 0xdc, 0xac, 0xd3, 0xdc, 0xcf, 0xfc, 0x00, 0x00, 0xdf, 0xfc, 0xdf, 0xfc };
uint8_t Lang_CN[] = { 14, 0x80, 0x04, 0x00, 0x00, 0x3f, 0xf0, 0x3c, 0xf0, 0x3c, 0xf0, 0x20, 0x10, 0x24, 0x90, 0x24, 0x90, 0x20, 0x10, 0x3c, 0xf0, 0x3c, 0xf0, 0x3f, 0xf0, 0x00, 0x00, 0x80, 0x04 };
uint8_t Lang_EN[] = { 14, 0x80, 0x04, 0x00, 0x00, 0x3f, 0xf0, 0x38, 0x70, 0x30, 0x30, 0x23, 0x10, 0x27, 0x90, 0x20, 0x10, 0x20, 0x10, 0x27, 0x90, 0x27, 0x90, 0x3f, 0xf0, 0x00, 0x00, 0x80, 0x04 };
uint8_t Lang_JP[] = { 14, 0x80, 0x04, 0x00, 0x00, 0x3f, 0xf0, 0x3d, 0xf0, 0x20, 0x10, 0x3b, 0xf0, 0x3b, 0x70, 0x30, 0x30, 0x2b, 0x50, 0x2a, 0xd0, 0x31, 0xb0, 0x3f, 0xf0, 0x00, 0x00, 0x80, 0x04 };
uint8_t Save[] = { 14, 0x00, 0x04, 0x58, 0x08, 0x58, 0x08, 0x58, 0x08, 0x40, 0x08, 0x7f, 0xf8, 0x60, 0x18, 0x5f, 0xe8, 0x5b, 0x68, 0x5f, 0xe8, 0x5b, 0x68, 0x1c, 0xe8, 0x5f, 0xe8, 0x00, 0x00 };
uint8_t Load[] = { 14, 0x0f, 0xc0, 0x1f, 0xe0, 0x3f, 0xf0, 0x7f, 0xb8, 0xff, 0xd8, 0xdc, 0xec, 0xfb, 0x6c, 0xdb, 0x6c, 0xdc, 0xec, 0x6f, 0xfc, 0x77, 0xf8, 0x3f, 0xf0, 0x1f, 0xe0, 0x07, 0xc0 };
uint8_t Lock[] = { 14, 0xe0, 0x1c, 0xc0, 0x0c, 0x8f, 0xc4, 0x9f, 0xe4, 0x9f, 0xe4, 0x9f, 0xe4, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x07, 0x80, 0x07, 0x80, 0x03, 0x00, 0x80, 0x04 };
uint8_t Set19[] = { 14, 0xf8, 0x7c, 0x80, 0x04, 0xbc, 0xf4, 0x3f, 0xf0, 0x38, 0x70, 0xb7, 0xb4, 0xb6, 0xb4, 0xb5, 0xb4, 0xb7, 0xb4, 0x38, 0x70, 0x3f, 0xf0, 0xbc, 0xf4, 0x80, 0x04, 0xf8, 0x7c };
uint8_t IMG_Pen[] = { 14, 0xf0, 0x7c, 0xe0, 0x3c, 0xea, 0xbc, 0xea, 0xbc, 0xea, 0xbc, 0xea, 0xbc, 0xea, 0xbc, 0xea, 0xbc, 0xea, 0xbc, 0xe0, 0x3c, 0xef, 0xbc, 0xf7, 0x7c, 0xf8, 0xfc, 0xfd, 0xfc };
uint8_t IMG_Pen2[] = { 14, 0xe0, 0x3c, 0xf0, 0x7c, 0xf0, 0x7c, 0xf7, 0x7c, 0xf7, 0x7c, 0xef, 0xbc, 0xed, 0xbc, 0xef, 0xbc, 0xf5, 0x7c, 0xf5, 0x7c, 0xfa, 0xfc, 0xfa, 0xfc, 0xfd, 0xfc, 0xfd, 0xfc };
uint8_t IMG_Tip[] = { 14, 0xf3, 0xfc, 0xf1, 0xfc, 0xf4, 0xfc, 0xf6, 0x7c, 0xf7, 0x3c, 0xf7, 0xbc, 0xf7, 0xbc, 0xf7, 0xbc, 0xf7, 0xbc, 0xf7, 0xbc, 0xe7, 0xbc, 0xe0, 0x5c, 0xe5, 0xdc, 0xe5, 0x5c };
uint8_t IMG_Files[] = { 14, 0x00, 0xfc, 0x7e, 0x7c, 0x7e, 0x3c, 0x70, 0x0c, 0x47, 0xe4, 0x77, 0xe0, 0x47, 0xf8, 0x74, 0x08, 0x47, 0xf8, 0x74, 0x08, 0x07, 0xf8, 0xf4, 0x08, 0xf7, 0xf8, 0xf0, 0x00 };
uint8_t IMG_Flip[] = { 14, 0x01, 0x30, 0x7d, 0x38, 0xbd, 0x1c, 0xbd, 0x1c, 0xbd, 0x1c, 0xdd, 0x0c, 0xdd, 0x0c, 0xdd, 0x0c, 0xed, 0x04, 0xed, 0x04, 0xf5, 0x04, 0xf5, 0x00, 0x75, 0x00, 0x39, 0x00 };
uint8_t IMG_Sun[] = { 14, 0xbc, 0xf4, 0x1c, 0xe0, 0x8f, 0xc4, 0xd8, 0x6c, 0xf0, 0x3c, 0xe3, 0x1c, 0x27, 0x90, 0x27, 0x90, 0xe3, 0x1c, 0xf0, 0x3c, 0xd8, 0x6c, 0x8f, 0xc4, 0x1c, 0xe0, 0xbc, 0xf4 };
uint8_t IMG_Size[] = { 14, 0x07, 0xfc, 0x1f, 0xfc, 0x0f, 0xfc, 0x47, 0xfc, 0x63, 0xfc, 0xf1, 0xfc, 0xf8, 0xfc, 0xfc, 0x7c, 0xfe, 0x3c, 0xff, 0x18, 0xff, 0x88, 0xff, 0xc0, 0xff, 0xe0, 0xff, 0x80 };
uint8_t IMG_Animation[] = { 14, 0xe0, 0xfc, 0xdf, 0x7c, 0xbf, 0xbc, 0x63, 0xdc, 0x67, 0xdc, 0x6b, 0xfc, 0x7d, 0x0c, 0x7e, 0xf4, 0xbd, 0x78, 0xdd, 0xb8, 0xe5, 0xd8, 0xfd, 0xf8, 0xfe, 0xf4, 0xff, 0x0c };
uint8_t IMG_Animation_DISABLE[] = { 14, 0xe0, 0xf4, 0xdf, 0x60, 0xbf, 0xc4, 0x63, 0x8c, 0x67, 0x1c, 0x6a, 0x3c, 0x7c, 0x4c, 0x78, 0xf4, 0xb1, 0x78, 0xe3, 0xb8, 0xc5, 0xd8, 0x8d, 0xf8, 0x1e, 0xf4, 0xbf, 0x0c };
uint8_t IMG_Trigger[] = { 14, 0x8f, 0xc4, 0x10, 0x20, 0x2c, 0xd0, 0x4f, 0xc8, 0x9d, 0xe4, 0x3d, 0xf0, 0x1d, 0xe0, 0x3e, 0xf0, 0x3f, 0x70, 0x9f, 0xe4, 0x4f, 0xc8, 0x24, 0xd0, 0x10, 0x20, 0x8f, 0xc4 };
uint8_t IMG_VibrationSwitch[] = { 21, 0xfe, 0x01, 0xf8, 0xfd, 0xfe, 0xf8, 0xfb, 0x83, 0x78, 0xfb, 0x7b, 0x78, 0xfb, 0x07, 0x78, 0xfb, 0x7b, 0x78, 0xfb, 0x83, 0x78, 0xfb, 0x7b, 0x78, 0xfb, 0x07, 0x78, 0xfb, 0x7b, 0x78, 0xfb, 0x83, 0x78, 0xfb, 0x7b, 0x78, 0xfb, 0x07, 0x78, 0xfb, 0x7b, 0x78, 0xfa, 0x01, 0x78, 0xfb, 0x7b, 0x78, 0xfa, 0x01, 0x78, 0xf8, 0x00, 0x78, 0xfa, 0x01, 0x78, 0xff, 0x7b, 0xf8, 0xff, 0x7b, 0xf8 };
uint8_t IMG_ReedSwitch[] = { 21, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xf8, 0xc3, 0xfe, 0x18, 0x80, 0x00, 0x08, 0x87, 0xff, 0x08, 0x0f, 0xff, 0x80, 0x1c, 0x1f, 0xc0, 0x01, 0xfc, 0x00, 0x1f, 0xc1, 0xc0, 0x0f, 0xff, 0x80, 0x07, 0xff, 0x08, 0x80, 0x00, 0x08, 0xc3, 0xfe, 0x18, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xf8 };
uint8_t IMG_BLE[] = { 14, 0xfc, 0x7c, 0xf8, 0x3c, 0x99, 0x1c, 0x89, 0x8c, 0xc1, 0xc4, 0xe1, 0x8c, 0xf0, 0x1c, 0xe0, 0x1c, 0xc1, 0x8c, 0x89, 0xc4, 0x99, 0x8c, 0xf9, 0x1c, 0xf8, 0x3c, 0xfc, 0x7c };
uint8_t IMG_ListMode[] = { 14, 0xff, 0xfc, 0xff, 0xfc, 0x98, 0x04, 0x98, 0x0c, 0xff, 0xfc, 0xff, 0xfc, 0x98, 0x04, 0x98, 0x0c, 0xff, 0xfc, 0xff, 0xfc, 0x98, 0x04, 0x98, 0x0c, 0xff, 0xfc, 0xff, 0xfc };

uint8_t IMG_Load[] = { 0x00, 0x3f, 0xc0, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x03, 0x7f, 0xfc, 0x00, 0x04, 0x7f, 0xfe, 0x00, 0x08, 0x3f, 0xff, 0x00, 0x10, 0x3f, 0xff, 0x80, 0x20, 0x1f, 0xff, 0xc0, 0x20, 0x1f, 0xff, 0xc0, 0x40, 0x0f, 0xff, 0xe0, 0x70, 0x0f, 0xdf, 0xe0, 0xcc, 0x0f, 0xef, 0xf0, 0xf3, 0x0f, 0xef, 0xf0, 0xfc, 0xf9, 0xff, 0xf0, 0xff, 0x10, 0xff, 0xf0, 0xff, 0xf0, 0x8f, 0xf0, 0xff, 0xf9, 0xf3, 0xf0, 0xff, 0x7f, 0x0c, 0xf0, 0xff, 0x7f, 0x03, 0x30, 0x7f, 0xbf, 0x00, 0xe0, 0x7f, 0xff, 0x00, 0x20, 0x3f, 0xff, 0x80, 0x40, 0x3f, 0xff, 0x80, 0x40, 0x1f, 0xff, 0xc0, 0x80, 0x0f, 0xff, 0xc1, 0x00, 0x07, 0xff, 0xe2, 0x00, 0x03, 0xff, 0xec, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x3f, 0xc0, 0x00 };
uint8_t PositioningCursor[] = { 0x18, 0x3c, 0x7e, 0xff, 0xff, 0x7e, 0x3c, 0x18 };
uint8_t Pointer[] = { 0x20, 0x20, 0x70, 0xf8 };

uint8_t Battery_NoPower[] = { 0x07, 0x80, 0x3f, 0xf0, 0x3f, 0xf0, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x37, 0xb0, 0x37, 0xb0, 0x30, 0x30, 0x3f, 0xf0, 0x3f, 0xf0 }; // 欠压警报
uint8_t IMG_BLE_S[] = { 0x0e, 0x00, 0x89, 0x00, 0x48, 0x80, 0x29, 0x00, 0x1a, 0x00, 0x0c, 0x00, 0x1a, 0x00, 0x29, 0x00, 0x48, 0x80, 0x89, 0x00, 0x0e, 0x00 }; // 蓝牙图标

// 状态提示图标
// width:14,height:14
uint8_t c1[] = { 0x03, 0x00, 0x07, 0x80, 0x0f, 0xc0, 0x0c, 0xc0, 0x1c, 0xe0, 0x1c, 0xe0, 0x3c, 0xf0, 0x3c, 0xf0, 0x7f, 0xf8, 0x7f, 0xf8, 0xfc, 0xfc, 0xfc, 0xfc, 0xff, 0xfc, 0x7f, 0xf8 }; // 三角形 感叹号
uint8_t c2[] = { 0x7f, 0xf8, 0xff, 0xfc, 0xc0, 0x0c, 0xc3, 0x0c, 0xcb, 0x4c, 0xdb, 0x6c, 0xdb, 0x6c, 0xd8, 0x6c, 0xdc, 0xec, 0xcf, 0xcc, 0xc7, 0x8c, 0xc0, 0x0c, 0xff, 0xfc, 0x7f, 0xf8 }; // 电源
uint8_t c3[] = { 0x00, 0x00, 0x06, 0x00, 0x0c, 0x00, 0x18, 0xf8, 0x38, 0xf0, 0x30, 0x30, 0x70, 0x60, 0x70, 0x78, 0x78, 0xf8, 0x78, 0x00, 0x3c, 0x02, 0x3f, 0x0c, 0x1f, 0xf8, 0x0f, 0xf0, 0x03, 0xc0, 0x00, 0x00 }; // 月亮睡觉
uint8_t c5[] = { 0x7f, 0xf0, 0xff, 0xe0, 0xc0, 0x08, 0xc0, 0x1c, 0xc0, 0x38, 0xc8, 0x70, 0xdc, 0xe4, 0xcf, 0xcc, 0xc7, 0x8c, 0xc3, 0x0c, 0xc0, 0x0c, 0xc0, 0x0c, 0xff, 0xfc, 0x7f, 0xf8 }; // 单选框打勾
uint8_t c6[] = { 0x1e, 0x10, 0x33, 0x38, 0x2d, 0x7c, 0x25, 0x38, 0x2d, 0x38, 0x25, 0x38, 0x2d, 0x38, 0x6d, 0x80, 0xde, 0xc0, 0xbf, 0x40, 0xbf, 0x40, 0xde, 0xc0, 0x61, 0x80, 0x3f, 0x00 }; // 温度
uint8_t c7[] = { 0x1f, 0xe0, 0x3f, 0xf0, 0x70, 0x38, 0x60, 0x18, 0x60, 0x18, 0x60, 0x18, 0xff, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xf8, 0x7c, 0xf8, 0x7c, 0xfc, 0xfc, 0x7f, 0xf8 }; // 小锁头
uint8_t Lightning[] = { 0x0f, 0xe0, 0x1f, 0xc0, 0x1f, 0xc0, 0x3f, 0x80, 0x3f, 0xf8, 0x7f, 0xf0, 0x7f, 0xe0, 0x07, 0xc0, 0x07, 0x80, 0x0f, 0x00, 0x0e, 0x00, 0x1c, 0x00, 0x18, 0x00, 0x10, 0x00 }; // 闪电

/* 复选框选中 10*10 */
uint8_t CheckBoxSelection[] = { 0xff, 0xc0, 0x80, 0x40, 0x80, 0xc0, 0x81, 0xc0, 0x81, 0xc0, 0x83, 0x40, 0x9b, 0x40, 0x8e, 0x40, 0x86, 0x40, 0xff, 0xc0 };

uint8_t pidImg[] = { 14, 0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc, 0x08, 0x84, 0x6d, 0xb0, 0x6d, 0xb8, 0x6d, 0xb8, 0x0d, 0xb8, 0x7d, 0xb8, 0x7d, 0xb0, 0x78, 0x84, 0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc }; // PID文字图标
uint8_t kalmanImg[] = { 14, 0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc, 0x03, 0x3c, 0xf3, 0x3c, 0xf2, 0x7c, 0xf4, 0xfc, 0xf0, 0xfc, 0xf2, 0x7c, 0xf3, 0x3c, 0xf3, 0x3c, 0xff, 0x80, 0xff, 0xfc, 0xff, 0xfc }; // kalman图标