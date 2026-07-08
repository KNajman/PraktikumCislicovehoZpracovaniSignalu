/*
 * MATLAB_2_STM32.h
 *
 *  Created on: Oct 15, 2024
 *      Author: Mirek
 */

#ifndef INC_MATLAB_2_STM32_H_
#define INC_MATLAB_2_STM32_H_

#include "main.h"

#define MAX_TXRX_DATA 2*1024 * 3



extern void DataReceive_MTLB_Callback(uint16_t iD, uint32_t * xData, uint16_t nData_in_values);



void m2s_Init(UART_HandleTypeDef *huart);
int DataTransmit2MTLB(uint16_t iD, uint8_t * xData, uint16_t nData_in_values);
//void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void m2s_Process(void);//called from inf. loop
int m2s_Status_GET();




#endif /* INC_MATLAB_2_STM32_H_ */
