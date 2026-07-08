/*
 * ZEL_Expander.h
 *
 *  Created on: Mar 26, 2025
 *      Author: Mirek
 */

#ifndef INC_ZEL_EXPANDER_H_
#define INC_ZEL_EXPANDER_H_

// hlavicka pro dany kontroler
#include "stm32l4xx_hal.h"

void ZelExpander_Init(UART_HandleTypeDef *huart_4_Expander_);
void ZelExpander_SendData(uint16_t data16_Int);

void ZelExpander_Send(void);
void ZelExpander_SetA(uint8_t nBit);
void ZelExpander_SetB(uint8_t nBit);
void ZelExpander_ResetA(uint8_t nBit);
void ZelExpander_ResetB(uint8_t nBit);


#endif /* INC_ZEL_EXPANDER_H_ */
