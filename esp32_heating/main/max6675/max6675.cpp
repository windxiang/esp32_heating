#include "heating.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char* TAG = "MAX6675";

#define SPI_HOST_ID SPI3_HOST // 使用SPI3
#define MAX6675_DMA_CHAN SPI_DMA_CH_AUTO // 默认使用的SPI-DMA通道
#define DMA_MAX_SIZE 32 // DMA最大传输字节输入

// 外设与SPI关联的句柄，通过此来调用SPI总线上的外设
static spi_device_handle_t SPIDeviceHandle = NULL;

#if 0
static void update_timer_callback(TimerHandle_t timer)
{
	struct max6675_state *state;
	int ret;
	spi_transaction_t trans = {
		.length = 2,
		.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA,
	};
	state = (struct max6675_state *)pvTimerGetTimerID(timer);
	ret = spi_device_polling_transmit(state->spi, &trans);
	ESP_ERROR_CHECK(ret);

	ESP_LOGI(TAG, "Received %u bytes: 1: 0x%02X 2: 0x%02X", trans.rxlength, trans.rx_data[0],
		 trans.rx_data[1]);
}
#endif

/**
 * @brief 读取摄氏度
 *
 * @return bool
 */
bool MAX6675ReadCelsius(float* pTemp)
{
    if (pTemp == NULL) {
        return false;
    }
    *pTemp = 0;

    uint16_t data = 0;

    spi_transaction_t trans = {};
    trans.tx_buffer = NULL;
    trans.rx_buffer = &data;
    trans.length = 16;
    trans.rxlength = 16;

    // gpio_set_level(PIN_MAX6675_SPI_CS, 0);
    // delay(1);

    spi_device_acquire_bus(SPIDeviceHandle, portMAX_DELAY);
    spi_device_transmit(SPIDeviceHandle, &trans);
    spi_device_release_bus(SPIDeviceHandle);

    uint16_t res = (int16_t)SPI_SWAP_DATA_RX(data, 16);

    // gpio_set_level(PIN_MAX6675_SPI_CS, 1);

    // ESP_LOGE(TAG, "SPI res:0x%x\n", res);
    if (res & (1 << 2)) {
        // ESP_LOGE(TAG, "Sensor is not connected\n");

    } else {
        res >>= 3;
        // ESP_LOGI(TAG, "SPI temp=%f\n", res * 0.25f);
        *pTemp = res * 0.25f;
        return true;
    }
    return false;
}

/**
 * @brief 读取华氏度
 *
 * @return float
 */
float MAX6675ReadFahrenheit(void)
{
    float temp = 0;
    if (MAX6675ReadCelsius(&temp)) {
        return temp * 9.0f / 5.0f + 32.0f;
    }
    return 0.0f;
}

/**
 * @brief  初始化SPI总线，配置为 SPI mode 1.(CPOL=0, CPHA=1)，CS引脚使用软件控制
 *      - 初始化除了设置SPI总线，没有其他过程，不用配置寄存器。
 *      - 例：spi_as5047p_init(SPI3_HOST, 100*1000, AS5047P_SOFT_CS0);
 *
 * @param  host_id SPI端口号。SPI1_HOST / SPI2_HOST / SPI3_HOST
 * @param  clk_speed 设备的SPI速度
 * @param  cs_io_num CS端口号，使用软件控制
 *
 * @return
 *     - none
 */
static void spi_max6675_init(spi_host_device_t host_id, uint32_t clk_speed, gpio_num_t cs_io_num)
{
    // 先关联 SPI总线及设备
    spi_device_interface_config_t devcfg = {};

    devcfg.clock_speed_hz = clk_speed; // CLK时钟频率
    devcfg.mode = 0; // SPI mode (CPOL=0, CPHA=1)
    devcfg.queue_size = 7; // 事务队列大小
    // devcfg.spics_io_num = -1; // CS引脚定义
    devcfg.spics_io_num = cs_io_num; // CS引脚定义

    // 将外设与SPI总线关联
    esp_err_t ret = spi_bus_add_device(host_id, &devcfg, &SPIDeviceHandle);
    ESP_ERROR_CHECK(ret);

    // 配置软件cs引脚
    // gpio_pad_select_gpio(cs_io_num);
    // gpio_set_direction(cs_io_num, GPIO_MODE_OUTPUT);
    // gpio_set_level(cs_io_num, 1);
}

/**
 * @brief  配置SPIx主机模式，配置DMA通道、DMA字节大小，及 MISO、MOSI、CLK的引脚。
 *      - （注意：普通GPIO最大只能30MHz，而IOMUX默认的SPI-IO，CLK最大可以设置到80MHz）
 *      - 例：spi_master_init(SPI2_HOST, LCD_DEF_DMA_CHAN, LCD_DMA_MAX_SIZE, SPI2_DEF_PIN_NUM_MISO, SPI2_DEF_PIN_NUM_MOSI, SPI2_DEF_PIN_NUM_CLK);
 *
 * @param  host_id SPI端口号。SPI1_HOST / SPI2_HOST / SPI3_HOST
 * @param  dma_chan 使用的DMA通道
 * @param  max_tran_size DMA最大的传输字节数（会根据此值给DMA分配内存，值越大分配给DMA的内存就越大，单次可用DMA传输的内容就越多）
 * @param  miso_io_num MISO端口号。除仅能做输入 和 6、7、8、9、10、11之外的任意端口，但仅IOMUX默认的SPI-IO才能达到最高80MHz上限。
 * @param  mosi_io_num MOSI端口号
 * @param  clk_io_num CLK端口号
 *
 * @return
 *     - none
 */
static void spi_master_init(spi_host_device_t host_id, int dma_chan, uint32_t max_tran_size, int miso_io_num, int mosi_io_num, int clk_io_num)
{
    // 配置 MISO、MOSI、CLK、CS 的引脚，和DMA最大传输字节数
    spi_bus_config_t buscfg = {};
    buscfg.miso_io_num = miso_io_num; // MISO引脚定义
    buscfg.mosi_io_num = mosi_io_num; // MOSI引脚定义
    buscfg.sclk_io_num = clk_io_num; // CLK引脚定义
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = max_tran_size; // 最大传输字节数

    // 初始化SPI总线
    esp_err_t ret = spi_bus_initialize(host_id, &buscfg, dma_chan);
    ESP_ERROR_CHECK(ret);
}

void max6675Init(void)
{
    spi_master_init(SPI_HOST_ID, MAX6675_DMA_CHAN, DMA_MAX_SIZE, PIN_MAX6675_SPI_MISO, GPIO_NUM_NC, PIN_MAX6675_SPI_CLK);
    spi_max6675_init(SPI_HOST_ID, 4 * 1000 * 1000, PIN_MAX6675_SPI_CS);
    delay(100);
}
