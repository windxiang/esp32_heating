#include "heating.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/rmt.h"
#include "argtable3/argtable3.h"

static const char* TAG = "WS2812";

#define CHASE_SPEED_MS (10) // 延时

#define RMT_TX_CHANNEL RMT_CHANNEL_0

static led_strip_t* strip = NULL;

/**
 * @brief WS2812 命令结构体
 *
 */
static struct {
    struct arg_int* r;
    struct arg_int* g;
    struct arg_int* b;
    struct arg_end* end;
} ws2812_cmd_args = {};

/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t* r, uint32_t* g, uint32_t* b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

/**
 * @brief WS2812 命令
 *
 * @param argc
 * @param argv
 * @return int
 */
static int do_ws2812_cmd(int argc, char** argv)
{
    int nerrors = arg_parse(argc, argv, (void**)&ws2812_cmd_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, ws2812_cmd_args.end, argv[0]);
        return 1;
    }

    uint32_t r = ws2812_cmd_args.r->count > 0 ? *ws2812_cmd_args.r->ival : 0;
    uint32_t g = ws2812_cmd_args.g->count > 0 ? *ws2812_cmd_args.g->ival : 0;
    uint32_t b = ws2812_cmd_args.b->count > 0 ? *ws2812_cmd_args.b->ival : 0;

    ESP_LOGV(TAG, "WS2812 color r:%d g:%d b:%d\n", r, g, b);
    WS2812Set(r, g, b);
    return 0;
}

void WS2812Clear(void)
{
    if (strip) {
        strip->clear(strip, 100);
        strip->refresh(strip, 100);
    }
}

void WS2812Set(uint32_t red, uint32_t green, uint32_t blue)
{
    if (strip) {
        strip->set_pixel(strip, 0, red, green, blue);
        strip->refresh(strip, 100);
    }
}

/**
 * @brief 初始化WS2812
 *
 */
void WS2812init(void)
{
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(PIN_WS2812RGB, RMT_TX_CHANNEL);
    config.clk_div = 2; // set counter clock to 40MHz

    rmt_config(&config);
    rmt_driver_install(config.channel, 0, 0);

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(STRIP_LED_NUMBER, (led_strip_dev_t)config.channel);
    strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
    }

    // 注册命令
    ws2812_cmd_args.r = arg_intn("r", "red", "<n>", 0, 1, "red color");
    ws2812_cmd_args.g = arg_intn("g", "green", "<n>", 0, 1, "green color");
    ws2812_cmd_args.b = arg_intn("b", "blue", "<n>", 0, 1, "blue color");
    ws2812_cmd_args.end = arg_end(20);
    register_cmd("ws2812", "控制WS2812输出", NULL, do_ws2812_cmd, &ws2812_cmd_args);

    // Clear LED strip (turn off all LEDs)
    WS2812Clear();
}
