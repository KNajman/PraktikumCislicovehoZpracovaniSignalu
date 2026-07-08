/*
 * TM1637_ASYNC.c
 *
 * Cleaned up & Refactored for stability
 * Created on: 20.12.2025
 * Author: Karel Najman @KNajman
 */

#include "TM1637_ASYNC.h"

// --- INTERNÍ PROMĚNNÉ ---
static TIM_HandleTypeDef *tm_timer;
static GPIO_TypeDef *CLK_Port;
static uint16_t CLK_Pin;
static GPIO_TypeDef *DIO_Port;
static uint16_t DIO_Pin;

// Buffery pro playback signálu.
// Velikost 300 bajtů stačí pro kompletní přenos (Start + 3 příkazy + Data + Stop).
#define SIGNAL_BUFFER_SIZE 300

static uint8_t clk_buffer[SIGNAL_BUFFER_SIZE];
static uint8_t dio_buffer[SIGNAL_BUFFER_SIZE];
static volatile uint16_t buffer_index = 0;   // Kde v bufferu zrovna jsme
static volatile uint16_t buffer_len = 0;     // Celková délka nahraného signálu
static volatile uint8_t is_transmitting = 0; // Zda se právě aktualizuje displej

// Pomocný kurzor pro ZÁPIS do bufferu
static uint16_t write_cursor = 0;

// --- INTERNÍ FUNKCE PRO GENEROVÁNÍ PRŮBĚHU ---

// Přidá jeden časový krok (stav CLK a DIO) do bufferu
static void Add_Step(uint8_t clk, uint8_t dio)
{
    if (write_cursor < SIGNAL_BUFFER_SIZE)
    {
        clk_buffer[write_cursor] = clk;
        dio_buffer[write_cursor] = dio;
        write_cursor++;
    }
}

// Vygeneruje sekvenci pro START bit (CLK High->High, DIO High->Low)
static void Gen_Start(void)
{
    Add_Step(1, 1);
    Add_Step(1, 0);
    Add_Step(0, 0);
}

// Vygeneruje sekvenci pro STOP bit (CLK Low->High, DIO Low->High)
static void Gen_Stop(void)
{
    Add_Step(0, 0);
    Add_Step(1, 0);
    Add_Step(1, 1);
}

// Vygeneruje sekvenci pro 1 Byte (LSB first) + čte ACK (ignorujeme ho, ale musíme vygenerovat takt)
static void Gen_Byte(uint8_t byte)
{
    for (int i = 0; i < 8; i++)
    {
        uint8_t bit = (byte >> i) & 0x01;
        // 1. CLK Low, změna dat
        Add_Step(0, bit);
        // 2. CLK High (platná data)
        Add_Step(1, bit);
        // 3. CLK Low
        Add_Step(0, bit);
    }

    // ACK (9. takt)
    // CLK Low, DIO uvolnit (High - simulujeme 1, TM1637 by ho měl stáhnout, my to nečteme)
    Add_Step(0, 1); // Příprava na ACK
    Add_Step(1, 1); // CLK High (ACK pulse)
    Add_Step(0, 1); // CLK Low
}

// Funkce pro otočení segmentů o 180 stupňů
static uint8_t Rotate_Segment_Bits(uint8_t seg)
{
    uint8_t r = 0;
    if (seg & 0x01)
        r |= 0x08; // SEG_A -> SEG_D
    if (seg & 0x02)
        r |= 0x10; // SEG_B -> SEG_E
    if (seg & 0x04)
        r |= 0x20; // SEG_C -> SEG_F
    if (seg & 0x08)
        r |= 0x01; // SEG_D -> SEG_A
    if (seg & 0x10)
        r |= 0x02; // SEG_E -> SEG_B
    if (seg & 0x20)
        r |= 0x04; // SEG_F -> SEG_C
    if (seg & 0x40)
        r |= 0x40; // SEG_G -> SEG_G (střed zůstává středem)
    if (seg & 0x80)
        r |= 0x80; // DP -> DP (tečka zůstane u dané číslice)
    return r;
}

// --- API ---

// Inicializace
void TM1637_Init(TIM_HandleTypeDef *htim, GPIO_TypeDef *clkPort, uint16_t clkPin, GPIO_TypeDef *dioPort, uint16_t dioPin)
{
    tm_timer = htim;
    CLK_Port = clkPort;
    CLK_Pin = clkPin;
    DIO_Port = dioPort;
    DIO_Pin = dioPin;

    // Default High State
    HAL_GPIO_WritePin(CLK_Port, CLK_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DIO_Port, DIO_Pin, GPIO_PIN_SET);

    is_transmitting = 0;
}

// Update displeje
void TM1637_Update_Async(const uint8_t *rawData, uint8_t brightness)
{
    if (is_transmitting)
    {
        // Pokud zrovna vysíláme, zahodíme požadavek (nebo bychom mohli čekat)
        // Pro jednoduchost a rychlost return;
        return;
    }

    // Reset zapisovacího kurzoru
    write_cursor = 0;

    // Omezení jasu (0-7), bit 3 musí být 1 (Display ON)
    if (brightness > 7)
        brightness = 7;
    uint8_t brightness_ctrl = 0x88 | brightness;

    // --- PŘÍPRAVA DAT (ROTACE) ---
    uint8_t finalData[4];

#if TM1637_ROTATE_180
    // Otočení o 180 stupňů:
    // 1. Prohodit pořadí (Index 0 jde na pozici 3)
    // 2. Prohodit segmenty (A za D atd.)
    finalData[0] = Rotate_Segment_Bits(rawData[3]);
    finalData[1] = Rotate_Segment_Bits(rawData[2]);
    finalData[2] = Rotate_Segment_Bits(rawData[1]);
    finalData[3] = Rotate_Segment_Bits(rawData[0]);
#else
    // Normální režim - jen kopie
    finalData[0] = rawData[0];
    finalData[1] = rawData[1];
    finalData[2] = rawData[2];
    finalData[3] = rawData[3];
#endif

    //1. Data Set (Write memory, Auto increment)
    Gen_Start();
    Gen_Byte(0x40);
    Gen_Stop();

    //2. Set Address (C0H = adresa 00H)
    Gen_Start();
    Gen_Byte(0xC0);

    // 3. Data (4 Byty) - posíláme v kuse, protože máme Auto Increment
    for (int i = 0; i < 4; i++)
    {
        Gen_Byte(finalData[i]);
    }
    Gen_Stop();

    // 4. Display Control (Brightness)
    Gen_Start();
    Gen_Byte(brightness_ctrl);
    Gen_Stop();

    // Na konci necháme linky nahoře
    Add_Step(1, 1);

    // Nastavení přehrávání
    buffer_len = write_cursor;
    buffer_index = 0;
    is_transmitting = 1;

    // Spuštění časovače
    HAL_TIM_Base_Start_IT(tm_timer);
}

// Volat z HAL_TIM_PeriodElapsedCallback
void TM1637_Timer_Callback(TIM_HandleTypeDef *htim)
{
    if (htim == tm_timer && is_transmitting)
    {
        if (buffer_index < buffer_len)
        {
            // Nastavit piny podle bufferu
            HAL_GPIO_WritePin(CLK_Port, CLK_Pin, clk_buffer[buffer_index] ? GPIO_PIN_SET : GPIO_PIN_RESET);
            HAL_GPIO_WritePin(DIO_Port, DIO_Pin, dio_buffer[buffer_index] ? GPIO_PIN_SET : GPIO_PIN_RESET);
            buffer_index++;
        }
        else
        {
            // Konec přenosu
            HAL_TIM_Base_Stop_IT(tm_timer);
            is_transmitting = 0;

            // Pojistka: Piny do High
            HAL_GPIO_WritePin(CLK_Port, CLK_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(DIO_Port, DIO_Pin, GPIO_PIN_SET);
        }
    }
}

/*
 * Pomocná funkce pro převod znaků
 */
uint8_t TM1637_CharToSegment(char c)
{
    switch (c)
    {
    case '0':
        return DIGIT_0;
    case '1':
        return DIGIT_1;
    case '2':
        return DIGIT_2;
    case '3':
        return DIGIT_3;
    case '4':
        return DIGIT_4;
    case '5':
        return DIGIT_5;
    case '6':
        return DIGIT_6;
    case '7':
        return DIGIT_7;
    case '8':
        return DIGIT_8;
    case '9':
        return DIGIT_9;
    case 'A':
    case 'a':
        return CHAR_A;
    case 'B':
    case 'b':
        return CHAR_b;
    case 'C':
    case 'c':
        return CHAR_C;
    case 'D':
    case 'd':
        return CHAR_d;
    case '*':
        return CHAR_H;
    case '#':
        return CHAR_DASH;
    case '-':
        return CHAR_DASH;
    default:
        return CHAR_BLANK;
    }
}

/*
 * Funkce pro vymazání displeje
 * */
void TM1637_ClearDisplay(void)
{
    uint8_t blank_data[4] = {CHAR_BLANK, CHAR_BLANK, CHAR_BLANK, CHAR_BLANK};
    TM1637_Update_Async(blank_data, 0); // Jas 0 (vypnuto)
}
