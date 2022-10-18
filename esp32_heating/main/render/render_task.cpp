#include "heating.h"
#include "ExternDraw.h"

static uint8_t curRenderMenuLevelID = 0; // 当前显示的菜单ID
static uint8_t LastMenuLevelId = 0; // 之前的菜单索引ID(跳转前的)
static uint8_t Menu_JumpAndExit = false; // 该标志位适用于快速打开菜单设置，当返回时立刻退出菜单 回到主界面
uint8_t Menu_JumpAndExit_Level = 255; //当跳转完成后 的 菜单层级 等于“跳转即退出层级”时，“跳转即退出”立马生效
uint8_t Menu_System_State = 1; // 菜单状态 0=显示主界面  1=当前显示菜单

/* 复选框选中 10*10 */
uint8_t CheckBoxSelection[] = { 0xff, 0xc0, 0x80, 0x40, 0x80, 0xc0, 0x81, 0xc0, 0x81, 0xc0, 0x83, 0x40, 0x9b, 0x40, 0x8e, 0x40, 0x86, 0x40, 0xff, 0xc0 };

// 判断是否文本渲染模式
// 若当前菜单层级没有图标化则使用普通文本菜单的模式进行渲染显示
// 若屏幕分辨率高度小于32 则强制启用文本菜单模式
#define ISTEXTRENDER(type) (MenuRenderText == type || MenuListMode || OLED_SCREEN_HEIGHT <= 32)

#define SCROLLBARWIDTH 3 // 文本菜单模式下 滚动条宽度

static void RunMenuControls(uint8_t lid, uint8_t id);

// 退出菜单系统
void Save_Exit_Menu_System(void)
{
    // 过渡离开
    u8g2_SetDrawColor(&u8g2, 0);
    Blur(0, 0, OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, 4, 66 * *SwitchControls[SwitchSpace_SmoothAnimation]);
    u8g2_SetDrawColor(&u8g2, 1);

    //保存配置
    // SYS_Save();

    // Exit_Menu_System();
}

/*
    @brief 获取当前菜单的渲染类型
    @param uint8_t id 菜单层对象id
    @return 菜单层对象 的索引值
*/
int GetRealMenuLevelId(int id)
{
    int idx = 0;
    for (int i = 0; i < sizeMenuLevel; i++) {
        if (MenuLevel[i].id == id) {
            idx = i;
        }
    }
    return idx;
}

/**
 * @brief 根据菜单lid 和 id 来获取当前Menu的索引位置
 *
 * @param lid
 * @param id
 * @return int Menu结构体中的索引
 */
int GetMenuId(int lid, int id)
{
    int idx = 0;
    for (int i = 0; i < sizeMenu; i++) {
        if (Menu[i].lid == lid && Menu[i].id == id) {
            idx = i;
            break;
        }
    }
    return idx;
}

//按照标题进行跳转 标题跳转 跳转标题
void JumpWithTitle(void)
{
    RunMenuControls(curRenderMenuLevelID, 0);
}

/**
 * @brief 过渡动画计算
 *
 */
static void MenuSmoothAnimation_System()
{
    for (uint8_t i = 0; i < sizeMenuSmoothAnimatioin; i++) {
        // 优化计算：变形过滤器
        if (menuSmoothAnimation[i].result && abs(menuSmoothAnimation[i].result * 100) < 1.5)
            menuSmoothAnimation[i].result = 0;

        // 动画控制变量是否需要更新
        if (menuSmoothAnimation[i].last != menuSmoothAnimation[i].val) {
            // 是否允许累加
            if (menuSmoothAnimation[i].isTotalization) {
                menuSmoothAnimation[i].result += menuSmoothAnimation[i].val - menuSmoothAnimation[i].last; // 累加计算
            } else {
                menuSmoothAnimation[i].result = menuSmoothAnimation[i].val - menuSmoothAnimation[i].last;
            }

            // 重置标志
            menuSmoothAnimation[i].last = menuSmoothAnimation[i].val;
        }

        // 使用被选的动画计算函数计算动画
        menuSmoothAnimation[i].result -= menuSmoothAnimation[i].result * menuSmoothAnimation[i].SlidingWeights;
    }
}

/**
 * @brief 在菜单中 更新系统编码器位置
 *
 */
static void updateRenderMenuEncoderPosition()
{
    if (!Menu_System_State)
        return;

    int32_t real_Level_Id = GetRealMenuLevelId(curRenderMenuLevelID);
    MenuLevelSystem* pMenuLevel = &MenuLevel[real_Level_Id];

    if (ISTEXTRENDER(pMenuLevel->RenderType)) {
        // 文本渲染

        // 设置编码器滚动范围
        pMenuLevel->min = 0; // 重置选项最小值：从图标模式切换到列表模式会改变该值

        uint8_t MinimumScrolling = min((int)SlideControls[Slide_space_Scroll].max, (int)pMenuLevel->max);
        RotarySet((int)SlideControls[Slide_space_Scroll].min, MinimumScrolling + 1, 1, (int)*SlideControls[Slide_space_Scroll].val + (1)); //+(1) 是因为实际上计算会-1 ,这里要补回来

    } else {
        // 图标渲染
        if (Menu[GetMenuId(real_Level_Id, 0)].type) {
            pMenuLevel->min = 1; //当前处在图标模式 如果目标层菜单的第一项为标题，则给予屏蔽
        }

        RotarySet(pMenuLevel->min, pMenuLevel->max, 1, pMenuLevel->index);
        *SlideControls[Slide_space_Scroll].val = 0;
    }
}

/*
    @函数 Next_Menu
    @brief 多级菜单跳转初始化参数
*/
static void Next_Menu(void)
{
    // 设置菜单标志位
    Menu_System_State = 1;

    // 设置编码器
    updateRenderMenuEncoderPosition();

    if (*SwitchControls[SwitchSpace_SmoothAnimation]) {
        if (LastMenuLevelId != curRenderMenuLevelID) {
            u8g2_SetDrawColor(&u8g2, 0);
            Blur(0, 0, OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, 4, 20 * *SwitchControls[SwitchSpace_SmoothAnimation]);
            u8g2_SetDrawColor(&u8g2, 1);
        }

        // 项目归位动画
        menuSmoothAnimation[3].last = 0;
        menuSmoothAnimation[3].val = 1;
    }
}

/**
 * @brief 跳转到新菜单
 *
 * @param lid 跳转前的MenuLevel ID
 * @param id 跳转前的菜单ID
 */
static void RunMenuControls(uint8_t lid, uint8_t id)
{
    printf("运行菜单控件 %d %d\n", lid, id);

    // 根据lid 和 id 找到当前Menu中的Index
    int Id = GetMenuId(lid, id);

    switch (Menu[Id].type) {
    case Type_GotoMenu: // 跳转到菜单
    case Type_MenuName: // 菜单名
        LastMenuLevelId = curRenderMenuLevelID; // 决定是否播放转场动画
        curRenderMenuLevelID = Menu[Id].ParamA; // 赋值新菜单ID
        printf("当前Level ID=%d\r\n", curRenderMenuLevelID);

        if (ISTEXTRENDER(MenuLevel[curRenderMenuLevelID].RenderType)) {
            // 使用文本渲染模式

            // 如果当前菜单层没有开启了图表化显示则对子菜单选项定向跳转执行配置
            uint8_t ExcellentLimit = MenuLevel[curRenderMenuLevelID].max + 1 - SCREEN_FONT_ROW; // 是为了从1开始计算
            uint8_t ExcellentMedian = SCREEN_FONT_ROW / 2; // 注意：这里从1开始计数

            // 计算最优显示区域
            if (Menu[Id].ParamB == 0) {
                // 头只有最差显示区域
                MenuLevel[curRenderMenuLevelID].index = 0;
                *SlideControls[Slide_space_Scroll].val = 0;

            } else if (Menu[Id].ParamB > 0 && Menu[Id].ParamB <= MenuLevel[curRenderMenuLevelID].max - ExcellentMedian) {
                // 中部拥有绝佳的显示区域
                MenuLevel[curRenderMenuLevelID].index = Menu[Id].ParamB - 1;
                *SlideControls[Slide_space_Scroll].val = 1;

            } else {
                // 靠后位置 以及 最差的尾部
                MenuLevel[curRenderMenuLevelID].index = ExcellentLimit;
                *SlideControls[Slide_space_Scroll].val = Menu[Id].ParamB - ExcellentLimit;
            }

        } else {
            // 使用图标渲染
            MenuLevel[curRenderMenuLevelID].index = Menu[Id].ParamB; // 打开新菜单后。默认图标位置
        }

        // 按需求跳转完成后执行函数
        if (Menu[Id].function) {
            Menu[Id].function();
        }

        // 检查“跳转即退出”标志
        if (Menu_JumpAndExit && curRenderMenuLevelID == Menu_JumpAndExit_Level) {
            Save_Exit_Menu_System();
        }

        // 再次确认菜单状态
        if (Menu_System_State) {
            Next_Menu(); // 由于执行函数可能会导致菜单状态被更改，所以这里需要确定菜单状态
        }
        break;

    case Type_RunFunction:
        // 运行函数
        if (Menu[Id].function) {
            Menu[Id].function();
        }
        updateRenderMenuEncoderPosition();
        break;

    case Type_Switch:
        // 开关控件
        *SwitchControls[Menu[Id].ParamA] = !*SwitchControls[Menu[Id].ParamA];
        if (Menu[Id].function) {
            Menu[Id].function();
        }
        break;

    case Type_CheckBox:
        //单选模式
        *SwitchControls[Menu[Id].ParamA] = Menu[Id].ParamB;
        if (Menu[Id].function) {
            Menu[Id].function();
        }
        break;

    case Type_Slider:
        // 滑动条

        // 设置编码器
        RotarySet(SlideControls[Menu[Id].ParamA].min, SlideControls[Menu[Id].ParamA].max, SlideControls[Menu[Id].ParamA].step, *SlideControls[Menu[Id].ParamA].val);

        // 背景变淡
        u8g2_SetDrawColor(&u8g2, 0);
        Blur(0, 0, OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, 3, 11 * *SwitchControls[SwitchSpace_SmoothAnimation]);
        u8g2_SetDrawColor(&u8g2, 1);

        // 绘制滑动条
        while (BUTTON_NULL == getRotaryButton()) {
            u8g2_SetDrawColor(&u8g2, 0);
            u8g2_DrawBox(&u8g2, OLED_SCREEN_WIDTH / 8 - 2, (OLED_SCREEN_HEIGHT - 24) / 2 - 3, 3 * OLED_SCREEN_WIDTH / 4 + 4, 24 + 4);
            u8g2_SetDrawColor(&u8g2, 1);
            u8g2_DrawRFrame(&u8g2, OLED_SCREEN_WIDTH / 8 - 3, (OLED_SCREEN_HEIGHT - 24) / 2 - 4, 3 * OLED_SCREEN_WIDTH / 4 + 4, 24 + 6, 2);

            *SlideControls[Menu[Id].ParamA].val = GetRotaryPositon();

            // 绘制数字
            u8g2_DrawUTF8(&u8g2, OLED_SCREEN_WIDTH / 8, (OLED_SCREEN_HEIGHT - 24) / 2 + 1, Menu[Id].name);

            // 绘制滑动条
            Draw_Num_Bar(*SlideControls[Menu[Id].ParamA].val, SlideControls[Menu[Id].ParamA].min, SlideControls[Menu[Id].ParamA].max, OLED_SCREEN_WIDTH / 8, (OLED_SCREEN_HEIGHT - 24) / 2 + CNSize + 3, 3 * OLED_SCREEN_WIDTH / 4, 7, 1);

            Display();

            //当前滑动条为屏幕亮度调节 需要特殊设置对屏幕亮度进行实时预览
            if (Menu[Id].function) {
                Menu[Id].function();
            }

            delay(10);
        }

        if (Menu[Id].function) {
            Menu[Id].function();
        }
        updateRenderMenuEncoderPosition();
        break;

    case Type_NULL:
    default:
        break;
    }
}

/**
 * @brief 渲染主菜单
 *
 */
static void RenderMenu(void)
{
    if (!Menu_System_State)
        return;

    // 清空BUf
    ClearOLEDBuffer();

    // 计算过渡动画
    if (true == *SwitchControls[SwitchSpace_SmoothAnimation]) {
        MenuSmoothAnimation_System();
    }

    SlideBar* pSlideSpace = &SlideControls[Slide_space_Scroll];

    // 分别获取 菜单层、菜单项 索引值
    int32_t real_Level_Id = GetRealMenuLevelId(curRenderMenuLevelID);

    if (ISTEXTRENDER(MenuLevel[real_Level_Id].RenderType)) {
        // 文本渲染模式

        int Pos_Id = GetMenuId(MenuLevel[real_Level_Id].id, MenuLevel[real_Level_Id].index + (int)*pSlideSpace->val); // 得到当前显示位置

        // 显示菜单项目名::这里有两行文字是在屏幕外 用于动过渡动画  所以-1~5共循环6次
        for (int i = -1; i < SCREEN_PAGE_NUM / 2 + 1; i++) {
            if (MenuLevel[real_Level_Id].index + i >= 0 && MenuLevel[real_Level_Id].index + i <= MenuLevel[real_Level_Id].max) {
                // 获得当前绘制的菜单内容
                MenuSystem* pItemMenu = &Menu[GetMenuId(real_Level_Id, MenuLevel[real_Level_Id].index + i)];

                // 绘制目录树 最左边的字符 + 或 -
                if (pItemMenu->type != Type_MenuName) {
                    u8g2_DrawUTF8(&u8g2, 0, (1 - menuSmoothAnimation[3].result * (i != -1)) * ((i + menuSmoothAnimation[0].result) * 16 + 1), pItemMenu->type == Type_GotoMenu ? "+" : "-");
                }

                // 绘制目录名
                u8g2_DrawUTF8(&u8g2, 7 * (pItemMenu->type != Type_MenuName), (1 - menuSmoothAnimation[3].result * (i != -1)) * ((i + menuSmoothAnimation[0].result) * 16 + 1) + 1, pItemMenu->name);

                // 对菜单控件分类渲染
                switch (pItemMenu->type) {
                case Type_Switch:
                    // 开关控件
                    u8g2_DrawUTF8(&u8g2, OLED_SCREEN_WIDTH - 32 - 1, (i + menuSmoothAnimation[0].result) * 16 + 2, *SwitchControls[pItemMenu->ParamA] ? (char*)"开启" : (char*)"关闭");
                    break;

                case Type_Slider:
                    // 弹出滑动条
                    char buffer[20];
                    sprintf(buffer, "%.2f", *SlideControls[pItemMenu->ParamA].val);
                    u8g2_DrawUTF8(&u8g2, OLED_SCREEN_WIDTH - 9 - u8g2_GetUTF8Width(&u8g2, buffer), (int)((i + menuSmoothAnimation[0].result) * 16) + 1, buffer);
                    break;

                case Type_CheckBox:
                    // 单选框
                    if ((*SwitchControls[pItemMenu->ParamA] == pItemMenu->ParamB)) {
                        Draw_Slow_Bitmap(OLED_SCREEN_WIDTH - 32 - 1 + 15, (i + menuSmoothAnimation[0].result) * 16 + 2, CheckBoxSelection, 10, 10);
                    } else {
                        u8g2_DrawFrame(&u8g2, OLED_SCREEN_WIDTH - 32 - 1 + 15, (i + menuSmoothAnimation[0].result) * 16 + 2, 10, 10);
                    }

                    // 当前项高亮
                    if ((int)*pSlideSpace->val == i) {
                        u8g2_SetDrawColor(&u8g2, 2);
                        u8g2_DrawBox(&u8g2, OLED_SCREEN_WIDTH - 32 - 2 + 15, (i + menuSmoothAnimation[0].result) * 16 + 1, 12, 12);
                        u8g2_SetDrawColor(&u8g2, 1);
                    }
                    break;

                default:
                    break;
                }
            }
        }

        int32_t iCurMenuItemIndex = MenuLevel[real_Level_Id].index + *pSlideSpace->val; // 得到当前指向第几个菜单 (页数 + 编码器位置)

        // 绘制右边的滚动条
        DrawScrollBar(OLED_SCREEN_WIDTH - SCROLLBARWIDTH, 0, SCROLLBARWIDTH, OLED_SCREEN_HEIGHT - 1,
            MenuLevel[real_Level_Id].max + 1,
            map(iCurMenuItemIndex, 0, MenuLevel[real_Level_Id].max + 1, -menuSmoothAnimation[1].result * (OLED_SCREEN_HEIGHT / (MenuLevel[real_Level_Id].max + 1)), OLED_SCREEN_HEIGHT - 1));

        // 显示右下角页码角标
        DrawPageFootnotes(iCurMenuItemIndex + 1, MenuLevel[real_Level_Id].max + 1);

        // 选中反色高亮被选项
        u8g2_SetDrawColor(&u8g2, 2);
        u8g2_DrawRBox(&u8g2, 0, ((int)*pSlideSpace->val - menuSmoothAnimation[1].result) * 16, *SwitchControls[SwitchSpace_OptionStripFixedLength] ? 123 : (u8g2_GetUTF8Width(&u8g2, Menu[Pos_Id].name) - menuSmoothAnimation[2].result + 12 * (Menu[Pos_Id].type != Type_MenuName) + 1), CNSize + 2, 0);
        u8g2_SetDrawColor(&u8g2, 1);

        // 刷新编码器位置
        *pSlideSpace->val = GetRotaryPositon() - 1.0f;
        if (*pSlideSpace->val >= pSlideSpace->max) {
            // 显示下一页
            MenuLevel[real_Level_Id].index++; // 页数+1
            *pSlideSpace->val = pSlideSpace->max - 1.0f;
            RotarySetPositon(pSlideSpace->max);

        } else if (*pSlideSpace->val <= -1.0f) {
            // 显示上一页
            MenuLevel[real_Level_Id].index--; // 页数-1
            *pSlideSpace->val = 0.0f;
            delay(50);
            RotarySetPositon(1);
        }

        // 翻页
        MenuLevel[real_Level_Id].index = constrain(MenuLevel[real_Level_Id].index, MenuLevel[real_Level_Id].min, (MenuLevel[real_Level_Id].max > (uint8_t)(pSlideSpace->max - 1.0f)) ? (MenuLevel[real_Level_Id].max - ((uint8_t)pSlideSpace->max - 1.0f)) : 0);

        // 更新过渡动画
        // real_Level_Id = GetRealMenuLevelId(curRenderMenuLevelID);
        Pos_Id = GetMenuId(MenuLevel[real_Level_Id].id, MenuLevel[real_Level_Id].index + (int)*pSlideSpace->val);

        // 计算滑动动画
        menuSmoothAnimation[0].val = MenuLevel[real_Level_Id].index;
        menuSmoothAnimation[1].val = MenuLevel[real_Level_Id].index + (int)*pSlideSpace->val;
        menuSmoothAnimation[2].val = u8g2_GetUTF8Width(&u8g2, Menu[Pos_Id].name);

    } else {
        // 图标渲染模式

        int id = GetMenuId(MenuLevel[real_Level_Id].id, MenuLevel[real_Level_Id].index);
        int Pos_Id;

        // 居中显示项目名
        u8g2_DrawUTF8(&u8g2, UTF8_HMiddle(0, 128, 1, Menu[id].name), 50 + 1, Menu[id].name);

        for (int8_t i = 0; i < 5; i++) {
            Pos_Id = GetMenuId(MenuLevel[real_Level_Id].id, MenuLevel[real_Level_Id].index + i - 2);

            if (MenuLevel[real_Level_Id].index - 2 + i >= 0 && MenuLevel[real_Level_Id].index - 2 + i <= MenuLevel[real_Level_Id].max) {
                // 绘制菜单项目图标
                if (Menu[id].type != Type_MenuName) {
                    if (Menu[Pos_Id].type != Type_MenuName) {
                        Draw_APP((1 - menuSmoothAnimation[3].result * (i != -1)) * (-69 + i * 56 + menuSmoothAnimation[0].result * 56), 3, Menu[Pos_Id].icon);
                    }
                }
            }
        }

        MenuLevel[real_Level_Id].index = GetRotaryPositon();
        menuSmoothAnimation[0].val = MenuLevel[real_Level_Id].index;
    }

    // 等待编码器按钮按下
    ROTARY_BUTTON_TYPE rotaryButton = getRotaryButton();
    switch (rotaryButton) {
    case BUTTON_CLICK:
    case BUTTON_DOUBLECLICK:
        // 单击 双击 执行项目
        RunMenuControls(MenuLevel[real_Level_Id].id, MenuLevel[real_Level_Id].index + *pSlideSpace->val);
        break;

    case BUTTON_LONGCLICK:
        // 长按 返回上一个菜单
        RunMenuControls(MenuLevel[real_Level_Id].id, 0);
        break;

    default:
        break;
    }

    Display();
}

void render_task(void* arg)
{
    while (1) {
        RenderMenu();
        u8g2_SendBuffer(&u8g2);
    }
}
