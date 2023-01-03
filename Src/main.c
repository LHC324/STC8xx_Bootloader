#include "GPIO.h"
#include "usart.h"
#include "eeprom.h"
#include "ymodem.h"

// #define xprintf(fmt, args...) (Uartx_Printf(OTA_WORK_UART, fmt, ##args))
/*************	本地变量声明	**************/
// sfr IAP_TPS = 0xf5; // EEPROM擦除等待时间控制寄存器
#define read_app_addr (*(unsigned int code *)(APP_JMP_ADDR))   // 读取app的跳转地址。
#define read_ota_addr (*(unsigned char code *)(OTA_FLAG_ADDR)) // 读取ota标志。
void (*jmp_app)(void);                                         // 跳转函数。

// ymodem xdata ymo = {0};

/*************	本地函数声明	**************/
extern uint8_t *uint32_to_string(uint32_t num);
ym_err_t Uartx_RecivePackge(void);
static ym_err_t Ymodem_Download(ymodem_t ym, uartx_rx_t rx);

void report_isp_information(ymodem_t ym)
{
    if (NULL == ym)
        return;

    Uartx_Printf(OTA_WORK_UART, "================================\r\n File name: %s\r\n", ym->file_name);
    Uartx_Printf(OTA_WORK_UART, "File length: %dBytes\r\n", ym->file_size);
    Uartx_Printf(OTA_WORK_UART, "Bootloader Version:   2022-12-29 by LHC\r\n");
    Uartx_Printf(OTA_WORK_UART, "================================\r\n\r\n");
}

uint8_t ota_flag;
static void reset_ota_flag(void)
{
    ota_flag = ~OTA_FLAG_VALUE;
    IapWrites(OTA_FLAG_ADDR, &ota_flag, sizeof(ota_flag)); // 擦除ota标志
}

static void set_ota_flag(void)
{
    ota_flag = OTA_FLAG_VALUE;
    IapWrites(OTA_FLAG_ADDR, &ota_flag, sizeof(ota_flag)); // 写入ota标志
}

void main(void)
{
    ymodem xdata ymo = {0};
    bool status = false;
    ymodem_t ym = &ymo;
    uartx_rx_t rx = &Boot_Rx;
    ym_err_t res;
    uint16_t count;

    // Reset_Registers();
    Gpio_Init();
    // #if (OTA_INFO_OUT_UART)
    Uart1_Init();
    // #endif
    /*串口2初始化*/
    Uart2_Init();
    /*关闭全局中断*/
    CLOSE_GLOBAL_OUTAGE();
    // IAP_TPS = 24; // STC8G和STC8H的设置，默认24MHz。

    while (1)
    {
        if (OTA_FLAG_VALUE == read_ota_addr) // IapRead(OTA_FLAG_ADDR)
        {
            Uartx_Printf(OTA_WORK_UART, "\r\n Press 'd' to start......\r\n");
            for (count = 0; count < WAITTIMES; count++)
            {
                Uartx_RecivePackge();
                if ((Boot_Rx.Rx_Counts == 1U) && (Boot_Rx.Rx_buffer[0] == 'd'))
                {
                    count = 250U;
                    // Uart_PutString("STC8xx BootLoader is Running.\r\n");
                    res = Ymodem_Download(ym, rx);
                    switch (res)
                    {
                    case ym_ok:
                        Uartx_Printf(OTA_WORK_UART, "\r\n Programming Completed Successfully !\r\n");
                        report_isp_information(ym);
                        reset_ota_flag();
                        goto __exe_app;
                        break;
                    case ym_user_cancel:
                        Uartx_Printf(OTA_WORK_UART, "\r\n MCU abort !\r\n");
                        break;
                    case ym_pc_cancel:
                        Uartx_Printf(OTA_WORK_UART, "\r\n User abort !\r\n");
                        break;
                    case ym_file_size_large:
                        Uartx_Printf(OTA_WORK_UART, "\r\n File size is too large !\r\n");
                        break;
                    case ym_program_err:
                        Uartx_Printf(OTA_WORK_UART, "\r\n Programming Error !\r\n");
                        break;
                    default:
                        Uartx_Printf(OTA_WORK_UART, "\r\n other error: %bd.\r\n", res);
                        break;
                    }
                }
            }
            /*超时退出，取消升级标志*/
            reset_ota_flag();
        }

    __exe_app:
        if (IapRead(ISP_ADDRESS - 3U) == LJMP)
        {
            jmp_app = (uint32_t)read_app_addr;
            Uartx_Printf(OTA_WORK_UART, "\r\n\r\n start jump user app[%#X]......\r\n", (uint16_t)read_app_addr);
            jmp_app();
        }
        else
            Uartx_Printf(OTA_WORK_UART, "\r\n\r\n Illegal instruction!\r\n");

        /*程序区损坏或者写入期间断电导致升级失败，写入ota标志，请求更新用户程序*/
        set_ota_flag();

        Uartx_Printf(OTA_WORK_UART, "NO Appliction !\r\n");
    }
}

void display_hex_data(uint8_t *dat, uint32_t len)
{
#define __is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')
#define HEXDUMP_WIDTH 16
    uint16_t i, j;

    if (NULL == dat || !len)
        return;

    for (i = 0; i < len; i += HEXDUMP_WIDTH)
    {
        Uartx_Printf(OTA_INFO_OUT_UART, "\r\n[%04x]: ", i);
        for (j = 0; j < HEXDUMP_WIDTH; ++j)
        {
            if (i + j < len)
            {
                Uartx_Printf(OTA_INFO_OUT_UART, "%02bX ", dat[i + j]);
            }
            else
            {
                Uartx_Printf(OTA_INFO_OUT_UART, "   ");
            }
        }
        Uartx_Printf(OTA_INFO_OUT_UART, "\t\t");
        for (j = 0; (i + j < len) && j < HEXDUMP_WIDTH; ++j)
            Uartx_Printf(OTA_INFO_OUT_UART, "%c",
                         __is_print(dat[i + j]) ? dat[i + j] : '.');
    }
    if (len)
        Uartx_Printf(OTA_INFO_OUT_UART, "\r\n\r\n");
}

static uint16_t ym_crc16(unsigned char *q, int len)
{
    uint16_t crc;
    char i;

    crc = 0;
    while (--len >= 0)
    {
        crc = crc ^ (int)*q++ << 8;
        i = 8;
        do
        {
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        } while (--i);
    }

    return (crc);
}

ym_err_t Uartx_RecivePackge(void)
{
    uint16_t j; // 5ms超时

    Boot_Rx.Rx_Counts = 0;
    for (j = 0; j < TIMEOUTS; j++) // 最后收到一个字节5ms后,超时退出	5300
    {
        if (OTA_UART_RI)
        {
            RESET_OTA_RI();
            Boot_Rx.Rx_buffer[Boot_Rx.Rx_Counts] = OTA_UART_SBUF;
            if (++Boot_Rx.Rx_Counts >= RX_BUFFER_SIZE)
                // Boot_Rx.Rx_Counts = 0;
                return ym_recv_err;
            j = 0; // 重新定时5ms
        }
    }
#if (USE_PRINTF_DEBUG == 1)
    if (Boot_Rx.Rx_Counts < 140U)
        display_hex_data(Boot_Rx.Rx_buffer, Boot_Rx.Rx_Counts);
#endif
        // if (Boot_Rx.Rx_Counts)
        //     return ym_ok;
#if (WDT_ENABLE == 1)
    WDT_reset(D_WDT_SCALE_256);
#endif
    return ym_ok;
}

// #pragma OT(0)
// void ym_delay_10ms() //@24.000MHz
// {
//     unsigned char i, j, k;

//     i = 2;
//     j = 56;
//     k = 172;
//     do
//     {
//         do
//         {
//             while (--k)
//                 ;
//         } while (--j);
//     } while (--i);
// }
// #pragma OT(9)

static ym_err_t ymodem_check_data_packet(uartx_rx_t rx)
{
#define RX_HEAD() (rx->Rx_buffer[0])
#define RX_FRAME(_id) (rx->Rx_buffer[_id])
#define RX_LEN() (rx->Rx_Counts)
    if (NULL == rx)
        return ym_err_other;

    if (RX_LEN() == 1) // 1字节
    {
        if (RX_HEAD() == EOT) // 收到EOT, 结束
            return YM_ACK;
        if ((RX_HEAD() == ABORT1) || (RX_HEAD() == ABORT2)) // 收到A或a, 取消
            return ym_user_cancel;                          // 用户取消
    }
    else if (RX_LEN() <= 5) // 小于5字节
    {
        if ((RX_HEAD() == CANCEL) && (RX_FRAME(1) == CANCEL)) // 收到两个CANCEL
            return ym_pc_cancel;                              // PC端取消
    }
    else if ((RX_LEN() == 133) || (RX_LEN() == 1029)) // 数据包
    {
        if (RX_FRAME(PACKET_SEQNO_INDEX) != (RX_FRAME(PACKET_SEQNO_COMP_INDEX) ^ 0xff)) // 判断发送序号是否正确
        {
            return YM_NACK; // 错误, 请求重发
        }
#if (USE_PRINTF_DEBUG == 1)
        Uartx_Printf(OTA_INFO_OUT_UART, "Get complete data package[%bd].\r\n", RX_FRAME(1));
#endif
        return ym_ok;
    }

    // return ym_err_other;
    return YM_NACK;
}

static ym_err_t ymodem_putchar(ym_err_t ym_code)
{
    char c;
    uint8_t i;
    static uint8_t errors = 0;
    switch (ym_code)
    {
    case YM_PUT_C:
        c = CRC16;
        break;
    case YM_ACK:
        c = ACK;
        errors = 0;
        break;
    case YM_NACK:
        c = NAK;
        if (errors++ > MAX_ERRORS)
            goto __exit;
        break;
    case YM_CAN:
        c = CANCEL;
        for (i = 0; i < RYM_END_SESSION_SEND_CAN_NUM; ++i)
            Uartx_SendStr(OTA_WORK_UART, (uint8_t *)&c, sizeof(c), UART_BYTE_SENDOVERTIME);
        return ym_ok;
    __exit:
    default:
        return ym_err_other;
    }
    Uartx_SendStr(OTA_WORK_UART, (uint8_t *)&c, sizeof(c), UART_BYTE_SENDOVERTIME);
    return ym_ok;
}

/**
 * @brief	ymodem开始接收数据前处理工作
 * @version V1.0.0,2022/01/18
 * @details
 * @param	None
 * @retval	None
 */
static ym_err_t ymodem_on_begin(ymodem_t ym, uartx_rx_t rx)
{
    uint8_t file_size[FILE_SIZE_LENGTH];
    // uint8_t user_isp_code[3];
    uint8_t site = PACKET_HEADER;
    uint16_t i = 0;

    if ((NULL == ym) || (NULL == rx))
        return ym_recv_err;

    if ('\0' == RX_FRAME(PACKET_HEADER))
        return ym_file_name_err;

    /*保存文件名*/
    for (i = 0; (i < FILE_NAME_LENGTH) && (RX_FRAME(site) != '\0'); i++)
        ym->file_name[i] = RX_FRAME(site++);

    memset(file_size, 0x00, FILE_SIZE_LENGTH);
    for (i = 0, site++; (RX_FRAME(site) != '\0') && (i < FILE_SIZE_LENGTH); i++)
        file_size[i] = RX_FRAME(site++);   // 保存文件长度
    ym->file_size = Str_To_Int(file_size); // 文件长度由字符串转成十六进制数据
#if (USE_PRINTF_DEBUG == 1)
    Uartx_Printf(OTA_INFO_OUT_UART, "[file name]: %s, [file size]: %d bytes.\r\n",
                 ym->file_name, ym->file_size);
#endif
    if (ym->file_size >= USER_FLASH_SIZE) // 长度过长错误
    {
        ymodem_putchar(YM_CAN);    // 错误返回N个 CA
        return ym_file_size_large; // 长度过长
    }
    Iap_Reads(FLASH_START_ADDR, ym->jmp_code, sizeof(ym->jmp_code));
#if (USE_PRINTF_DEBUG == 1)
    Uartx_Printf(OTA_INFO_OUT_UART, "boot loader addr: 0x%bX 0x%bX 0x%bX .\r\n",
                 ym->jmp_code[0], ym->jmp_code[1], ym->jmp_code[2]);

#endif
    // IapWrites(FLASH_START_ADDR, user_isp_code, sizeof(user_isp_code));

    // #if (USE_PRINTF_DEBUG == 1)
    //     memset(user_isp_code, 0x00, sizeof(user_isp_code));
    //     Iap_Reads(FLASH_START_ADDR, user_isp_code, sizeof(user_isp_code));
    //     Uartx_Printf(OTA_INFO_OUT_UART, "real boot: %bx %bx %bx .\r\n",
    //                  user_isp_code[0], user_isp_code[1], user_isp_code[2]);
    // #endif
    for (i = FLASH_START_ADDR; i < USER_FLASH_SIZE; i += SECTOR_SIZE) // 擦除N页
        IapErase(i);

    // memset(ym, 0x00, sizeof(ymodem));
    ym->next_flash_addr = FLASH_START_ADDR; //+ 3U 记录数据帧开始写入的首地址(3byte ISP code + 3Byte NULL)
    // ym->check_sum = 0;
    ym->packets = 0;
    ymodem_putchar(YM_ACK); // 擦除完成, 返回应答
    return ym_ok;
}

/**
 * @brief	ymodem发送握手信号
 * @version V1.0.0,2022/01/18
 * @details
 * @param	None
 * @retval	None
 */
static ym_err_t ymodem_handshake(ymodem_t ym, uartx_rx_t rx)
{
    ym_err_t res;
    uint16_t count;
    uint32_t recv_timeout;

    if (NULL == ym || NULL == rx)
        return ym_err_other;

    for (count = 0; count < (YM_HANDSHAKE_TIMEOUT * 10U); ++count) // 30s超时退出
    {
        // Uartx_SendStr(OTA_WORK_UART, &c, sieof(c), UART_BYTE_SENDOVERTIME);
        ymodem_putchar(YM_PUT_C);
        recv_timeout = YM_HANDSHAKE_TIMEOUT;
        while (--recv_timeout)
        {
            if (Uartx_RecivePackge() != ym_ok)
                return ym_recv_err;

            res = ymodem_check_data_packet(rx);
            if (res != ym_ok && res != ym_err_other)
                return res;

            if ((SOH == RX_HEAD()) && (RE_PACKET_128B_SIZE == RX_LEN()))
                return ymodem_on_begin(ym, rx);
        }
    }
    return ym_rec_timeout;
}

void ym_memcpy(void *s1, const void *s2, size_t n)
{
    uint8_t *dest = (uint8_t *)s1;
    const uint8_t *source = (const uint8_t *)s2;

    if (NULL == dest || NULL == source)
        return;

    while (n--)
    {
        *dest++ = *source++;
    }
}

// uint8_t ymodem_buf[1024];
// #pragma OT(0)
/**
 * @brief	用户处理数据帧
 * @details
 * @param	None
 * @retval	None
 */
static ym_err_t ymodem_on_data(ymodem_t ym, uartx_rx_t rx)
{
    uint16_t base_addr, rqe_crc16, rel_crc16;
    uint16_t write_size = RX_LEN() - PACKET_OVERHEAD; // 去掉5字节帧头、帧尾
    uint8_t *pdat = &RX_FRAME(PACKET_HEADER), t_jmp_code[3];

    if (NULL == ym || NULL == rx)
        return ym_err_other;

    rqe_crc16 = ym_crc16(&RX_FRAME(PACKET_HEADER), RX_LEN() - 5U);
    rel_crc16 = ((uint16_t)RX_FRAME(RX_LEN() - 2U) << 8U) | RX_FRAME(RX_LEN() - 1U);
#if (USE_PRINTF_DEBUG == 1)
    Uartx_Printf(OTA_INFO_OUT_UART, "req_crc\trel_crc\r\n%#x\t%#x\r\n", rqe_crc16, rel_crc16);
#endif
    if (rqe_crc16 != rel_crc16)
        return ym_recv_err;

    if (0 == ym->packets) // 第0帧数据特殊处理
    {
        ym_memcpy(t_jmp_code, pdat, sizeof(t_jmp_code));           // 取出应用程序跳转地址
        ym_memcpy(pdat, ym->jmp_code, sizeof(ym->jmp_code));       // 写回bootloader跳转地址
        ym_memcpy(ym->jmp_code, t_jmp_code, sizeof(ym->jmp_code)); // 记录app跳转地址

        // memcpy(t_jmp_code, &pdat[0], sizeof(t_jmp_code));     // 取出应用程序跳转地址
        // memcpy(&pdat[0], ym->jmp_code, sizeof(ym->jmp_code)); // 写回bootloader跳转地址
        // //                                                         // memcpy(pdat, ym->jmp_code, sizeof(ym->jmp_code)); // 写回bootloader跳转地址
        // //                                                         // pdat[0] = ym->jmp_code[0], pdat[1] = ym->jmp_code[1], pdat[2] = ym->jmp_code[2];
        // memcpy(ym->jmp_code, t_jmp_code, sizeof(ym->jmp_code)); // 记录app跳转地址
    }

    // if (ym->next_flash_addr >= USER_FLASH_SIZE)
    //     return ym_program_err;

    // IapWrites(ym->next_flash_addr, pdat, write_size);
    // ym->next_flash_addr += write_size;

    for (base_addr = ym->next_flash_addr;
         pdat && ym->next_flash_addr < base_addr + write_size;
         ++ym->next_flash_addr, ++pdat) // 去掉3个字节帧头
    {
        // if (ym->next_flash_addr < ym->file_size)
        //     ym->check_sum += *pdat;

        IapProgram(ym->next_flash_addr, *pdat);

        if (ym->next_flash_addr >= USER_FLASH_SIZE)
            return ym_program_err;
    }

#if (USE_PRINTF_DEBUG == 1)
    Uartx_Printf(OTA_INFO_OUT_UART, "start\tend\tsize\tpackets\r\n%#x\t%#x\t%#x\t%#x\r\n",
                 (ym->next_flash_addr - write_size), ym->next_flash_addr, write_size, ym->packets);
#endif
    return ym_ok;
}
// #pragma OT(9)

/**
 * @brief	ymodem连续接收数据帧
 * @details
 * @param	None
 * @retval	None
 */
static ym_err_t ymodem_do_trans(ymodem_t ym, uartx_rx_t rx)
{
    uint32_t timeout = YM_RECV_TIMEOUT;
    ym_err_t ym_res, cmd;

    if (NULL == ym || NULL == rx)
        return ym_err_other;

    ymodem_putchar(YM_PUT_C); // 请求pc发送数据帧
#if (USE_PRINTF_DEBUG == 1)
    Uartx_Printf(OTA_INFO_OUT_UART, "start recv data.\r\n");
#endif
__wait:
    while (--timeout)
    {
        if (Uartx_RecivePackge() != ym_ok)
            return ym_recv_err;

        if (0 == RX_LEN())
            goto __wait;

        cmd = ymodem_check_data_packet(rx);

#if (USE_PRINTF_DEBUG == 1)
        Uartx_Printf(OTA_INFO_OUT_UART, "cmd: %bd.\r\n", cmd);
#endif

        if (YM_ACK == cmd) // EOT
            return ym_ok;
        else if (YM_NACK == cmd)
        {
            cmd = ymodem_putchar(YM_NACK); // 数据包不完整：请求重发
        }

        if ((cmd != ym_ok)) //&& (cmd != YM_NACK)
            return cmd;
        else
        {
            ym_res = ymodem_on_data(ym, rx);
            if (ym_ok != ym_res)
                return ym_res;

            timeout = YM_RECV_TIMEOUT;
        }

        if (ym_ok == cmd)
            cmd = YM_ACK;

        ymodem_putchar(cmd); // 保存完成, 返回应答
        ym->packets++;
    }

    return ym_rec_timeout;
}

/**
 * @brief	ymodem用户收尾工作
 * @details
 * @param	None
 * @retval	None
 */
static ym_err_t ymodem_on_end(ymodem_t ym, uartx_rx_t rx)
{
    // uint16_t addr;
    //    uint8_t site;
    //    uint32_t check_sum = 0;

    if (NULL == ym || NULL == rx)
        return ym_err_other;

        // for (site = 0; site < 3U; ++site)
        // {
        //     IapProgram(ISP_ADDRESS - 3 + site, ym->jmp_code[site]); // 全部下载结束,最后写用户入口地址
        //     check_sum += IapRead(ISP_ADDRESS - 3 + site);
        // }

        // for (addr = 3U; addr < ym->file_size; ++addr)
        //     check_sum += IapRead(addr); // 计算FLASH累加和

        // #if (USE_PRINTF_DEBUG == 1)
        //     Uartx_Printf(OTA_INFO_OUT_UART, "\r\ncur_sum: ");
        //     Uartx_SendStr(OTA_INFO_OUT_UART, uint32_to_string(ym->check_sum), 8U, UART_BYTE_SENDOVERTIME);
        //     Uartx_Printf(OTA_INFO_OUT_UART, "\treal_sum: ");
        //     Uartx_SendStr(OTA_INFO_OUT_UART, uint32_to_string(check_sum), 8U, UART_BYTE_SENDOVERTIME);
        //     Uartx_Printf(OTA_INFO_OUT_UART, "\r\n");
        //     // Uartx_Printf(OTA_INFO_OUT_UART, "cur_sum: %#X, real_sum: %#X.\r\n",ym->check_sum,check_sum);
        // #endif

#if (USE_PRINTF_DEBUG == 1)
        // for (addr = 0; addr < 5U * 1024U; addr += 1024U)
        // {
        //     memset(Boot_Rx.Rx_buffer, 0x00, sizeof(Boot_Rx.Rx_buffer));
        //     Iap_Reads(addr, Boot_Rx.Rx_buffer, 1024U);
        //     display_hex_data(Boot_Rx.Rx_buffer, 1024U);
        // }
#endif

    // if (check_sum == ym->check_sum)
    //     return ym_ok; // 正确
    // else
    // {
    //     IapErase(ISP_ADDRESS - SECTOR_SIZE);
    //     return ym_program_err; // 写入错误
    // }

    IapWrites(USER_FLASH_SIZE, ym->jmp_code, sizeof(ym->jmp_code));

#if (USE_PRINTF_DEBUG == 1)
    // Iap_Reads(USER_FLASH_SIZE, ym->jmp_code, sizeof(ym->jmp_code));
    // Uartx_Printf(OTA_INFO_OUT_UART, "app loader addr: 0x%bX 0x%bX 0x%bX .\r\n",
    //              ym->jmp_code[0], ym->jmp_code[1], ym->jmp_code[2]);
#endif

    return ym_ok; // 正确
}

/**
 * @brief	ymodem收尾工作
 * @details
 * @param	None
 * @retval	None
 */
static ym_err_t ymodem_do_fin(ymodem_t ym, uartx_rx_t rx)
{
    uint32_t timeout = YM_RECV_TIMEOUT;
    ym_err_t cmd;

    if (NULL == ym || NULL == rx)
        return ym_err_other;

    if (ymodem_on_end(ym, rx) != ym_ok)
        return ym_err_other;

    ymodem_putchar(YM_NACK);
__wait:
    while (--timeout)
    {
        if (Uartx_RecivePackge() != ym_ok)
            return ym_recv_err;

        if (0 == RX_LEN())
            goto __wait;

        cmd = ymodem_check_data_packet(rx);
#if (USE_PRINTF_DEBUG == 1)
        Uartx_Printf(OTA_INFO_OUT_UART, "cmd: %bd.\r\n", cmd);
#endif

        if (YM_ACK == cmd) // EOT
        {
            ymodem_putchar(YM_ACK);
            ymodem_putchar(YM_PUT_C);
            timeout = YM_RECV_TIMEOUT;
            goto __wait;
        }

        if ((cmd != ym_ok) && (cmd != YM_NACK))
            return cmd;
        else
        {
            if ((SOH == RX_HEAD()) && 0 == RX_FRAME(PACKET_SEQNO_INDEX))
            {
                ymodem_putchar(YM_ACK); // 最后一次应答
                return ym_ok;
            }
        }
    }
    return ym_rec_timeout;
}

/**
 * @brief	按Ymodem接收文件数据并写入用户FLASH.
 * @version V1.0.0,2022/01/18
 * @details
 * @param	None
 * @retval	None
 */
ym_err_t Ymodem_Download(ymodem_t ym, uartx_rx_t rx)
{
    ym_err_t ym_res;

    if (NULL == ym || NULL == rx)
        return ym_err_other;

    Uartx_Printf(OTA_WORK_UART, "\r\n\r\n Waiting for the file to be sent ... (press 'a' to abort)\r\n");

    ym_res = ymodem_handshake(ym, rx);
    if (ym_res != ym_ok)
        return ym_res;
#if (USE_PRINTF_DEBUG == 1)
    Uartx_Printf(OTA_INFO_OUT_UART, "Handshake succeeded.\r\n");
#endif
    ym_res = ymodem_do_trans(ym, rx);
    if (ym_res != ym_ok)
        return ym_res;
#if (USE_PRINTF_DEBUG == 1)
    Uartx_Printf(OTA_INFO_OUT_UART, "Data frame reception completed.\r\n");
#endif
    return ymodem_do_fin(ym, rx);
}

/**
 * @brief	GPIO初始化
 * @details	对应功能引脚都需要初始化后使用
 * @param	None
 * @retval	None
 */
static void Gpio_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

#if EXTERNAL_CRYSTAL
    P_SW2 = 0x80;
    XOSCCR = 0xC0;
    /*启动外部晶振11.0592MHz*/
    while (!(XOSCCR & 1))
        ;
    /*时钟不分频*/
    CLKDIV = 0x01;
    /*选择外部晶振*/
    CKSEL = 0x01;
    P_SW2 = 0x00;
#endif
    /*手册提示，串口1、2、3、4的发送引脚必须设置为推挽输出*/
    P_SW1 = 0x00; /*串口1引脚使用P3.0、P3.1*/
    /*设置P0.7为推免输出*/
    GPIO_InitStruct.Mode = GPIO_OUT_PP;
    GPIO_InitStruct.Pin = GPIO_Pin_7;
    GPIO_IOInit(GPIO_P0, &GPIO_InitStruct);
    // /*设置P1.0为准双向口*/
    GPIO_InitStruct.Mode = GPIO_PullUp;
    GPIO_InitStruct.Pin = GPIO_Pin_0;
    GPIO_IOInit(GPIO_P1, &GPIO_InitStruct);
    /*设置P1.1为推免输出*/
    GPIO_InitStruct.Mode = GPIO_OUT_PP;
    GPIO_InitStruct.Pin = GPIO_Pin_1;
    GPIO_IOInit(GPIO_P1, &GPIO_InitStruct);
    // /*设置P1.0、P1.1为推免输出*/
    // GPIO_InitStruct.Mode = GPIO_OUT_PP;
    // GPIO_InitStruct.Pin = GPIO_Pin_0 | GPIO_Pin_1;
    // GPIO_IOInit(GPIO_P1, &GPIO_InitStruct);
    /*设置P3.0为准双向口*/
    GPIO_InitStruct.Mode = GPIO_PullUp;
    GPIO_InitStruct.Pin = GPIO_Pin_0;
    GPIO_IOInit(GPIO_P3, &GPIO_InitStruct);
    /*设置P3.1为推挽输出*/
    GPIO_InitStruct.Mode = GPIO_OUT_PP;
    GPIO_InitStruct.Pin = GPIO_Pin_1;
    GPIO_IOInit(GPIO_P3, &GPIO_InitStruct);
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
