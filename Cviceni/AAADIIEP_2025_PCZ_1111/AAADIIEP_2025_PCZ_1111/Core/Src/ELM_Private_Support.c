/*
 * ELM_Private_Support.c
 *
 *  Created on: Nov 26, 2024
 *      Author: Mirek
 */
//#include "main.h"
#include "ELM_Private_Support.h"
#include "ELM_Pool.h"
GPIO_TypeDef *LP_GPIO_Port_A[8];
uint16_t     LP_Pin_A[8];
uint16_t _DAC;
int ADC_Start;
int ADC_Get_Start(void)
{
	return(ADC_Start);
}


//__weak void ELM_Callback_TIM6_100ms();
//{
//	return;
//}
int power2(int exp)
{
	int base = 2;
    int result = 1;
    while(exp) { result *= base; exp--; }
    return result;
}

void ELMP_LogProbe_8bits_Init(void)
{
	LP_GPIO_Port_A[0]=PIN0_GPIO_Port;
	LP_GPIO_Port_A[1]=PIN1_GPIO_Port;
	LP_GPIO_Port_A[2]=PIN2_GPIO_Port;
	LP_GPIO_Port_A[3]=PIN3_GPIO_Port;
	//LP_GPIO_Port_A[4]=PIN4_GPIO_Port;
	LP_GPIO_Port_A[4]=0;
	LP_GPIO_Port_A[5]=PIN5_GPIO_Port;
	LP_GPIO_Port_A[6]=PIN6_GPIO_Port;
	LP_GPIO_Port_A[7]=PIN7_GPIO_Port;

	LP_Pin_A[0]=PIN0_Pin;
	LP_Pin_A[1]=PIN1_Pin;
	LP_Pin_A[2]=PIN2_Pin;
	LP_Pin_A[3]=PIN3_Pin;
	//LP_Pin_A[4]=PIN4_Pin;
	LP_Pin_A[4]=0;
	LP_Pin_A[5]=PIN5_Pin;
	LP_Pin_A[6]=PIN6_Pin;
	LP_Pin_A[7]=PIN7_Pin;
}
void ELMP_Write_Pin_A(uint16_t A_Pin_Index, int value_Set_Reset)
{
	if(A_Pin_Index>7) return;
	HAL_GPIO_WritePin(LP_GPIO_Port_A[A_Pin_Index], LP_Pin_A[A_Pin_Index], value_Set_Reset);
}

int ELMP_Read_Pin_A(uint16_t A_Pin_Index)
{
	if(A_Pin_Index>7) return -1;
	return(HAL_GPIO_ReadPin(LP_GPIO_Port_A[A_Pin_Index], LP_Pin_A[A_Pin_Index]));
}
void ELMP_SET_Pin_A(uint16_t A_Pin_Index){	if(A_Pin_Index>7) return;	HAL_GPIO_WritePin(LP_GPIO_Port_A[A_Pin_Index], LP_Pin_A[A_Pin_Index], 1);}
void ELMP_RESET_Pin_A(uint16_t A_Pin_Index){	if(A_Pin_Index>7) return;	HAL_GPIO_WritePin(LP_GPIO_Port_A[A_Pin_Index], LP_Pin_A[A_Pin_Index], 0);}

void ELMP_LogProbe_Show(uint8_t x)//zobrazi cislo (00-FF) na Log Probe A0-A7
{
	for(int n=0; n<8; n++)
		if(x & (power2(n)))
			HAL_GPIO_WritePin(LP_GPIO_Port_A[n], LP_Pin_A[n], 1);
		else
			HAL_GPIO_WritePin(LP_GPIO_Port_A[n],LP_Pin_A[n], 0);
	return;
}

void ELMP_LogProbe_Show_A0_A3(uint8_t x)//zobrazi cislo (0-F) na Log Probe A0-A3
{
	for(int n=0; n<4; n++)
		if(x & (power2(n)))
			HAL_GPIO_WritePin(LP_GPIO_Port_A[n], LP_Pin_A[n], 1);
		else
			HAL_GPIO_WritePin(LP_GPIO_Port_A[n],LP_Pin_A[n], 0);
	return;
}

void ELMP_LogProbe_Show_A4_A7(uint8_t x)//zobrazi cislo (0-F) na Log Probe A4-A7
{
	for(int n=0; n<4; n++)
		if(x & (power2(n)))
			HAL_GPIO_WritePin(LP_GPIO_Port_A[n+4], LP_Pin_A[n+4], 1);
		else
			HAL_GPIO_WritePin(LP_GPIO_Port_A[n+4],LP_Pin_A[n+4], 0);
	return;
}



void ELMP_Write_LED(uint16_t LED_Index, int value_Set_Reset)
{
	if(LED_Index>3) return;
	if(LED_Index==0) return;
	if(LED_Index==1)		HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, value_Set_Reset);
	if(LED_Index==2)		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, value_Set_Reset);
	if(LED_Index==3)		HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, value_Set_Reset);
}

void ELMP_Toggle_LED(uint16_t LED_Index)
{
	if(LED_Index>3) return;
	if(LED_Index==0) return;
	if(LED_Index==1)		HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
	if(LED_Index==2)		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
	if(LED_Index==3)		HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
}

int ELM_Read_B1(void)
{
	return(HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin));
}

TIM_HandleTypeDef *htim6__;
TIM_HandleTypeDef *htim7__;
int tim6_Status, tim7_Status;
void ELM_TIM_Init(TIM_HandleTypeDef *htim6_, TIM_HandleTypeDef *htim7_)
{	htim6__=htim6_; htim7__=htim7_; tim6_Status=0; tim7_Status=0; }

void ELM_TIM6_START(void)
{	tim6_Status=1;	HAL_TIM_Base_Start_IT(htim6__);}
void ELM_TIM6_STOP(void)
{	tim6_Status=0;	HAL_TIM_Base_Stop_IT(htim6__);}
int EML_TIM6_IsOn(void)
{	return(tim6_Status);}

void ELM_TIM7_START(void)
{	tim7_Status=1;	HAL_TIM_Base_Start_IT(htim7__);}
void ELM_TIM7_STOP(void)
{	tim7_Status=0;	HAL_TIM_Base_Stop_IT(htim7__);}
int EML_TIM7_IsOn(void)
{	return(tim7_Status);}


void ELMP_MTLB_Callback(uint16_t iD, uint32_t * xData, uint16_t nData_in_values)
{
	switch(iD)
	{
	case 10001:
		EML_Matlab_Callback_10001();
		break;
	case 10002:
		EML_Matlab_Callback_10002();
		break;
	case 10005:
		_DAC= ((uint16_t*) xData)[0];
		break;
	case 10006:
		ADC_Start=1;
		break;
	case 10007:
		ADC_Start=0;
		break;
	case 40002:
	//	DataTransmit2MTLB(40002,(uint8_t *) teplota, 61);

	default:

	}
}

uint16_t ELMP_DAC_Get_Value(int i)
		{
			return(_DAC);
		}

