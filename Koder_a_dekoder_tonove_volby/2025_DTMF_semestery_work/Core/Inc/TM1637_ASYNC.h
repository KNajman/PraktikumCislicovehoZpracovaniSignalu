/*
 * TM1637_ASYNC.h
 *
 * Cleaned up & Refactored for stability
 * Created on: 20.12.2025
 * Author: Karel Najman @KNajman
 */

#ifndef TM1637_ASYNC_H_
#define TM1637_ASYNC_H_

#include "main.h" // Pro HAL driver a definice pinu

// --- KONFIGURACE ---
#define TM1637_USE_DOTS 1   // 1 = Dvojtečka se ovládá 8. bitem (pro hodiny)
#define TM1637_ROTATE_180 1 // 1 = Otočit displej o 180 stupňů
// --- MAPOVÁNÍ SEGMENTŮ (Hex kódy) ---
//      A
//     ---
//  F |   | B
//     -G-
//  E |   | C
//     ---
//      D
#define SEG_A 0x01
#define SEG_B 0x02
#define SEG_C 0x04
#define SEG_D 0x08
#define SEG_E 0x10
#define SEG_F 0x20
#define SEG_G 0x40
#define SEG_DP 0x80

/*Numbers 0-9*/
#define DIGIT_0 (SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F)
#define DIGIT_1 (SEG_B | SEG_C)
#define DIGIT_2 (SEG_A | SEG_B | SEG_D | SEG_E | SEG_G)
#define DIGIT_3 (SEG_A | SEG_B | SEG_C | SEG_D | SEG_G)
#define DIGIT_4 (SEG_B | SEG_C | SEG_F | SEG_G)
#define DIGIT_5 (SEG_A | SEG_C | SEG_D | SEG_F | SEG_G)
#define DIGIT_6 (SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G)
#define DIGIT_7 (SEG_A | SEG_B | SEG_C)
#define DIGIT_8 (SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G)
#define DIGIT_9 (SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G)

/*Chars A-F*/
#define CHAR_A (SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G)
#define CHAR_b (SEG_C | SEG_D | SEG_E | SEG_F | SEG_G)
#define CHAR_C (SEG_A | SEG_D | SEG_E | SEG_F)
#define CHAR_d (SEG_B | SEG_C | SEG_D | SEG_E | SEG_G)
#define CHAR_E (SEG_A | SEG_D | SEG_E | SEG_F | SEG_G)
#define CHAR_F (SEG_A | SEG_E | SEG_F | SEG_G)

/* Other chars */
#define CHAR_H (SEG_B | SEG_C | SEG_E | SEG_F | SEG_G)
#define CHAR_h (SEG_C | SEG_E | SEG_F | SEG_G)
#define CHAR_I (SEG_B | SEG_C)
#define CHAR_L (SEG_D | SEG_E | SEG_F)
#define CHAR_P (SEG_A | SEG_B | SEG_E | SEG_F | SEG_G)
#define CHAR_r (SEG_E | SEG_G)
#define CHAR_S (SEG_A | SEG_C | SEG_D | SEG_F | SEG_G)
#define CHAR_U (SEG_B | SEG_C | SEG_D | SEG_E | SEG_F)
#define CHAR_u (SEG_C | SEG_D | SEG_E)
#define CHAR_Y (SEG_B | SEG_C | SEG_D | SEG_F | SEG_G)

/* Symbols */
#define CHAR_DASH (SEG_G)
#define CHAR_BLANK 0x00

/*PROTOTYPY FUNKCÍ */

// Inicializace
void TM1637_Init(TIM_HandleTypeDef *htim, GPIO_TypeDef *clkPort, uint16_t clkPin, GPIO_TypeDef *dioPort, uint16_t dioPin);

// Hlavní funkce pro zobrazení (Asynchronní - neblokuje)
// rawData: pole 4 bajtů s kódy segmentů
// brightness: 0 (tma) až 7 (max jas)
void TM1637_Update_Async(const uint8_t *rawData, uint8_t brightness);

// Helper pro převod char na segmenty (pokud ho chcete využít v main)
uint8_t TM1637_CharToSegment(char c);

// Tuto funkci dejte do callbacku časovače (HAL_TIM_PeriodElapsedCallback)
void TM1637_Timer_Callback(TIM_HandleTypeDef *htim);

// Vyčistí displej (zobrazí prázdné znaky)
void TM1637_ClearDisplay(void);

#endif /* TM1637_ASYNC_H_ */
