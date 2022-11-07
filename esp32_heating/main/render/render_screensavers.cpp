#include "heating.h"
#include "ExternDraw.h"
#include <math.h>

// https://github.com/ArduboyCollection/arduboy-moire

#define MAXSPEED 6 // 最大速度
#define MAXLINES 20 // 最大线条

/**
 * @brief 线段坐标
 *
 */
struct Point {
    int x;
    int y;
};

/**
 * @brief 速度
 *
 */
struct Velocity {
    int dx;
    int dy;
};

/**
 * @brief 点和速度
 *
 */
struct End {
    Point p; // 点
    Velocity v; // 速度
};

/**
 * @brief 绘制线条坐标
 *
 */
struct Line {
    End e0; // 起始点
    End e1; // 结束点
};

struct Scene {
    Line lines[MAXLINES];
    int indexOfHeadLine = MAXLINES - 1;
};

static bool SleepScreenInit = false; // 是否初始化
static Scene scene = {};

// 零坐标 零速度
static Point PointZero = { 0, 0 };
static Velocity VelocityZero = { 0, 0 };
static End EndZero = { PointZero, VelocityZero };
static Line LineZero = { EndZero, EndZero };

/**
 * @brief 生成2个随机坐标
 *
 * @return Point 随机坐标
 */
static Point randomPoint(void)
{
    Point p = {};
    p.x = myRand(0, OLED_SCREEN_WIDTH - 1);
    p.y = myRand(0, OLED_SCREEN_HEIGHT - 1);
    return p;
}

/**
 * @brief 生成随机速度
 *
 * @return int
 */
static int randomSpeed(void)
{
    return myRand(1, MAXSPEED - 1);
}

/**
 * @brief 生成随机速度
 *
 * @return Velocity
 */
static Velocity randomVelocity(void)
{
    return { randomSpeed(), randomSpeed() };
}

/**
 * @brief 生成随机点
 *
 * @return End
 */
static End randomEnd(void)
{
    return { randomPoint(), randomVelocity() };
}

/**
 * @brief
 *
 * @return Line
 */
static Line randomLine(void)
{
    return { randomEnd(), randomEnd() };
}

static int sign(int x)
{
    if (x > 0) {
        return 1;

    } else if (x < 0) {
        return -1;
    }
    return 0;
}

/*
 * functions to advance basic value types
 */
static End endByAdvancingEnd(End en)
{
    int ndx = en.v.dx;
    int ndy = en.v.dy;

    int nx = en.p.x + ndx;
    int ny = en.p.y + ndy;

    if (nx < 0 || nx > OLED_SCREEN_WIDTH) {
        ndx = -1 * sign(ndx) * randomSpeed();
        nx = en.p.x + ndx;
    }
    if (ny < 0 || ny > OLED_SCREEN_HEIGHT) {
        ndy = -1 * sign(ndy) * randomSpeed();
        ny = en.p.y + ndy;
    }

    return {
        { nx, ny },
        { ndx, ndy }
    };
}

static Line lineByAdvancingLine(Line line)
{
    return {
        endByAdvancingEnd(line.e0),
        endByAdvancingEnd(line.e1)
    };
}

/**
 * @brief 初始化
 *
 * @param scene
 */
static void sceneInit(Scene* scene)
{
    for (int i = 0; i < MAXLINES; i++) {
        scene->lines[i] = LineZero;
    }
    scene->indexOfHeadLine = MAXLINES - 1;
    scene->lines[scene->indexOfHeadLine] = randomLine();
}

/**
 * @brief 得到当前线条
 *
 * @param scene
 * @param index
 * @return Line*
 */
static Line* sceneLine(Scene* scene, int index)
{
    int j = (scene->indexOfHeadLine + 1 + index) % MAXLINES;
    return &(scene->lines[j]);
}

static void sceneAdvance(Scene* scene)
{
    Line head = scene->lines[scene->indexOfHeadLine];
    Line nextHead = lineByAdvancingLine(head);

    scene->indexOfHeadLine = (scene->indexOfHeadLine + 1) % MAXLINES;
    scene->lines[scene->indexOfHeadLine] = nextHead;
}

/**
 * @brief 绘制线段
 *
 * @param line
 * @param color
 */
static void drawLine(Line* line, uint16_t color)
{
    u8g2_DrawLine(&u8g2, line->e0.p.x, line->e0.p.y, line->e1.p.x, line->e1.p.y);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 线条
 *
 */
static void DrawIntensiveComputingLine(void)
{
    static uint8_t Line[4];

    for (uint8_t a = 0; a < sizeof(Line) / sizeof(uint8_t); a++) {
        Line[a] += rand() % 2 - 1;
        if (Line[a] > OLED_SCREEN_WIDTH) {
            Line[a] = OLED_SCREEN_WIDTH;
        }

        for (uint8_t b = 0; b < rand() % 3 + 3; b++) {
            u8g2_DrawHLine(&u8g2, 0, Line[a] + rand() % 20 - 10, OLED_SCREEN_WIDTH); //水平线
            u8g2_DrawVLine(&u8g2, Line[a] + rand() % 20 - 10, 0, 64); //垂直线
        }
    }
}

/**
 * @brief 随机点
 *
 */
void DrawIntensiveComputing(void)
{
    ClearOLEDBuffer();

    //随机线条
    DrawIntensiveComputingLine();

    // 随机点
    float calculate = sin(xTaskGetTickCount() / 4000.0f);
    for (int i = 0; i < calculate * 256 + 256; i++) {
        u8g2_DrawPixel(&u8g2, rand() % 128, rand() % 64);
    }

    Display();

    //波浪警告声
    // SetTone(64 + calculate * 64 + rand() % 16 - 8);
    // SetTone(1500 + calculate * 500 + rand() % 64 - 32 - (((xTaskGetTickCount() / 1000) % 2 == 1) ? 440 : 0));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 运行屏保程序
 *
 */
void RenderScreenSavers(void)
{
    // 初始化一次
    if (!SleepScreenInit) {
        sceneInit(&scene);
        SleepScreenInit = true;
    }

    ClearOLEDBuffer();

    // 开始绘图
    Line* tail = sceneLine(&scene, 0);
    drawLine(tail, 0);
    sceneAdvance(&scene);

    for (int i = 0; i < MAXLINES; i++) {
        Line* line = sceneLine(&scene, i);
        drawLine(line, 1);
    }
    Display();
}
