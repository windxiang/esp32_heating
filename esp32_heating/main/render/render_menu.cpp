#include "heating.h"
#include "ExternDraw.h"
#include "bitmap.h"

// static const char* TAG = "render_task";

struct MenuControlStruct {
    uint8_t curRenderMenuLevelID; // 当前显示的菜单ID
    uint8_t LastMenuLevelId; // 之前的菜单索引ID(跳转前的)
    uint8_t Menu_JumpAndExit; // 该标志位适用于快速打开菜单设置，当返回时立刻退出菜单 回到主界面
    uint8_t Menu_JumpAndExit_Level; // 当跳转完成后 的 菜单层级 等于“跳转即退出层级”时，“跳转即退出”立马生效
};

static MenuControlStruct MenuControl = {
    curRenderMenuLevelID : 0,
    LastMenuLevelId : 0,
    Menu_JumpAndExit : 0,
    Menu_JumpAndExit_Level : 255,
};

// 判断是否文本渲染模式
// 若当前菜单层级没有图标化则使用普通文本菜单的模式进行渲染显示
// 若屏幕分辨率高度小于32 则强制启用文本菜单模式
#define ISTEXTRENDER(type) (MenuRenderText == type || SystemMenuSaveData.MenuListMode || OLED_SCREEN_HEIGHT <= 32)

#define SCROLLBARWIDTH 3 // 文本菜单模式下 滚动条宽度

static void RunMenuControls(uint8_t menuID, uint8_t subMenuIndex);

// 退出菜单系统
void ExitMenuSystem(void)
{
    // 过渡离开
    u8g2_SetDrawColor(&u8g2, 0);
    Blur(0, 0, OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, 4, 66 * *SwitchControls[SwitchSpace_SmoothAnimation]);
    u8g2_SetDrawColor(&u8g2, 1);

    // 保存配置
    settings_write_all();

    // 退出菜单
    exitMenu();

    printf("退出菜单\r\n");
}

/**
 * @brief 跳转到上一层菜单
 *
 */
void JumpWithTitle(void)
{
    RunMenuControls(MenuControl.curRenderMenuLevelID, 0);
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
    menuSystem* pMenuRoot = getCurRenderMenu(MenuControl.curRenderMenuLevelID);

    if (ISTEXTRENDER(pMenuRoot->RenderType)) {
        // 文本渲染
        // 设置编码器滚动范围
        pMenuRoot->minMenuSize = 0; // 重置选项最小值：从图标模式切换到列表模式会改变该值

        uint8_t MinimumScrolling = min((int)SlideControls[Slide_space_Scroll].max, pMenuRoot->maxMenuSize - 1);
        RotarySet((int)SlideControls[Slide_space_Scroll].min, MinimumScrolling + 1, 1, (int)*SlideControls[Slide_space_Scroll].val + (1)); //+(1) 是因为实际上计算会-1 ,这里要补回来
    } else {
        // 图标渲染
        subMenu* pSubMenu = getSubMenu(pMenuRoot, 0);
        if (Type_MenuName == pSubMenu->type) {
            // 当前处在图标模式 如果目标层菜单的第一项为标题，则给予屏蔽
            pMenuRoot->minMenuSize = 1;
        }

        RotarySet(pMenuRoot->minMenuSize, pMenuRoot->maxMenuSize - 1, 1, pMenuRoot->index);
        *SlideControls[Slide_space_Scroll].val = 0;
    }
}

/*
    @函数 Next_Menu
    @brief 多级菜单跳转初始化参数
*/
static void Next_Menu(void)
{
    // 设置编码器
    updateRenderMenuEncoderPosition();

    // 转场动画
    if (*SwitchControls[SwitchSpace_SmoothAnimation]) {
        if (MenuControl.LastMenuLevelId != MenuControl.curRenderMenuLevelID) {
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
 * @param menuID 跳转前的菜单ID
 * @param subMenuIndex 跳转前的子菜单
 */
static void RunMenuControls(uint8_t menuID, uint8_t subMenuIndex)
{
    // 得到当前正在显示的菜单信息
    menuSystem* pMenuRoot = getCurRenderMenu(MenuControl.curRenderMenuLevelID);
    subMenu* pSubMenu = getSubMenu(pMenuRoot, subMenuIndex);

    switch (pSubMenu->type) {
    case Type_ReturnMenu: // 返回上一级菜单
    case Type_MenuName: // 菜单名
    case Type_GotoMenu: // 跳转到菜单
    {
        MenuControl.LastMenuLevelId = MenuControl.curRenderMenuLevelID; // 决定是否播放转场动画
        MenuControl.curRenderMenuLevelID = pSubMenu->ParamA; // 赋值新菜单ID

        // 得到目标菜单
        menuSystem* pNextMenuRoot = getCurRenderMenu(MenuControl.curRenderMenuLevelID);

        if (ISTEXTRENDER(pNextMenuRoot->RenderType)) {
            // 使用文本渲染模式

            // 如果当前菜单层没有开启了图表化显示则对子菜单选项定向跳转执行配置
            uint8_t ExcellentLimit = pNextMenuRoot->maxMenuSize - SCREEN_FONT_ROW; // 是为了从1开始计算
            uint8_t ExcellentMedian = SCREEN_FONT_ROW / 2; // 注意：这里从1开始计数

            // 计算最优显示区域
            if (pSubMenu->ParamB == 0) {
                // 头只有最差显示区域
                pNextMenuRoot->index = 0;
                *SlideControls[Slide_space_Scroll].val = 0;

            } else if (pSubMenu->ParamB > 0 && pSubMenu->ParamB <= pNextMenuRoot->maxMenuSize - 1 - ExcellentMedian) {
                // 中部拥有绝佳的显示区域
                pNextMenuRoot->index = pSubMenu->ParamB - 1;
                *SlideControls[Slide_space_Scroll].val = 1;

            } else {
                // 靠后位置 以及 最差的尾部
                pNextMenuRoot->index = ExcellentLimit;
                *SlideControls[Slide_space_Scroll].val = pSubMenu->ParamB - ExcellentLimit;
            }

        } else {
            // 使用图标渲染
            subMenu* pNextSubMenu = getSubMenu(pNextMenuRoot, 1);
            if (pNextSubMenu != NULL && Type_CheckBox == pNextSubMenu->type) {
                // 如果是 Type_CheckBox 控件
                if ((*SwitchControls[pNextSubMenu->ParamA] + 1) <= (pNextMenuRoot->maxMenuSize - 1)) {
                    pNextMenuRoot->index = *SwitchControls[pNextSubMenu->ParamA] + 1;
                } else {
                    pNextMenuRoot->index = 0;
                }

            } else {
                pNextMenuRoot->index = pSubMenu->ParamB; // 打开新菜单后。默认图标位置
            }
        }

        // 按需求跳转完成后执行函数
        if (pSubMenu->function) {
            pSubMenu->function();
        }

        // 检查“跳转即退出”标志
        if (MenuControl.Menu_JumpAndExit && MenuControl.curRenderMenuLevelID == MenuControl.Menu_JumpAndExit_Level) {
            ExitMenuSystem();
        }

        Next_Menu();

    } break;

    case Type_RunFunction:
        // 运行函数
        if (pSubMenu->function) {
            pSubMenu->function();
        }
        updateRenderMenuEncoderPosition();
        break;

    case Type_Switch:
        // 开关控件
        *SwitchControls[pSubMenu->ParamA] = !*SwitchControls[pSubMenu->ParamA];
        if (pSubMenu->function) {
            pSubMenu->function();
        }
        updateRenderMenuEncoderPosition();
        break;

    case Type_CheckBox:
        // 单选模式
        *SwitchControls[pSubMenu->ParamA] = pSubMenu->ParamB;
        if (pSubMenu->function) {
            pSubMenu->function();
        }
        break;

    case Type_Slider:
        // 滑动条

        // 设置编码器
        RotarySet(SlideControls[pSubMenu->ParamA].min, SlideControls[pSubMenu->ParamA].max, SlideControls[pSubMenu->ParamA].step, *SlideControls[pSubMenu->ParamA].val);

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

            *SlideControls[pSubMenu->ParamA].val = GetRotaryPositon();

            // 绘制数字
            u8g2_DrawUTF8(&u8g2, OLED_SCREEN_WIDTH / 8, (OLED_SCREEN_HEIGHT - 24) / 2 + 1, pSubMenu->name.c_str());

            // 绘制滑动条
            Draw_Num_Bar(*SlideControls[pSubMenu->ParamA].val, SlideControls[pSubMenu->ParamA].min, SlideControls[pSubMenu->ParamA].max, OLED_SCREEN_WIDTH / 8, (OLED_SCREEN_HEIGHT - 24) / 2 + CNSize + 3, 3 * OLED_SCREEN_WIDTH / 4, 7, 1);

            Display();

            //当前滑动条为屏幕亮度调节 需要特殊设置对屏幕亮度进行实时预览
            if (pSubMenu->function) {
                pSubMenu->function();
            }

            delay(10);
        }

        if (pSubMenu->function) {
            pSubMenu->function();
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
void RenderMenu(void)
{
    // 清空BUf
    ClearOLEDBuffer();

    // 计算过渡动画
    if (true == *SwitchControls[SwitchSpace_SmoothAnimation]) {
        MenuSmoothAnimation_System();
    }

    const SlideBar* pSlideSpace = &SlideControls[Slide_space_Scroll];

    // 分别获取 菜单层、菜单项 索引值
    menuSystem* pMenuRoot = getCurRenderMenu(MenuControl.curRenderMenuLevelID);

    if (ISTEXTRENDER(pMenuRoot->RenderType)) {
        // 文本渲染模式

        // 显示菜单项目名::这里有两行文字是在屏幕外 用于动过渡动画  所以-1~5共循环6次
        for (int i = -1; i < SCREEN_PAGE_NUM / 2 + 1; i++) {
            if (pMenuRoot->index + i >= 0 && pMenuRoot->index + i <= (pMenuRoot->maxMenuSize - 1)) {
                // 获得当前绘制的菜单内容
                subMenu* pSubMenu = getSubMenu(pMenuRoot, pMenuRoot->index + i);

                // 绘制目录树 最左边的字符 + 或 -
                if (pSubMenu->type != Type_MenuName) {
                    u8g2_DrawUTF8(&u8g2, 0, (1 - menuSmoothAnimation[3].result * (i != -1)) * ((i + menuSmoothAnimation[0].result) * 16 + 1), pSubMenu->type == Type_GotoMenu ? "+" : "-");
                }

                // 绘制目录名
                u8g2_DrawUTF8(&u8g2, 7 * (pSubMenu->type != Type_MenuName), (1 - menuSmoothAnimation[3].result * (i != -1)) * ((i + menuSmoothAnimation[0].result) * 16 + 1) + 1, pSubMenu->name.c_str());

                // 对菜单控件分类渲染
                switch (pSubMenu->type) {
                case Type_Switch:
                    // 开关控件
                    u8g2_DrawUTF8(&u8g2, OLED_SCREEN_WIDTH - 32 - 1, (i + menuSmoothAnimation[0].result) * 16 + 2, *SwitchControls[pSubMenu->ParamA] ? "开启" : "关闭");
                    break;

                case Type_Slider:
                    // 弹出滑动条
                    char buffer[20];
                    sprintf(buffer, "%.2f", *SlideControls[pSubMenu->ParamA].val);
                    u8g2_DrawUTF8(&u8g2, OLED_SCREEN_WIDTH - 9 - u8g2_GetUTF8Width(&u8g2, buffer), (int)((i + menuSmoothAnimation[0].result) * 16) + 1, buffer);
                    break;

                case Type_CheckBox:
                    // 单选框
                    if ((*SwitchControls[pSubMenu->ParamA] == pSubMenu->ParamB)) {
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

        int32_t iCurMenuItemIndex = pMenuRoot->index + *pSlideSpace->val; // 得到当前指向第几个菜单 (页数 + 编码器位置)

        // 绘制右边的滚动条
        DrawScrollBar(OLED_SCREEN_WIDTH - SCROLLBARWIDTH, 0, SCROLLBARWIDTH, OLED_SCREEN_HEIGHT - 1,
            pMenuRoot->maxMenuSize,
            map(iCurMenuItemIndex, 0, pMenuRoot->maxMenuSize, -menuSmoothAnimation[1].result * (OLED_SCREEN_HEIGHT / pMenuRoot->maxMenuSize), OLED_SCREEN_HEIGHT - 1));

        // 显示右下角页码角标
        DrawPageFootnotes(iCurMenuItemIndex + 1, pMenuRoot->maxMenuSize);

        // 选中反色高亮被选项
        subMenu* pSubMenu = getSubMenu(pMenuRoot, iCurMenuItemIndex); // 得到当前显示的菜单
        u8g2_SetDrawColor(&u8g2, 2);
        u8g2_DrawRBox(&u8g2, 0, ((int)*pSlideSpace->val - menuSmoothAnimation[1].result) * 16, *SwitchControls[SwitchSpace_OptionWidth] ? 123 : (u8g2_GetUTF8Width(&u8g2, pSubMenu->name.c_str()) - menuSmoothAnimation[2].result + 12 * (pSubMenu->type != Type_MenuName) + 1), CNSize + 2, 0);
        u8g2_SetDrawColor(&u8g2, 1);

        // 刷新编码器位置
        *pSlideSpace->val = GetRotaryPositon() - 1.0f;
        if (*pSlideSpace->val >= pSlideSpace->max) {
            // 显示下一页
            pMenuRoot->index++; // 页数+1
            *pSlideSpace->val = pSlideSpace->max - 1.0f;
            RotarySetPositon(pSlideSpace->max);

        } else if (*pSlideSpace->val <= -1.0f) {
            // 显示上一页
            pMenuRoot->index--; // 页数-1
            *pSlideSpace->val = 0.0f;
            RotarySetPositon(1);
        }

        // 翻页
        pMenuRoot->index = constrain(pMenuRoot->index, pMenuRoot->minMenuSize, ((pMenuRoot->maxMenuSize - 1) > (uint8_t)(pSlideSpace->max - 1.0f)) ? ((pMenuRoot->maxMenuSize - 1) - ((uint8_t)pSlideSpace->max - 1.0f)) : 0);

        // 得到新的菜单
        subMenu* pNextItemMenu = getSubMenu(pMenuRoot, pMenuRoot->index + *pSlideSpace->val);

        // 计算滑动动画
        menuSmoothAnimation[0].val = pMenuRoot->index;
        menuSmoothAnimation[1].val = pMenuRoot->index + (int)*pSlideSpace->val;
        menuSmoothAnimation[2].val = u8g2_GetUTF8Width(&u8g2, pNextItemMenu->name.c_str());

    } else {
        // 图标渲染模式

        // 居中显示菜单名
        subMenu* pSubMenu = getSubMenu(pMenuRoot, pMenuRoot->index);
        if (NULL != pSubMenu) {
            u8g2_DrawUTF8(&u8g2, UTF8_HMiddle(0, OLED_SCREEN_WIDTH, 1, pSubMenu->name.c_str()), 50 + 1, pSubMenu->name.c_str());
        }

        // 显示两边 和 当前的图标
        for (int8_t i = 0; i < 5; i++) {
            int8_t offset = pMenuRoot->index + i - 2;
            if (offset >= 0 && offset <= (pMenuRoot->maxMenuSize - 1)) {
                subMenu* pSubMenu = getSubMenu(pMenuRoot, offset);
                if (pSubMenu && pSubMenu->type != Type_MenuName) {
                    Draw_APP((1 - menuSmoothAnimation[3].result * (i != -1)) * (-69 + i * 56 + menuSmoothAnimation[0].result * 56), 3, pSubMenu->icon); // 绘制菜单项目图标
                }
            }
        }

        // 更新位置
        pMenuRoot->index = (int8_t)GetRotaryPositon();

        // 更新动画
        menuSmoothAnimation[0].val = pMenuRoot->index;
    }

    // 等待编码器按钮按下
    ROTARY_BUTTON_TYPE rotaryButton = getRotaryButton();
    switch (rotaryButton) {
    case BUTTON_CLICK:
    case BUTTON_DOUBLECLICK:
        // 单击 双击 执行项目
        RunMenuControls(pMenuRoot->menuID, pMenuRoot->index + *pSlideSpace->val);
        break;

    case BUTTON_LONGCLICK:
        // 长按 返回上一个菜单
        RunMenuControls(pMenuRoot->menuID, 0);
        break;

    default:
        break;
    }

    Display();
}
