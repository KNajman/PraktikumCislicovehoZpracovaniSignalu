/*
 * ZEL_Expander.c
 *
 *  Created on: Mar 26, 2025
 *      Author: Mirek
 *
 *      Zaklady ELektroniky
 */

#include "ZEL_Expander.h"

UART_HandleTypeDef *huart_4_Expander;
uint8_t ZelExpander_Data[2]; //naposledy odeslano

void ZelExpander_Init(UART_HandleTypeDef *huart_4_Expander_)
{
	huart_4_Expander = huart_4_Expander_;
	ZelExpander_SendData(0);
	return;
}
// ZelExpander_
void ZelExpander_SendData(uint16_t data16_Int)
{
	ZelExpander_Data[1] = data16_Int & 0x00FF;
	ZelExpander_Data[0] = data16_Int >> 8;
	ZelExpander_Send();
}

int ttt;
void ZelExpander_Send(void)
{
	ZelExpander_Data[0] = ZelExpander_Data[0] | 0x80;
	HAL_StatusTypeDef su=HAL_UART_Transmit_DMA(huart_4_Expander, ZelExpander_Data, 2);
	if(su)
	{
		ttt++;
		if (ttt<100){ HAL_Delay(10); ZelExpander_Send(); }
	}
	else ttt=0;
}

void ZelExpander_SetA(uint8_t nBit)
{
	if(nBit<7)		ZelExpander_Data[0] = ZelExpander_Data[0] | (1<<nBit);
}
void ZelExpander_SetB(uint8_t nBit)
{
	if(nBit<8)		ZelExpander_Data[1] = ZelExpander_Data[1] | (1<<nBit);
}

void ZelExpander_ResetA(uint8_t nBit)
{
	uint8_t n=0;
	if(nBit<7)
	{
		n = 1<<nBit;
		n = !n;
		ZelExpander_Data[0] = ZelExpander_Data[0] & n;
	}
}
void ZelExpander_ResetB(uint8_t nBit)
{
	uint8_t n=0;
	if(nBit<8)
	{
		n = 1<<nBit;
		n = !n;
		ZelExpander_Data[1] = ZelExpander_Data[1] & n;
	}
}

void ZelExpander_Bit(uint8_t nBit, uint8_t value)
{
	//if(value)

}

