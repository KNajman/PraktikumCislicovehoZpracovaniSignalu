/*
 * ELM_Private_Support.h
 *
 *  Created on: Nov 26, 2024
 *      Author: Mirek
 */

#ifndef INC_ELM_PRIVATE_SUPPORT_H_
#define INC_ELM_PRIVATE_SUPPORT_H_

#include "main.h"

//GPIO_TypeDef *LP_GPIO_Port_A[8];
//uint16_t     LP_Pin_A[8];

int power2(int exp);

#define LED_GREEN 1
#define LED_BLUE 2
#define LED_RED 3

void ELMP_LogProbe_8bits_Init(void);
void ELMP_Write_Pin_A(uint16_t A_Pin_Index, int value_Set_Reset);
int  ELMP_Read_Pin_A(uint16_t A_Pin_Index);
void ELMP_SET_Pin_A(uint16_t A_Pin_Index);
void ELMP_RESET_Pin_A(uint16_t A_Pin_Index);
void ELMP_LogProbe_Show(uint8_t x);//zobrazi cislo (00-FF) na Log Probe A0-A7
void ELMP_LogProbe_Show_A0_A3(uint8_t x);//zobrazi cislo (0-F) na Log Probe A0-A3
void ELMP_LogProbe_Show_A4_A7(uint8_t x);//zobrazi cislo (0-F) na Log Probe A4-A7


void ELMP_Write_LED(uint16_t LED_Index, int value_Set_Reset);
void ELMP_Toggle_LED(uint16_t LED_Index);
int ELM_Read_B1(void);

void ELM_TIM_Init(TIM_HandleTypeDef *htim6_, TIM_HandleTypeDef *htim7_);
void ELM_TIM6_START(void);
void ELM_TIM6_STOP(void);
int EML_TIM6_IsOn(void);
void ELM_TIM7_START(void);
void ELM_TIM7_STOP(void);
int EML_TIM7_IsOn(void);

void ELMP_MTLB_Callback(uint16_t iD, uint32_t * xData, uint16_t nData_in_values);

uint16_t ELMP_DAC_Get_Value(int i);
int ADC_Get_Start(void);
#endif /* INC_ELM_PRIVATE_SUPPORT_H_ */
