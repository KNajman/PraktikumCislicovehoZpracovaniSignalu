/*
 *  ELM_Pool.h
 *  Created on: Nov 26, 2024
 *
 */
#ifndef INC_ELM_POOL_H_
#define INC_ELM_POOL_H_
#include "main.h"
#include "ELM_Private_Support.h"
#include "MATLAB_2_STM32.h"

void ELM_POOL_INIT(void);

void ELM_Callback_TIM7_100us(void);
void ELM_Callback_TIM6_100ms(void);
void ELM_Callback_TIM6_500ms(void);
void ELM_Callback_TIM6_1s(void);
void ELM_Callback_B1(void);
void ELM_Callback_PIN6(void);
void ELM_Callback_PIN7(void);

void EML_Matlab_Callback_10001(void);
void EML_Matlab_Callback_10002(void);

void ELM_ADC_Callback_1ms(uint32_t x);

#endif /* INC_ELM_POOL_H_ */
