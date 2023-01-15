
/*************	功能说明	**************

本文件为STC8xxx系列的端口初始化程序,用户几乎可以不修改这个程序.


******************************************/

#include "GPIO.h"

/**
 * @brief	初始化IO口
 * @details	对应功能引脚都需要初始化后使用.
 * @param	GPIOx: 结构参数,请参考timer.h里的定义.
 * @retval	成功返回0, 空操作返回1,错误返回2.
 * @version V2.0,2022/01/15
 */
uint8_t GPIO_Inilize(uint8_t GPIO, GPIO_InitTypeDef *GPIOx)
{
	if (GPIO > GPIO_P5)
		return 1; // 空操作
	if (GPIOx->Mode > GPIO_OUT_PP)
		return 2; // 错误

	switch (GPIO)
	{
	case GPIO_P0:
	{
		if (GPIOx->Mode == GPIO_PullUp)
			P0M1 &= ~GPIOx->Pin, P0M0 &= ~GPIOx->Pin; // 上拉准双向口
		if (GPIOx->Mode == GPIO_HighZ)
			P0M1 |= GPIOx->Pin, P0M0 &= ~GPIOx->Pin; // 浮空输入
		if (GPIOx->Mode == GPIO_OUT_OD)
			P0M1 |= GPIOx->Pin, P0M0 |= GPIOx->Pin; // 开漏输出
		if (GPIOx->Mode == GPIO_OUT_PP)
			P0M1 &= ~GPIOx->Pin, P0M0 |= GPIOx->Pin; // 推挽输出
	}
	break;
	case GPIO_P1:
	{
		if (GPIOx->Mode == GPIO_PullUp)
			P1M1 &= ~GPIOx->Pin, P1M0 &= ~GPIOx->Pin; // 上拉准双向口
		if (GPIOx->Mode == GPIO_HighZ)
			P1M1 |= GPIOx->Pin, P1M0 &= ~GPIOx->Pin; // 浮空输入
		if (GPIOx->Mode == GPIO_OUT_OD)
			P1M1 |= GPIOx->Pin, P1M0 |= GPIOx->Pin; // 开漏输出
		if (GPIOx->Mode == GPIO_OUT_PP)
			P1M1 &= ~GPIOx->Pin, P1M0 |= GPIOx->Pin; // 推挽输出
	}
	break;
	case GPIO_P2:
	{
		if (GPIOx->Mode == GPIO_PullUp)
			P2M1 &= ~GPIOx->Pin, P2M0 &= ~GPIOx->Pin; // 上拉准双向口
		if (GPIOx->Mode == GPIO_HighZ)
			P2M1 |= GPIOx->Pin, P2M0 &= ~GPIOx->Pin; // 浮空输入
		if (GPIOx->Mode == GPIO_OUT_OD)
			P2M1 |= GPIOx->Pin, P2M0 |= GPIOx->Pin; // 开漏输出
		if (GPIOx->Mode == GPIO_OUT_PP)
			P2M1 &= ~GPIOx->Pin, P2M0 |= GPIOx->Pin; // 推挽输出
	}
	break;
	case GPIO_P3:
	{
		if (GPIOx->Mode == GPIO_PullUp)
			P3M1 &= ~GPIOx->Pin, P3M0 &= ~GPIOx->Pin; // 上拉准双向口
		if (GPIOx->Mode == GPIO_HighZ)
			P3M1 |= GPIOx->Pin, P3M0 &= ~GPIOx->Pin; // 浮空输入
		if (GPIOx->Mode == GPIO_OUT_OD)
			P3M1 |= GPIOx->Pin, P3M0 |= GPIOx->Pin; // 开漏输出
		if (GPIOx->Mode == GPIO_OUT_PP)
			P3M1 &= ~GPIOx->Pin, P3M0 |= GPIOx->Pin; // 推挽输出
	}
	break;
	case GPIO_P4:
	{
		if (GPIOx->Mode == GPIO_PullUp)
			P4M1 &= ~GPIOx->Pin, P4M0 &= ~GPIOx->Pin; // 上拉准双向口
		if (GPIOx->Mode == GPIO_HighZ)
			P4M1 |= GPIOx->Pin, P4M0 &= ~GPIOx->Pin; // 浮空输入
		if (GPIOx->Mode == GPIO_OUT_OD)
			P4M1 |= GPIOx->Pin, P4M0 |= GPIOx->Pin; // 开漏输出
		if (GPIOx->Mode == GPIO_OUT_PP)
			P4M1 &= ~GPIOx->Pin, P4M0 |= GPIOx->Pin; // 推挽输出
	}
	break;
	case GPIO_P5:
	{
		if (GPIOx->Mode == GPIO_PullUp)
			P5M1 &= ~GPIOx->Pin, P5M0 &= ~GPIOx->Pin; // 上拉准双向口
		if (GPIOx->Mode == GPIO_HighZ)
			P5M1 |= GPIOx->Pin, P5M0 &= ~GPIOx->Pin; // 浮空输入
		if (GPIOx->Mode == GPIO_OUT_OD)
			P5M1 |= GPIOx->Pin, P5M0 |= GPIOx->Pin; // 开漏输出
		if (GPIOx->Mode == GPIO_OUT_PP)
			P5M1 &= ~GPIOx->Pin, P5M0 |= GPIOx->Pin; // 推挽输出
	}
	default:
		break;
	}
	return 0; // 成功

	// if(GPIO == GPIO_P0)
	// {
	// 	if(GPIOx->Mode == GPIO_PullUp)		P0M1 &= ~GPIOx->Pin,	P0M0 &= ~GPIOx->Pin;	 //上拉准双向口
	// 	if(GPIOx->Mode == GPIO_HighZ)		P0M1 |=  GPIOx->Pin,	P0M0 &= ~GPIOx->Pin;	 //浮空输入
	// 	if(GPIOx->Mode == GPIO_OUT_OD)		P0M1 |=  GPIOx->Pin,	P0M0 |=  GPIOx->Pin;	 //开漏输出
	// 	if(GPIOx->Mode == GPIO_OUT_PP)		P0M1 &= ~GPIOx->Pin,	P0M0 |=  GPIOx->Pin;	 //推挽输出
	// }
	// if(GPIO == GPIO_P1)
	// {
	// 	if(GPIOx->Mode == GPIO_PullUp)		P1M1 &= ~GPIOx->Pin,	P1M0 &= ~GPIOx->Pin;	 //上拉准双向口
	// 	if(GPIOx->Mode == GPIO_HighZ)		P1M1 |=  GPIOx->Pin,	P1M0 &= ~GPIOx->Pin;	 //浮空输入
	// 	if(GPIOx->Mode == GPIO_OUT_OD)		P1M1 |=  GPIOx->Pin,	P1M0 |=  GPIOx->Pin;	 //开漏输出
	// 	if(GPIOx->Mode == GPIO_OUT_PP)		P1M1 &= ~GPIOx->Pin,	P1M0 |=  GPIOx->Pin;	 //推挽输出
	// }
	// if(GPIO == GPIO_P2)
	// {
	// 	if(GPIOx->Mode == GPIO_PullUp)		P2M1 &= ~GPIOx->Pin,	P2M0 &= ~GPIOx->Pin;	 //上拉准双向口
	// 	if(GPIOx->Mode == GPIO_HighZ)		P2M1 |=  GPIOx->Pin,	P2M0 &= ~GPIOx->Pin;	 //浮空输入
	// 	if(GPIOx->Mode == GPIO_OUT_OD)		P2M1 |=  GPIOx->Pin,	P2M0 |=  GPIOx->Pin;	 //开漏输出
	// 	if(GPIOx->Mode == GPIO_OUT_PP)		P2M1 &= ~GPIOx->Pin,	P2M0 |=  GPIOx->Pin;	 //推挽输出
	// }
	// if(GPIO == GPIO_P3)
	// {
	// 	if(GPIOx->Mode == GPIO_PullUp)		P3M1 &= ~GPIOx->Pin,	P3M0 &= ~GPIOx->Pin;	 //上拉准双向口
	// 	if(GPIOx->Mode == GPIO_HighZ)		P3M1 |=  GPIOx->Pin,	P3M0 &= ~GPIOx->Pin;	 //浮空输入
	// 	if(GPIOx->Mode == GPIO_OUT_OD)		P3M1 |=  GPIOx->Pin,	P3M0 |=  GPIOx->Pin;	 //开漏输出
	// 	if(GPIOx->Mode == GPIO_OUT_PP)		P3M1 &= ~GPIOx->Pin,	P3M0 |=  GPIOx->Pin;	 //推挽输出
	// }
	// if(GPIO == GPIO_P4)
	// {
	// 	if(GPIOx->Mode == GPIO_PullUp)		P4M1 &= ~GPIOx->Pin,	P4M0 &= ~GPIOx->Pin;	 //上拉准双向口
	// 	if(GPIOx->Mode == GPIO_HighZ)		P4M1 |=  GPIOx->Pin,	P4M0 &= ~GPIOx->Pin;	 //浮空输入
	// 	if(GPIOx->Mode == GPIO_OUT_OD)		P4M1 |=  GPIOx->Pin,	P4M0 |=  GPIOx->Pin;	 //开漏输出
	// 	if(GPIOx->Mode == GPIO_OUT_PP)		P4M1 &= ~GPIOx->Pin,	P4M0 |=  GPIOx->Pin;	 //推挽输出
	// }
	// if(GPIO == GPIO_P5)
	// {
	// 	if(GPIOx->Mode == GPIO_PullUp)		P5M1 &= ~GPIOx->Pin,	P5M0 &= ~GPIOx->Pin;	 //上拉准双向口
	// 	if(GPIOx->Mode == GPIO_HighZ)		P5M1 |=  GPIOx->Pin,	P5M0 &= ~GPIOx->Pin;	 //浮空输入
	// 	if(GPIOx->Mode == GPIO_OUT_OD)		P5M1 |=  GPIOx->Pin,	P5M0 |=  GPIOx->Pin;	 //开漏输出
	// 	if(GPIOx->Mode == GPIO_OUT_PP)		P5M1 &= ~GPIOx->Pin,	P5M0 |=  GPIOx->Pin;	 //推挽输出
	// }
	// return 0;	//成功
}
