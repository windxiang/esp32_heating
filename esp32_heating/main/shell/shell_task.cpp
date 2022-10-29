#include "heating.h"

#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_vfs_dev.h"

#ifndef PROMPT_STR
#define PROMPT_STR "Heating"
#endif

static struct {
    struct arg_lit* help;
    struct arg_int* int1;
    struct arg_int* int2;
    // struct arg_str* str2;
    struct arg_end* end;
} test_cmd_args;

// 命令提示
const static char* prompt = LOG_COLOR_I PROMPT_STR "> " LOG_RESET_COLOR;

static void initialize_console()
{
    /* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));

    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    uart_config_t uart_config = {};
    uart_config.baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
    uart_config.source_clk = UART_SCLK_REF_TICK;
#else
    uart_config.source_clk = UART_SCLK_XTAL;
#endif

    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config));

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {};
    console_config.max_cmdline_args = 8;
    console_config.max_cmdline_length = 256;
#if CONFIG_LOG_COLORS
    console_config.hint_color = atoi(LOG_COLOR_CYAN);
#endif
    ESP_ERROR_CHECK(esp_console_init(&console_config));

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*)&esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);

    /* Set command maximum length */
    linenoiseSetMaxLineLen(console_config.max_cmdline_length);

    /* Don't return empty lines */
    linenoiseAllowEmpty(false);

#if CONFIG_STORE_HISTORY
    /* Load command history from filesystem */
    linenoiseHistoryLoad(HISTORY_PATH);
#endif
}

static void shell_task(void* arg)
{
    while (1) {
        /* 使用linenoise获得一串字符串。 当按下ENTER时，该行将被返回。 */
        char* line = linenoise(prompt);
        if (line == NULL) { /* Break on EOF or error */
            continue;
        }

        /* 如果不是空的，则将该命令添加到历史记录中 */
        if (strlen(line) > 0) {
            linenoiseHistoryAdd(line);

#if CONFIG_STORE_HISTORY
            /* 保存命令历史到文件系统 */
            linenoiseHistorySave(HISTORY_PATH);
#endif
            /* 尝试运行命令 */
            int ret = 0;
            esp_err_t err = esp_console_run(line, &ret);
            if (err == ESP_ERR_NOT_FOUND) {
                printf("未被识别的命令\n");
            } else if (err == ESP_ERR_INVALID_ARG) {
                // 命令是空的
            } else if (err == ESP_OK && ret != ESP_OK) {
                printf("命令返回非零错误代码: 0x%x (%s)\n", ret, esp_err_to_name(ret));
            } else if (err != ESP_OK) {
                printf("内部错误: %s\n", esp_err_to_name(err));
            }
        }

        /* linenoise在堆上分配了行缓冲区，所以需要释放它。 */
        linenoiseFree(line);
    }

    esp_console_deinit();
}

/**
 * @brief 测试命令
 *
 * @param argc
 * @param argv
 * @return int
 */
static int do_hello_cmd(int argc, char** argv)
{
    printf("Hello World\n");

    int nerrors = arg_parse(argc, argv, (void**)&test_cmd_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, test_cmd_args.end, argv[0]);
        return 1;
    }

    printf("int1=%d\r\n", *test_cmd_args.int1->ival);
    for (int i = 0; i < test_cmd_args.int2->count; i++) {
        printf("int2=%d\r\n", test_cmd_args.int2->ival[i]);
    }
    return 0;
}

/**
 * @brief 注册一个命令
 *
 * @param command
 * @param help
 * @param hint
 * @param func
 * @param argtable
 */
void register_cmd(const char* command, const char* help, const char* hint, esp_console_cmd_func_t func, void* argtable)
{
    esp_console_cmd_t cmd = {};
    cmd.command = command;
    cmd.help = help;
    cmd.hint = hint;
    cmd.func = func;
    cmd.argtable = argtable;
    esp_console_cmd_register(&cmd);
}

static void register_test_cmd(void)
{
    test_cmd_args.help = arg_litn(NULL, "help", 0, 1, "测试"); // 0-1 可选参数 可以不输入
    // test_cmd_args.str2 = arg_str0(NULL, NULL, "<str2>", "test str2");
    test_cmd_args.int1 = arg_intn("k", "scalar", "<n>", 1, 1, "foo value"); // 最少要1个
    test_cmd_args.int2 = arg_intn("q", NULL, "<n>", 1, 2, "foo value"); // 最少要1个 最大输入2个
    test_cmd_args.end = arg_end(20);

    register_cmd("hello", "这是一个测试命令", NULL, do_hello_cmd, &test_cmd_args);
}

void shell_init(void)
{
    // 注册控制台
    initialize_console();

    // 注册控制台命令
    esp_console_register_help_command();

    // 注册一个测试命令
    register_test_cmd();

    printf("\n"
           "这是一个ESP-IDF控制台组件.\n"
           "输入'help'以获得命令列表.\n"
           "使用上/下箭头在命令历史中导航.\n"
           "输入命令名称时按TAB键自动完成.\n"
           "按回车键或Ctrl+C将终止控制台环境.\n");

    /* 弄清终端是否支持转义序列 */
    int probe_status = linenoiseProbe();
    if (probe_status) { /* zero indicates success */
        printf("\n\n\n"
               "你的终端应用程序不支持转义序列.\n"
               "行编辑和历史功能被禁用.\n"
               "在Windows上，尝试使用Putty代替.\n\n\n");

        linenoiseSetDumbMode(1);

#if CONFIG_LOG_COLORS
        // 终端不支持转义序列,所以不要加入颜色代码
        prompt = PROMPT_STR "> ";
#endif // CONFIG_LOG_COLORS
    }

    // 初始化shell
    xTaskCreatePinnedToCore(shell_task, "render", 1024 * 5, NULL, 5, NULL, tskNO_AFFINITY);
}