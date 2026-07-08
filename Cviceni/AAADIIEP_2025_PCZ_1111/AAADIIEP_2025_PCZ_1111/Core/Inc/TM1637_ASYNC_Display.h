/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : TM1637_ASYNC_Display.h
  * @brief          : Header for TM1637_ASYNC_Display.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  *
  * Copyright (c) 2024 byMH.
  *
  * Ovladac pracuje asynchronne s dispejem TM1637,
  * 2025-03-06 - otoceni napuisu
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TM1637_ASYNC_DISPLAY_H
#define __TM1637_ASYNC_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
// hlavicka pro dany kontroler
#include "stm32l4xx_hal.h"
//#include "main.h"

void TM1637_Init(TIM_HandleTypeDef *htim_4_Display, GPIO_TypeDef *SCLK_Port_, uint16_t SCLK_Pin_, GPIO_TypeDef *SDO_Port_, uint16_t SDO_Pin_, uint8_t disp_Rotate_);

void TM1637_Process_Data_Display(TIM_HandleTypeDef *htim);

void TM1637_display_MyData(uint8_t *DisplayBuffer, uint8_t Br);//kompletni funkce na zobrazeni
void TM1637_display(char *text4char);//zobrazi 4 znaky v posledni Brig...(jasnosti)
uint8_t TM1637_Get_disp_X(void); //pokud nula, tak se nezapisuje do displeje
uint8_t char2segments(char c);

void TM1637_Time_SET(uint8_t hour, uint8_t minut, uint8_t second);
void TM1637_Time_GET(uint8_t *hour, uint8_t *minut, uint8_t *second);
void TM1637_Show_Time_MMSS(uint8_t minut, uint8_t second);
void TM1637_Show_Time_HHMM_Next(void);
void TM1637_Show_Time_MMSS_Next(void);

void TM1637_Set_Brightness(uint8_t br);//0-7

void TM1637_Show_Time_PAUSE(void);
void TM1637_Show_Time_RESUME(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
