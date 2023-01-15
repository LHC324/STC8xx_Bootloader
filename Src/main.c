#include "GPIO.h"
#include "usart.h"
#include "eeprom.h"
#include "spi.h"
#include "w25qx.h"

/*************	本地变量声明	**************/
static uint8_t data_buf[0x200];
// sfr IAP_TPS = 0xf5; // EEPROM擦除等待时间控制寄存器
#define read_jmp_addr (*(unsigned char code *)(USER_FLASH_ADDR))       // 读取jmp指令
#define read_file_size_addr (*(unsigned char code *)(USER_FLASH_ADDR)) // 读取文件尺寸
#define read_app_addr (*(unsigned int code *)(APP_JMP_ADDR))           // 读取app的跳转地址
#define read_ota_addr (*(unsigned char code *)(OTA_FLAG_ADDR))         // 读取ota标志
void (*jmp_app)(void);                                                 // 跳转函数

/*************	本地函数声明	**************/
typedef struct
{
    uint16_t ota_value;
    uint16_t file_size;
} file_info;

static void reset_ota_flag(void)
{
    file_info info = {0, 0};

    dev_flash_read_bytes((uint8_t *)&info, OTA_FLAG_ADDR, sizeof(info));

    if (OTA_FLAG_VALUE == info.ota_value)
    {
        info.ota_value = ~OTA_FLAG_VALUE;
        dev_flash_auto_adapt_erase(OTA_FLAG_ADDR, sizeof(info));
        dev_flash_write_page((uint8_t *)&info, OTA_FLAG_ADDR, sizeof(info));
    }

    // uint16_t ota_flag = ~OTA_FLAG_VALUE;
    // if (read_ota_addr != (~OTA_FLAG_VALUE))
    //     IapWrites(OTA_FLAG_ADDR, (uint8_t *)&ota_flag, sizeof(ota_flag)); // 擦除ota标志
}

static void set_ota_flag(void)
{
    file_info info = {0, 0};

    dev_flash_read_bytes((uint8_t *)&info, OTA_FLAG_ADDR, sizeof(info));

    if ((~OTA_FLAG_VALUE) == info.ota_value)
    {
        info.ota_value = OTA_FLAG_VALUE;
        dev_flash_auto_adapt_erase(OTA_FLAG_ADDR, sizeof(info));
        dev_flash_write_page((uint8_t *)&info, OTA_FLAG_ADDR, sizeof(info));
    }

    // uint16_t ota_flag = OTA_FLAG_VALUE;
    // if (read_ota_addr != OTA_FLAG_VALUE)
    //     IapWrites(OTA_FLAG_ADDR, (uint8_t *)&ota_flag, sizeof(ota_flag)); // 写入ota标志
}

// #if !defined USING_SIMULATE
static void display_hex_data(uint8_t *dat, uint32_t len)
{
#define __is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')
#define HEXDUMP_WIDTH 16
    uint16_t i, j;

    if (NULL == dat || !len)
        return;

    for (i = 0; i < len; i += HEXDUMP_WIDTH)
    {
        Uartx_Printf(OTA_WORK_UART, "\r\n[%04x]: ", i);
        for (j = 0; j < HEXDUMP_WIDTH; ++j)
        {
            if (i + j < len)
            {
                Uartx_Printf(OTA_WORK_UART, "%02bX ", dat[i + j]);
            }
            else
            {
                Uartx_Printf(OTA_WORK_UART, "   ");
            }
        }
        Uartx_Printf(OTA_WORK_UART, "\t\t");
        for (j = 0; (i + j < len) && j < HEXDUMP_WIDTH; ++j)
            Uartx_Printf(OTA_WORK_UART, "%c",
                         __is_print(dat[i + j]) ? dat[i + j] : '.');
    }
    if (len)
        Uartx_Printf(OTA_WORK_UART, "\r\n\r\n");
}
// #endif

static void Delay1000ms() //@24MHz
{
    unsigned char i, j, k;

    _nop_();
    _nop_();
    i = 255;
    j = 193;
    k = 128;
    do
    {
        do
        {
            while (--k)
                ;
        } while (--j);
    } while (--i);
}

void main(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    file_info info = {0, 0};
    uint16_t i;
#define USING_W25QX______________________________________
    {
#define W25QX_NSS GPIO_Pin_2
#define W25QX_MOSI GPIO_Pin_3
#define W25QX_MISO GPIO_Pin_4
#define W25QX_CLK GPIO_Pin_5
#define W25QX_PORT GPIO_P2

        SPIInit_Type spi = {
            SPI_Type_Master,
            SPI_SCLK_DIV_16,
            SPI_Mode_0,
            SPI_Tran_MSB,
            true,
        };

        GPIO_InitStruct.Mode = GPIO_OUT_PP;
        GPIO_InitStruct.Pin = W25QX_NSS | W25QX_CLK | W25QX_MOSI;
        GPIO_Inilize(W25QX_PORT, &GPIO_InitStruct);

        GPIO_InitStruct.Mode = GPIO_HighZ;
        GPIO_InitStruct.Pin = W25QX_MISO;
        GPIO_Inilize(W25QX_PORT, &GPIO_InitStruct);

        GPIO_spi_sw_port(SW_Port2);
        spi_init(&spi);
    }
#if !defined USING_SIMULATE
    Uart1_Init();
#endif
    /*串口2初始化*/
    Uart2_Init();
#if (!DWIN_USING_RB)
    /*关闭全局中断*/
    CLOSE_GLOBAL_OUTAGE();
#else
    IE2 &= ~0x80;
    IE2 &= ~0x10;
    OPEN_GLOBAL_OUTAGE();
#endif

    // IAP_TPS = 24; // STC8G和STC8H的设置，默认24MHz。
    Uartx_Printf(OTA_WORK_UART, "SPI FLASH ID: %#x.\r\n", dev_flash_read_device_id());
    while (1)
    {
    __read_ota_flag:
        // Iap_Reads(OTA_FLAG_ADDR, (uint8_t *)&info, sizeof(info)); // 读取文件信息
        dev_flash_read_bytes((uint8_t *)&info, OTA_FLAG_ADDR, sizeof(info));
        Uartx_Printf(OTA_WORK_UART, "ota_flag\tfile_size\r\n%#x\t\t%#X\r\n", info.ota_value, info.file_size);
        if (info.file_size && (OTA_FLAG_VALUE == info.ota_value)) // OTA_FLAG_VALUE == read_ota_addr                  // IapRead(OTA_FLAG_ADDR)
        {
            Uartx_Printf(OTA_WORK_UART, " start erase...\r\n");
            for (i = 0; i < info.file_size && i < (USER_FLASH_ADDR - SECTOR_SIZE); i += SECTOR_SIZE) // 擦除N页
            {
                dev_flash_read_bytes(data_buf, i, SECTOR_SIZE);
                // display_hex_data(data_buf, SECTOR_SIZE);

                // Uartx_Printf(OTA_WORK_UART, "read:\r\n");
                // // Iap_Reads(i, data_buf, SECTOR_SIZE);
                // display_hex_data(data_buf, SECTOR_SIZE);

                if (0 == i)
                {
                    Iap_Reads(i, data_buf, 3U); // 读出3字节boot地址
                    Uartx_Printf(OTA_WORK_UART, "boot loader addr: 0x%bX 0x%bX 0x%bX .\r\n",
                                 data_buf[0], data_buf[1], data_buf[2]);
                }
                // IapErase(i);

                IapWrites(i, data_buf, sizeof(data_buf));

                Uartx_Printf(OTA_WORK_UART, "rate: %02d%%\b\b\b\r", (uint16_t)(((uint32_t)i * 100U) / info.file_size));
            }

            /*取消升级标志*/
            reset_ota_flag();
            Uartx_Printf(OTA_WORK_UART, "\r\nProgramming completed.\r\n");
            goto __read_ota_flag;
        }
        // Uartx_Printf(OTA_WORK_UART, "read:\r\n");
        // Iap_Reads(0, data_buf, SECTOR_SIZE);
        // display_hex_data(data_buf, SECTOR_SIZE);

        dev_flash_read_bytes(data_buf, 0x00, 3U);
        if (((~OTA_FLAG_VALUE) == info.ota_value) && (data_buf[0] == LJMP))
        {
#if (DWIN_USING_RB)
            /*关闭全局中断*/
            CLOSE_GLOBAL_OUTAGE();
#endif
            jmp_app = (uint32_t)((uint16_t)data_buf[1] << 8U | data_buf[2]);
            Uartx_Printf(OTA_WORK_UART, "start jump user app[%#X]......\r\n",
                         (uint16_t)data_buf[1] << 8U | data_buf[2]);
            jmp_app();
        }
        else
        {
            // set_ota_flag(); // 首次从SPI flash加载到内部ROM
            // if (read_jmp_addr != 0xFF)
            //     IapErase(USER_FLASH_ADDR); // 擦除跳转地址，防止变砖
            Uartx_Printf(OTA_WORK_UART, "illegal instruction!\r\n");
        }

        if ((data_buf[0] == LJMP)) // 解决内部rom和外部flash不匹配问题
            set_ota_flag();

        /*程序区损坏或者写入期间断电导致升级失败，写入ota标志，请求更新用户程序*/
        // set_ota_flag();

        Uartx_Printf(OTA_WORK_UART, "no appliction !\r\n");
        Delay1000ms();
    }
}

/**
 * @brief	复位硬件寄存器
 * @details	部分外设寄存器在上电时默认值并不为0，需要清除其默认值
 * @param	None
 * @retval	None
 */
// static void Reset_Registers(void)
// {
//     AUXR = 0x00;
//     S2CON = 0x00;
//     SCON = 0;
//     TMOD = 0;
//     TL0 = 0;
//     TH0 = 0;
//     TH1 = 0;
//     TL1 = 0;
//     TCON = 0;
//     IAP_CMD = 0;
// }
