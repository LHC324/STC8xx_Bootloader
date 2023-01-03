#include "eeprom.h"

// #pragma OT(0)
/**
 * @brief	配置STC8单片机EEPROM的等待时间
 * @version V1.0.0,2022/01/18
 * @author  LHC
 * @details
 * @param	None
 * @retval	None
 */
static void IapConfigWaitTime()
{
	uint32_t XTAL = FOSC / 1000000;

	if (XTAL >= 30) //>=30M
		IAP_CONTR = WT_30M;
	else if (XTAL >= 24)
		IAP_CONTR = WT_24M;
	else if (XTAL >= 20)
		IAP_CONTR = WT_20M;
	else if (XTAL >= 12)
		IAP_CONTR = WT_12M;
	else if (XTAL >= 6)
		IAP_CONTR = WT_6M;
	else if (XTAL >= 3)
		IAP_CONTR = WT_3M;
	else if (XTAL >= 2)
		IAP_CONTR = WT_2M;
	else
		IAP_CONTR = WT_1M;
}

/**
 * @brief	关闭iap功能
 * @version V1.0.0,2022/01/18
 * @author  LHC
 * @details
 * @param	None
 * @retval	None
 */
void IapIdle()
{
	IAP_CONTR = 0x00; // 关闭IAP功能
	IAP_CMD = 0x00;	  // 清除命令寄存器
	IAP_TRIG = 0x00;  // 清除触发寄存器
	IAP_ADDRH = 0xFF; // 将地址设置到非IAP区域
	IAP_ADDRL = 0xFF;
}

/*********************************************************
 * 函数名：char IapRead(unsigned short addr)
 * 功能：  读函数  读出一个字节
 * 参数：
 * 作者：
 * note：
 *
 **********************************************************/
char IapRead(unsigned short addr)
{
#if (1 == EEPROM_USING_MOVC)
	addr += IAP_OFFSET;					  // 使用 MOVC 读取 EEPROM 需要加上相应的偏移
	return *(unsigned char code *)(addr); // 使用 MOVC 读取数据
#else
	char dat = '\0';

	IapConfigWaitTime();
	IAP_CMD = 0x01; // EEPROM读命令
	IAP_ADDRL = addr;
	IAP_ADDRH = addr >> 8U;
	IAP_TRIG = 0x5A; // 触发指令
	IAP_TRIG = 0xA5;
	dat = IAP_DATA;

	if (IAP_CONTR & 0x10)
	{
		IAP_CONTR &= 0xEF;
		return 0xA5;
	}
	else
		return dat;
#endif
}

/*********************************************************
 * 函数名：void IapProgram(unsigned short addr, char dat)
 * 功能：  指定地址写数据
 * 参数：
 * 作者：
 * note：
 *
 **********************************************************/
void IapProgram(uint16_t addr, uint8_t dat)
{
	IapConfigWaitTime();
	IAP_CMD = 0x02;			// 设置IAP写命令
	IAP_ADDRL = addr;		// 设置IAP低地址
	IAP_ADDRH = addr >> 8U; // 设置IAP高地址
	IAP_DATA = dat;			// 写IAP数据
	IAP_TRIG = 0x5a;		// 写触发命令(0x5a)
	IAP_TRIG = 0xa5;		// 写触发命令(0xa5)
	//_nop_();
	//	Delay_ms(5);//延时5毫秒
	IapIdle(); // 关闭IAP功能
}

/*********************************************************
 * 函数名：void IapErase(int addr)
 * 功能：  擦除指定地址的内容
 * 参数：
 * 作者：
 * note：擦除每次按照512B进行，仅提供首地址即可
 *
 **********************************************************/
void IapErase(uint16_t addr)
{ // 使能IAP
	IapConfigWaitTime();
	IAP_CMD = 0x03;			// 设置IAP擦除命令
	IAP_ADDRL = addr;		// 设置IAP低地址
	IAP_ADDRH = addr >> 8U; // 设置IAP高地址
	IAP_TRIG = 0x5a;		// 写触发命令(0x5a)
	IAP_TRIG = 0xa5;		// 写触发命令(0xa5)
	// _nop_();						//
	IapIdle(); // 关闭IAP功能
			   //  Delay_ms(4); //延时5毫秒
}

/*********************************************************
 * 函数名：void EEPROM_writeString(unsigned short Address,unsigned char *Pdata,unsigned short length);
 * 功能：  指定地址 写入数据
 * 参数：
 * 作者：
 * note：写数据一般请按照一个扇区一个扇区的来写，不然在擦除数据的时候会造成数据丢失
 * 一个扇区的大小是 512字节
 *
 *
 *
 ***********************************************************************************/
void IapWrites(uint16_t addr, const uint8_t *str, uint16_t len)
{
	if (str)
	{
		IapErase(addr);
		while (len--)
		{
			IapProgram(addr, *str);
			if (IapRead(addr) != *str)
				break;
			addr++, str++;
		}
	}
}

/**
 * @brief	从指定地址读取多个字节数据
 * @version V1.0.0,2022/01/18
 * @author  LHC
 * @details
 * @param	addr:开始地址, *Str: 指向目标缓冲区指针, Len:长度
 * @retval	None
 */
void Iap_Reads(uint16_t addr, uint8_t *str, uint16_t len)
{
	if (str)
	{
		while (len--)
		{
			*str++ = IapRead(addr++);
		}
	}
}
