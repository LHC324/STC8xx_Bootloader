#include "config.h"
#include "usart.h"

/**
 * @brief	把一个16位整型数转成十进制送串口发送.
 * @version V1.0.0,2022/01/18
 * @details
 * @param	num 要处理的16位整型数.
 * @retval	None
 */
// void U16_To_Dec(uint16_t num)
// {
//     uint8_t i;
//     uint8_t tmp[10];
//     for (i = 4; i < 5; i--)
//         tmp[i] = num % 10 + '0', num = num / 10;
//     for (i = 0; i < 4; i++)
//     {
//         if (tmp[i] != '0')
//             break;
//         tmp[i] = ' ';
//     }
//     for (i = 0; i < 5; i++)
//         Uart_PutByte(tmp[i]);
// }

/**
 * @brief	把一个32位长整型数转成十进制送串口发送.
 * @details
 * @param	num 要处理的32位整型数.
 * @retval	None
 * @version V1.0.0,2022/01/18
 */
// void U32_To_Dec(uint32_t num)
// {
//     uint8_t i, k;
//     uint8_t tmp[10];
//     for (i = 8; i > 0; i--)
//     {
//         k = ((uint8_t)num) & 0x0f;
//         if (k <= 9)
//             tmp[i] = k + '0';
//         else
//             tmp[i] = k - 10 + 'A';
//         num >>= 4;
//     }
//     for (i = 1; i < 9; i++)
//         Uart_PutByte(tmp[i]);
// }

// uint8_t *uint32_to_string(uint32_t num)
// {
//     uint8_t i, k;
//     static uint8_t tmp[10];
//     for (i = 8; i > 0; i--)
//     {
//         k = ((uint8_t)num) & 0x0f;
//         if (k <= 9)
//             tmp[i] = k + '0';
//         else
//             tmp[i] = k - 10 + 'A';
//         num >>= 4;
//     }
//     return tmp;
//     // for (i = 1; i < 9; i++)
//     //     Uart_PutByte(tmp[i]);
// }

/**
 * @brief	字符串转整型.
 * @details
 * @param	inputstr 字符串指针.
 * @retval	16位整型数.
 * @version V1.0.0,2022/01/18
 */
uint16_t Str_To_Int(uint8_t *inputstr)
{
    uint16_t val = 0;
    uint8_t i;

    for (i = 0; i < 5; i++)
    {
        if ((inputstr[i] < '0') || (inputstr[i] > '9'))
            break;
        val = val * 10 + inputstr[i] - '0';
    }
    return val;
}
