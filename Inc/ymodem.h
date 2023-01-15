
#ifndef _YMODEM_H_
#define _YMODEM_H_

#define PACKET_SEQNO_INDEX 1
#define PACKET_SEQNO_COMP_INDEX 2

#define PACKET_HEADER 3
#define PACKET_TRAILER 2
#define PACKET_OVERHEAD (PACKET_HEADER + PACKET_TRAILER)
#define PACKET_SIZE 128
#define PACKET_1K_SIZE 1024
#define RE_PACKET_128B_SIZE (PACKET_SIZE + PACKET_OVERHEAD)
#define RE_PACKET_1K_SIZE (PACKET_1K_SIZE + PACKET_OVERHEAD)

#define FILE_NAME_LENGTH 32
#define FILE_SIZE_LENGTH 8

#define SOH 0x01    /* start of 128-byte data packet */
#define STX 0x02    /* start of 1024-byte data packet */
#define EOT 0x04    /* end of transmission */
#define ACK 0x06    /* acknowledge */
#define NAK 0x15    /* negative acknowledge */
#define CANCEL 0x18 /* two of these in succession aborts transfer */
#define CRC16 0x43  /* 'C' == 0x43, request 16-bit CRC */

#define ABORT1 'A' /* 'A' == 0x41, abort by user */
#define ABORT2 'a' /* 'a' == 0x61, abort by user */

/* how many CAN be sent when user active end the session. */
#ifndef RYM_END_SESSION_SEND_CAN_NUM
#define RYM_END_SESSION_SEND_CAN_NUM 0x07
#endif

#define YM_HANDSHAKE_TIMEOUT 200U // 2000U     /*握手超时时间*/
#define YM_RECV_TIMEOUT 100       //(30U * 20000U) /*接收超时时间:30s*/
#define NAK_TIMEOUT 10000
#define WAITTIMES 50000 // 300*5ms
#define TIMEOUTS 5000   // 5ms5300 1000
#define MAX_ERRORS 30

#pragma OT(0)
typedef enum
{
    ym_none = -1,
    ym_ok,
    ym_user_cancel,
    ym_pc_cancel,
    ym_file_size_large,
    ym_file_name_err,
    ym_program_err,
    ym_recv_err,
    ym_rec_timeout,
    ym_handshake_err,
    ym_err_other,
    /*应答指令*/
    YM_PUT_C,
    YM_ACK,
    YM_NACK,
    YM_CAN,
    ym_err_max,
} ym_err_t;

typedef struct ymodem_s *ymodem_t;
typedef struct ymodem_s ymodem;
struct ymodem_s
{
    uint8_t file_name[FILE_NAME_LENGTH];
    uint16_t file_size;
    volatile uint16_t next_flash_addr;
    uint16_t packets;
    // volatile uint32_t check_sum;
    volatile uint8_t jmp_code[3];
    uint8_t *buf;
    void *rb;
};

#pragma OT(9)
#endif

/*******************(C)COPYRIGHT 2013 STC *****END OF FILE****/
