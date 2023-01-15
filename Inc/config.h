#ifndef __CONFIG_H_
#define __CONFIG_H_

#include <STC8.H>
#include <stdio.h>
#include <intrins.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <absacc.h> //可直接操作内存地址
#include <stdarg.h>

/**********************布尔变量定义**********************/
#define true 1
#define false 0
/**********************布尔变量定义**********************/

/***********************************API配置接口***********************************/
/*使用仿真模式*/
#define USING_SIMULATE
/*EEPROM使用MOVC指令*/
#define EEPROM_USING_MOVC 1
/*使用自动下载功能*/
#define UAING_AUTO_DOWNLOAD 1
/*flash用做EEPROM尺寸*/
#define EEPROM_SIZE() (64U * 1024U)
/*内部falsh开始地址*/
#define FLASH_START_ADDR 0x0000
/*定义IAP编程地址*/
#define ISP_PROGRAM_ADDR 0xFA00
/*定义OTA目标串口*/
#define OTA_UART 5
/*使用外部晶振*/
#define EXTERNAL_CRYSTAL 0
/*调试是否启用串口*/
#if defined USING_SIMULATE
#define USE_PRINTF_DEBUG 0
#else
#define USE_PRINTF_DEBUG 1
#endif
/*启用环形缓冲区*/
#define DWIN_USING_RB 0
/*调试选项*/
#define DEBUGGING 1
/*迪文屏幕使用CRC校验*/
#define USING_CRC 1
/*当前版本*/
#define SOFT_VERSION "1.0.5"
/*是否显示logo*/
#define BOOT_SHOW_INFO 1
/*是否使能硬件看门狗*/
#define WDT_ENABLE 0
/*BootLoader 版本号*/
#define BOOTLOADER_VERSION "1.0.0"
/*ISP开始地址, 高地址必须是偶数, 注意要预留ISP空间,本例程留10K*/
#define ISP_ADDRESS 0xC800U
/*用户FLASH长度, 保留个字节存放用户地址程序的跳转地址*/
#define USER_FLASH_ADDR (ISP_ADDRESS - 3U)
#define APP_JMP_ADDR (ISP_ADDRESS - 2U)
/*OTA升级标志*/
#define OTA_FLAG_ADDR 0x10000 //(0xFC00 + 0x200U)
#define USER_FILE_SIZE_ADDR (OTA_FLAG_ADDR + 1U)
#define OTA_FLAG_VALUE 0x5AA5

/*扇区大小512B*/
#define SECTOR_SIZE 0x200
/*长跳转操作符代号*/
#define LJMP 0x02
/*跳转地址字节长度*/
#define LJMP_SIZE 0x03
/*定义WIFI模块相关引脚*/
#define WIFI_RESET P23
#define WIFI_RELOAD P20

#define COUNTMAX 65536U

/*消除编译器未使用变量警告*/
#define UNUSED_VARIABLE(x) ((void)(x))
#define UNUSED_PARAMETER(x) UNUSED_VARIABLE(x)

//(1/FOSC)*count =times(us)->count = time*FOSC/1000(ms)
#define FOSC 24000000UL
/*10ms(时钟频率越高，所能产生的时间越小)*/
#define TIMES 10U
/*定时器模式选择*/
#define TIMER_MODE 12U
/*定时器分频系数，默认为一分频*/
#define TIME_DIV 1U
#define T12_MODE (TIMES * FOSC / 1000 / 12 / TIME_DIV)
#define T1_MODE (TIMES * FOSC / 1000 / TIME_DIV)
#define TIMERS_OVERFLOW (COUNTMAX * 1000 * TIMER_MODE * TIME_DIV) / FOSC

#define OPEN_GLOBAL_OUTAGE() (EA = 1 << 0)
#define CLOSE_GLOBAL_OUTAGE() (EA = 0 << 0)
/*判断延时数是否超出硬件允许范围*/
#if (TIMES > TIMERS_OVERFLOW)
#error The timer cannot generate the current duration!
#endif
/***********************************API配置接口***********************************/

/***********************************常用的数据类型***********************************/
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short int uint16_t;
typedef int int16_t;
typedef unsigned long uint32_t;
typedef signed long int32_t;
typedef volatile __IO;
typedef bit bool;

/***********************************常用的数据类型***********************************/

#define LED_R P10 /*RGB红色LED用IO口P10*/
#define LED_G P11 /*RGB绿色LED用IO口P11*/
#define LED_B P07 /*RGB蓝色LED用IO口P07*/
/***********************************函数声明***********************************/
// static void Gpio_Init();
static void Reset_Registers(void);
extern void U16_To_Dec(uint16_t num);
extern void U32_To_Dec(uint32_t num);
extern uint16_t Str_To_Int(uint8_t *inputstr);
/***********************************函数声明***********************************/
#endif