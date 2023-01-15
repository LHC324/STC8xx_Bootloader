#include "usart.h"
#include "GPIO.h"
#if (DWIN_USING_RB)
#include "utils_ringbuffer.h"
#endif
/*********************************************************
* 函数名：
* 功能：
* 参数：
* 作者：
* note：
        同时使用多个串口的时候会出现数据传输错误的情况 建议在使用该板子与其他
        通讯模块建立通讯的时候使用1对1的建立连接的模式

        解决了多串口同时数据传输错误问题 //2021/5/31

        在切换串口的引脚输入时，建议将RX端初始化的时候给个0值 TX引脚手动给个1值
        （基于STC单片机的特性）

**********************************************************/

#if (OTA_UART != 5U)
void (*IspProgram)(void) = ISP_PROGRAM_ADDR;
char cnt7f = 0;
#else
// Uartx_HandleTypeDef Boot_Rx = {0};
// uint8_t uart2_buf[1024];
#if (DWIN_USING_RB)
struct ringbuffer rm_rb = {
    Boot_Rx.Rx_buffer,
    sizeof(Boot_Rx.Rx_buffer) - 1U,
    0,
    0,
};
#endif
#endif

Uart_HandleTypeDef Uart1; // 串口1句柄
Uart_HandleTypeDef Uart2; // 串口2句柄
// Uart_HandleTypeDef Uart3; // 串口3句柄
// Uart_HandleTypeDef Uart4; // 串口4句柄

#if !defined USING_SIMULATE
/*********************************************************
 * 函数名：void Uart_1Init(void)
 * 功能：  串口1的初始化
 * 参数：
 * 作者：  LHC
 * note：
 *		使用的是定时器1作为波特率发生器,LAN口用
 **********************************************************/
void Uart1_Init(void) // 串口1选择定时器1作为波特率发生器
{
    Uart1.Instance = UART1;
    Uart1.Register_SCON = 0x50; // 模式1，8位数据，可变波特率
    Uart1.Uart_Mode = 0x00;     // 定时器模式0，16bit自动重载
    Uart1.Uart_Count = BRT_1T(BAUD_115200);
    Uart1.RunUart_Enable = true;
    Uart1.Interrupt_Enable = true;
    Uart1.Register_AUXR = 0x40;  // 定时器1，1T模式
    Uart1.Register_AUXR &= 0xFE; // 波特率发生器选用定时器1，最好按照要求来

    Uart1.Uart_NVIC.Register_IP = 0xEF; // PS=0,PSH=0,串口1中断优先级为第0级，最低级
    Uart1.Uart_NVIC.Register_IPH = 0xEF;

    Uart_Base_MspInit(&Uart1);
}

#if (UAING_AUTO_DOWNLOAD)
/**
 * @brief    软件复位自动下载功能，需要在串口中断里调用，
 *           需要在STC-ISP助手里设置下载口令：10个0x7F。
 * @details  Software reset automatic download function,
 *			 need to be called in serial interrupt,
 *			 The download password needs to be
 *			 set in the STC-ISP assistant: 10 0x7F.
 * @param    None.
 * @return   None.
 **/
void Auto_RST_download(void)
{
    static uint8_t semCont = 0;
    if (SBUF == 0x7F || SBUF == 0x80)
    {
        if (++semCont >= 10)
        {
            semCont = 0;
            IAP_CONTR = 0x60;
        }
    }
    else
    {
        semCont = 0;
    }
}
#endif

// #if (OTA_UART != 5U)
/*********************************************************
 * 函数名：void Uart1_ISR() interrupt 4 using 2
 * 功能：  串口1的定时中断服务函数
 * 参数：
 * 作者：  LHC
 * note：
 *		使用的是定时器2作为波特率发生器,485口用
 **********************************************************/
void Uart1_ISR() interrupt 4 using 2 // 串口1的定时中断服务函数
{
    if (TI) // 发送中断标志
    {
        TI = 0;
        Uart1.Uartx_busy = false; // 发送完成，清除占用
    }

    if (RI) // 接收中断标志
    {
#if (OTA_UART == 1)
        if (SBUF == 0x7F)
        {
            cnt7f++;
            if (cnt7f >= 16)
            {
                IspProgram();
            }
        }
        else
        {
            cnt7f = 0;
        }
#endif

        RI = 0;
#if (UAING_AUTO_DOWNLOAD)
        Auto_RST_download();
#endif
    }
}
#endif

/*********************************************************
 * 函数名：void Uart_2Init(void)
 * 功能：  串口2的初始化
 * 参数：
 * 作者：  LHC
 * note：
 *		使用的是定时器2作为波特率发生器,485口用
 **********************************************************/
void Uart2_Init(void) // 串口2选择定时器2作为波特率发生器
{
    Uart2.Instance = UART2;
    Uart2.Register_SCON = 0x10; // 模式1，8位数据，可变波特率，开启串口2接收
    Uart2.Uart_Mode = 0x00;     // 定时器模式0，16bit自动重载
    Uart2.Uart_Count = BRT_1T(BAUD_115200);
    Uart2.RunUart_Enable = true;
    Uart2.Interrupt_Enable = 0x01;
    Uart2.Register_AUXR = 0x14;         // 开启定时器2，1T模式
    Uart2.Uart_NVIC.Register_IP = 0x01; // PS2=1,PS2H=0,串口2中断优先级为第1级
    Uart2.Uart_NVIC.Register_IPH = 0xFE;

    Uart_Base_MspInit(&Uart2);
}

// #if (OTA_UART != 5U)
/*********************************************************
 * 函数名：void Uart2_ISR() interrupt 8 using 2
 * 功能：  串口2中断函数
 * 参数：
 * 作者：  LHC
 * note：
 *		使用的是定时器2作为波特率发生器,4G口用
 **********************************************************/
// void Uart2_ISR() interrupt 8 using 2
// {
//     // static uint16_t i = 0;
//     // static uint16_t site = 0;
// #if (DWIN_USING_RB)
//     struct ringbuffer *const rb = &rm_rb;
// #endif
//     if (S2CON & S2TI) // 发送中断
//     {
//         S2CON &= ~S2TI;
//         Uart2.Uartx_busy = false; // 发送完成，清除占用
//     }

//     if (S2CON & S2RI) // 接收中断
//     {
//         S2CON &= ~S2RI;
// #if (DWIN_USING_RB)
//         // if (Uart4.pbuf && Uart4.rx_count < rx_size)
//         //     Uart4.pbuf[Uart4.rx_count++] = S4BUF;
//         if (NULL == rb->buf)
//             return;

//         // rb->buf[rb->write_index & rb->size] = S2BUF;
//         // site = rb->write_index % rb->size;
//         // uart2_buf[site] = S2BUF;
//         // if (fingbuffer_get_num(rb) > 128)
//         //     while (1)
//         //         ;

//         rb->buf[rb->write_index & rb->size] = S2BUF;
//         /*
//          * buffer full strategy: new data will overwrite the oldest data in
//          * the buffer
//          */
//         if ((rb->write_index - rb->read_index) > rb->size)
//         {
//             rb->read_index = rb->write_index - rb->size;
//         }

//         rb->write_index++;

//         // if (fingbuffer_get_num(rb) > 128)
//         //     while (1)
//         //         ;
// #endif

// #if (OTA_UART == 2)
//         if (S2BUF == 0x7F)
//         {
//             cnt7f++;
//             if (cnt7f >= 16)
//             {
//                 IspProgram();
//             }
//         }
//         else
//         {
//             cnt7f = 0;
//         }
// #endif
//     }
// }
// #endif

/**********************************公用函数************************/

/*********************************************************
 * 函数名：Uart_Base_MspInit(Uart_HandleTypeDef *uart_baseHandle)
 * 功能：  所有串口底层初始化函数
 * 参数：  Uart_HandleTypeDef *uart_baseHandle串口句柄
 * 作者：  LHC
 * note：
 *		注意正确给出串口句柄
 **********************************************************/
void Uart_Base_MspInit(Uart_HandleTypeDef *const uart_baseHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    switch (uart_baseHandle->Instance)
    {
    case UART1:
    {
        SCON = uart_baseHandle->Register_SCON;
        TMOD |= uart_baseHandle->Uart_Mode;
        TL1 = uart_baseHandle->Uart_Count;
        TH1 = uart_baseHandle->Uart_Count >> 8;
        TR1 = uart_baseHandle->RunUart_Enable;
        AUXR |= uart_baseHandle->Register_AUXR;
        IP &= uart_baseHandle->Uart_NVIC.Register_IP;
        IPH &= uart_baseHandle->Uart_NVIC.Register_IPH;
#if USEING_PRINTF // 如果使用printf
        TI = 1;   // 放到printf重定向
#else
        ES = uart_baseHandle->Interrupt_Enable; // 串口1中断允许位
#endif
        /*设置P3.0为准双向口*/
        GPIO_InitStruct.Mode = GPIO_PullUp;
        GPIO_InitStruct.Pin = GPIO_Pin_0;
        GPIO_Inilize(GPIO_P3, &GPIO_InitStruct);

        /*设置P3.1为推挽输出*/
        GPIO_InitStruct.Mode = GPIO_OUT_PP;
        GPIO_InitStruct.Pin = GPIO_Pin_1;
        GPIO_Inilize(GPIO_P3, &GPIO_InitStruct);
    }
    break;
    case UART2:
    {
        S2CON = uart_baseHandle->Register_SCON;
        TL2 = uart_baseHandle->Uart_Count;
        TH2 = uart_baseHandle->Uart_Count >> 8;
        AUXR |= uart_baseHandle->Register_AUXR;
        IE2 = (uart_baseHandle->Interrupt_Enable & 0x01); // 串口2中断允许位
        IP2 &= uart_baseHandle->Uart_NVIC.Register_IP;
        IP2H &= uart_baseHandle->Uart_NVIC.Register_IPH;
        /*设置P1.0为准双向口*/
        GPIO_InitStruct.Mode = GPIO_PullUp;
        GPIO_InitStruct.Pin = GPIO_Pin_0;
        GPIO_Inilize(GPIO_P1, &GPIO_InitStruct);

        /*设置P1.1为推挽输出*/
        GPIO_InitStruct.Mode = GPIO_OUT_PP;
        GPIO_InitStruct.Pin = GPIO_Pin_1;
        GPIO_Inilize(GPIO_P1, &GPIO_InitStruct);
    }
    break;
    // case UART3:
    // {
    //     S3CON = uart_baseHandle->Register_SCON;
    //     T4T3M = uart_baseHandle->Uart_Mode;
    //     T3L = uart_baseHandle->Uart_Count;
    //     T3H = uart_baseHandle->Uart_Count >> 8;
    //     IE2 |= (uart_baseHandle->Interrupt_Enable & 0x08); // 串口3中断允许位

    //     /*设置P0.0为准双向口*/
    //     GPIO_InitStruct.Mode = GPIO_PullUp;
    //     GPIO_InitStruct.Pin = GPIO_Pin_0;
    //     GPIO_Inilize(GPIO_P0, &GPIO_InitStruct);

    //     // GPIO_InitStruct.Mode = GPIO_OUT_OD;
    //     // GPIO_InitStruct.Pin = GPIO_Pin_0;
    //     // GPIO_Inilize(GPIO_P0, &GPIO_InitStruct);

    //     /*设置P0.1为推挽输出*/
    //     GPIO_InitStruct.Mode = GPIO_OUT_PP;
    //     GPIO_InitStruct.Pin = GPIO_Pin_1;
    //     GPIO_Inilize(GPIO_P0, &GPIO_InitStruct);
    // }
    // break;
    // case UART4:
    // {
    //     S4CON = uart_baseHandle->Register_SCON;
    //     T4T3M |= uart_baseHandle->Uart_Mode; // 此处串口3和串口4共用
    //     T4L = uart_baseHandle->Uart_Count;
    //     T4H = uart_baseHandle->Uart_Count >> 8;
    //     IE2 |= (uart_baseHandle->Interrupt_Enable & 0x10); // 串口4中断允许位

    //     /*设置P0.2为准双向口*/
    //     GPIO_InitStruct.Mode = GPIO_PullUp;
    //     GPIO_InitStruct.Pin = GPIO_Pin_2;
    //     GPIO_Inilize(GPIO_P0, &GPIO_InitStruct);

    //     /*设置P0.3为推挽输出*/
    //     GPIO_InitStruct.Mode = GPIO_OUT_PP;
    //     GPIO_InitStruct.Pin = GPIO_Pin_3;
    //     GPIO_Inilize(GPIO_P0, &GPIO_InitStruct);
    // }
    // break;
    default:
        break;
    }
}

#if (DWIN_USING_RB)
/*********************************************************
 * 函数名：static void Busy_Await(Uart_HandleTypeDef * const Uart, uint16_t overtime)
 * 功能：  字节发送超时等待机制
 * 参数：  Uart_HandleTypeDef * const Uart;uint16_t overtime
 * 作者：  LHC
 * note：
 *
 **********************************************************/
void Busy_Await(Uart_HandleTypeDef *const Uart, uint16_t overtime)
{

    while (Uart->Uartx_busy) // 等待发送完成：Uart->Uartx_busy清零
    {
        if (!(overtime--))
            break;
    }

    Uart->Uartx_busy = true; // 发送数据，把相应串口置忙
}
#endif

/*********************************************************
 * 函数名：Uartx_SendStr(Uart_HandleTypeDef *const Uart,uint8_t *p,uint8_t length)
 * 功能：  所有串口字符串发送函数
 * 参数：  Uart_HandleTypeDef *const Uart,uint8_t *p;uint8_t length
 * 作者：  LHC
 * note：
 *
 **********************************************************/
void Uartx_SendStr(Uart_HandleTypeDef *const Uart,
                   uint8_t *p,
                   uint8_t length,
                   uint16_t time_out)
{
    if (NULL == Uart || !length)
        return;

    while (length--)
    {
#if (DWIN_USING_RB)
        Busy_Await(&(*Uart), time_out); // 等待当前字节发送完成
        switch (Uart->Instance)
        {
        case UART1:
            SBUF = *p++;
            break;
        case UART2:
            S2BUF = *p++;
            break;
        case UART3:
            S3BUF = *p++;
            break;
        case UART4:
            S4BUF = *p++;
            break;
        default:
            break;
        }
#else
#define S1BUF SBUF
#define S1CON TI
#define S1TI 1
#define UARTX_TX(_id)                                       \
    do                                                      \
    {                                                       \
        S##_id##BUF = *p++;                                 \
        while (!(S##_id##CON & S##_id##TI) && (--time_out)) \
            ;                                               \
        S##_id##CON = (S##_id##CON & ~S##_id##TI);          \
    } while (0)

        switch (Uart->Instance)
        {
        case UART1:
            // TI = 0;
            UARTX_TX(1);
            break;
        case UART2:
            UARTX_TX(2);
            break;
        case UART3:
            UARTX_TX(3);
            break;
        case UART4:
            UARTX_TX(4);
            break;
        default:
            break;
        }
#undef S1CON
#undef S1TI
#undef S1BUF
#endif
    }
}

// #if !defined USING_SIMULATE
/*********************************************************
 * 函数名：char putchar(char str)
 * 功能：  putchar重定向,被printf调用
 * 参数：  char str，发送的字符串
 * 作者：  LHC
 * note：
 *		  使用printf函数将会占用1K 左右FLASH
 **********************************************************/
// static char idata buf[128] = {0};
void Uartx_Printf(Uart_HandleTypeDef *const uart, const char *format, ...)
{
    uint16_t length = 0;
    va_list ap;

    char idata buf[128] = {0};
    // memset(buf, 0x00, sizeof(buf));
    va_start(ap, format);
    /*使用可变参数的字符串打印,类似sprintf*/
    length = vsprintf(buf, format, ap);
    va_end(ap);
    if (uart)
        Uartx_SendStr(uart, (uint8_t *)buf, length, UART_BYTE_SENDOVERTIME);
}
// #endif
/**********************************公用函数************************/
