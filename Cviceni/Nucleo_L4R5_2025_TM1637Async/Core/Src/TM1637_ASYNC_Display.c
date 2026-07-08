/*
 * TM1637_ASYNC_Display.c
 *
 *  Created on: Jul 18, 2024
 *      Author: Mirek
 */
// Segments
//   --0x01--
//  |        |
// 0x20     0x02
//  |        |
//   --0x40- -
//  |        |
// 0x10     0x04
//  |        |
//   --0x08--
//  A_SEG				0x01
//  B_SEG				0x02
//  C_SEG				0x04
//  D_SEG				0x08
//  E_SEG				0x10
//  F_SEG				0x20
//  G_SEG				0x40
//  DP_SEG				0x80

// TM1637_

#include "TM1637_ASYNC_Display.h"

TIM_HandleTypeDef *htim_4_TM1637;
GPIO_TypeDef *TM1637_SCLK_Port, *TM1637_SDO_Port;
uint16_t TM1637_SCLK_Pin, TM1637_SDO_Pin;

uint8_t disp_X;
uint8_t disp_Rotate; // 0... normal, 1...rotace napisu o 180°
uint8_t disp_n;		 // pomocna promenna
uint8_t disp_CLK_buff[256];
uint8_t disp_Data_buff[256];

void TM1637_Init(TIM_HandleTypeDef *htim_4_Display, GPIO_TypeDef *SCLK_Port_, uint16_t SCLK_Pin_, GPIO_TypeDef *SDO_Port_, uint16_t SDO_Pin_, uint8_t disp_Rotate_)
{
	disp_X = 0;
	disp_Rotate = disp_Rotate_;
	htim_4_TM1637 = htim_4_Display;
	TM1637_SCLK_Port = SCLK_Port_;
	TM1637_SCLK_Pin = SCLK_Pin_;
	TM1637_SDO_Port = SDO_Port_;
	TM1637_SDO_Pin = SDO_Pin_;
	HAL_TIM_Base_Start_IT(htim_4_TM1637); // start timer for display driver
}

void TM1637_Process_Data_Display(TIM_HandleTypeDef *htim)
{
	if (htim == htim_4_TM1637)
	{
		if (disp_X)
		{
			HAL_GPIO_WritePin(TM1637_SCLK_Port, TM1637_SCLK_Pin, disp_CLK_buff[disp_X]);
			HAL_GPIO_WritePin(TM1637_SDO_Port, TM1637_SDO_Pin, disp_Data_buff[disp_X]);
			disp_X++;
		}
		else
			HAL_TIM_Base_Stop_IT(htim_4_TM1637);
	}
	return;
}
uint8_t TM1637_Get_disp_X(void)
{
	return disp_X;
}

void predchoziData(uint8_t nn)
{
	if (nn > 0)
		disp_Data_buff[nn] = disp_Data_buff[nn - 1];
	else
		disp_Data_buff[nn] = disp_Data_buff[255];
}
void predchoziCLK(uint8_t nn)
{
	if (nn > 0)
		disp_CLK_buff[nn] = disp_CLK_buff[nn - 1];
	else
		disp_CLK_buff[nn] = disp_CLK_buff[255];
}

void CLKlow()
{
	disp_CLK_buff[disp_n] = 0;
	predchoziData(disp_n);
	disp_n++;
}
void CLKhigh()
{
	disp_CLK_buff[disp_n] = 1;
	predchoziData(disp_n);
	disp_n++;
}
void SDOlow()
{
	predchoziCLK(disp_n);
	disp_Data_buff[disp_n] = 0;
	disp_n++;
}
void SDOhigh()
{
	predchoziCLK(disp_n);
	disp_Data_buff[disp_n] = 1;
	disp_n++;
}

void start_pack()
{
	CLKhigh();
	SDOhigh();
	SDOlow();
	CLKlow();
}
void end_pack()
{
	CLKlow();
	SDOlow();
	CLKhigh();
	SDOhigh();
}
void ACKcheck() // atrapa...
{
	CLKlow(); // lower CLK line
	CLKlow(); // lower CLK line
	CLKlow();
}

void disp_DataOut(uint8_t *tm1637_TxBuffer) // Low level data transfer function
{
	for (int8_t j = 0; j < 8; j++) // Send least significant bit first
	{
		CLKlow();
		predchoziCLK(disp_n);
		disp_Data_buff[disp_n] = tm1637_TxBuffer[j];
		disp_n++; // Check logic level
		CLKhigh();
	}
}
void disp_TxCommand(uint8_t *Command)
{ // Handles high level (bit by bit) transmission operation
	uint8_t ByteData[8] = {0};

	for (uint8_t i = 0; i < 8; i++)
		ByteData[i] = (Command[0] & (0x01 << i)) && 1;

	start_pack();
	disp_DataOut(ByteData); // Send one byte
	CLKlow();				// Send one CLK for acknowledgment
	CLKhigh();
	ACKcheck();						   // wait for acknowledgment.
	if ((Command[0] & 0xC0) != (0xC0)) // Check if the received packet is not an address.
		end_pack();
}

void disp_TxData(uint8_t *Data, uint8_t PacketSize)
{ // Handles high level (bit by bit) transmission operation
	uint8_t ByteData[8] = {0};

	for (uint8_t i = 0; i < PacketSize; i++)
	{
		for (uint8_t j = 0; j < 8; j++)
		{
			ByteData[j] = (Data[i] & (0x01 << j)) && 1;
		}
		disp_DataOut(ByteData);
		CLKlow();
		CLKhigh();
		ACKcheck(); // Transmit byte by byte
	}
	end_pack(); // Send end packet at the end of data transmission.
}
void DisplayClear()
{
	uint8_t EmptyBuffer[4] = {0};
	uint8_t CommandCarrier[1] = {0};
	CommandCarrier[0] = 0x40; // DATA_SET;									//Send set data command
	disp_TxCommand(CommandCarrier);
	CommandCarrier[0] = 0xC0; // Set address
	disp_TxCommand(CommandCarrier);
	disp_TxData(EmptyBuffer, 4);
	CommandCarrier[0] = 0x80; // DISPLAY_OFF;
	disp_TxCommand(CommandCarrier);
}
void disp_SetBr(uint8_t BrighnessLevel)
{
	uint8_t BrighnessBuffer[8] = {0};
	if (BrighnessLevel <= 7)					// there are 7 levels of brightness
	{											// Any value above that will be ignored.
		BrighnessLevel = BrighnessLevel | 0x88; // Set Brightness level with display on command

		for (uint8_t i = 0; i < 8; i++)
		{
			BrighnessBuffer[i] = (BrighnessLevel & (0x01 << i)) && 1;
		}
		start_pack();
		disp_DataOut(BrighnessBuffer);
		CLKlow(); // Send one CLK for acknowledgment
		CLKhigh();
		ACKcheck(); // wait for acknowledgment.
		end_pack();
	}
	return;
}

uint8_t Rotate_180(uint8_t c)
{
	uint8_t a = 0;
	if (c & 0x01)
		a = a + 0x08;
	if (c & 0x02)
		a = a + 0x10;
	if (c & 0x04)
		a = a + 0x20;
	if (c & 0x08)
		a = a + 0x01;
	if (c & 0x10)
		a = a + 0x02;
	if (c & 0x20)
		a = a + 0x04;
	if (c & 0x40)
		a = a + 0x40;
	if (c & 0x80)
		a = a + 0x80;
	return (a);
}

uint8_t char2segments(char c)
{
	switch (c)
	{
	case '0':
		return 0x3f;
	case '1':
		return 0x06;
	case '2':
		return 0x5b;
	case '3':
		return 0x4f;
	case '4':
		return 0x66;
	case '5':
		return 0x6d;
	case '6':
		return 0x7d;
	case '7':
		return 0x07;
	case '8':
		return 0x7f;
	case '9':
		return 0x6f;
	case '_':
		return 0x08;
	case '^':
		return 0x01; // ¯
	case '-':
		return 0x40;
	case '*':
		return 0x63; // °
	case ' ':
		return 0x00; // space
	case 'A':
		return 0x77; // upper case A
	case 'a':
		return 0x5f; // lower case a
	case 'B':		 // lower case b
	case 'b':
		return 0x7c; // lower case b
	case 'C':
		return 0x39; // upper case C
	case 'c':
		return 0x58; // lower case c
	case 'D':		 // lower case d
	case 'd':
		return 0x5e; // lower case d
	case 'E':		 // upper case E
	case 'e':
		return 0x79; // upper case E
	case 'F':		 // upper case F
	case 'f':
		return 0x71; // upper case F
	case 'G':		 // upper case G
	case 'g':
		return 0x35; // upper case G
	case 'H':
		return 0x76; // upper case H
	case 'h':
		return 0x74; // lower case h
	case 'I':
		return 0x06; // 1
	case 'i':
		return 0x04; // lower case i
	case 'J':
		return 0x1e; // upper case J
	case 'j':
		return 0x16; // lower case j
	case 'K':		 // upper case K
	case 'k':
		return 0x75; // upper case K
	case 'L':		 // upper case L
	case 'l':
		return 0x38; // upper case L
	case 'M':		 // twice tall n
	case 'm':
		return 0x37; // twice tall �?�
	case 'N':		 // lower case n
	case 'n':
		return 0x54; // lower case n
	case 'O':		 // lower case o
	case 'o':
		return 0x5c; // lower case o
	case 'P':		 // upper case P
	case 'p':
		return 0x73; // upper case P
	case 'Q':
		return 0x7b; // upper case Q
	case 'q':
		return 0x67; // lower case q
	case 'R':		 // lower case r
	case 'r':
		return 0x50; // lower case r
	case 'S':		 // 5
	case 's':
		return 0x6d; // 5
	case 'T':		 // lower case t
	case 't':
		return 0x78; // lower case t
	case 'U':		 // lower case u
	case 'u':
		return 0x1c; // lower case u
	case 'V':		 // twice tall u
	case 'v':
		return 0x3e; // twice tall u
	case 'W':
		return 0x7e; // upside down A
	case 'w':
		return 0x2a; // separated w
	case 'X':		 // upper case H
	case 'x':
		return 0x76; // upper case H
	case 'Y':		 // lower case y
	case 'y':
		return 0x6e; // lower case y
	case 'Z':		 // separated Z
	case 'z':
		return 0x1b; // separated Z
	}
	return 0;
}

void TM1637_display_MyData(uint8_t *DisplayBuffer, uint8_t Br) // kompletni funkce na zobrazeni
{
	if (disp_X)
		return; // zastavit, protoze bezi vysilani
	disp_n = 1;
	if (Br > 8)
		Br = 7; // there are 7 levels of brightness
	// pokud je Br = 8 potom nechat predchozi/ nenastavovat
	uint8_t CommandCarrier[1] = {0};
	CommandCarrier[0] = 0x40; // DATA_SET;
	disp_TxCommand(CommandCarrier);
	CommandCarrier[0] = 0xC0; // Set address
	disp_TxCommand(CommandCarrier);

	// otoceni napisu
	if (disp_Rotate)
	{
		uint8_t a, *x = DisplayBuffer;
		a = Rotate_180(x[0]);
		x[0] = Rotate_180(x[3]);
		x[3] = a;

		a = Rotate_180(x[1]);
		x[1] = Rotate_180(x[2]);
		x[2] = a;

		if (x[2] & 0x80)
			x[1] = x[1] + 0x80; // dvojtecka
	}
	//~otoceni napisu

	disp_TxData(DisplayBuffer, 4); // Map the data stored in RAM to the display
	disp_SetBr(Br);				   // Turn on display and set brightness
	for (int i = disp_n; i < 256; i++)
	{
		disp_CLK_buff[i] = 1;
		disp_Data_buff[i] = 1;
	}
	disp_X = 1;
	HAL_TIM_Base_Start_IT(htim_4_TM1637); // display driver
	return;
}

void TM1637_Set_Brightness(uint8_t br) // nastavit jasnost dispeje 0-7, 8 je pro puvodni hodnotu, jinak 7
{
	uint8_t x[4] = {0, 0, 0, 0};
	TM1637_display_MyData(x, br);
}

void TM1637_display(char *text4char) // zobrazi 4 znaky v posledni Brig...(jasnosti)
{
	uint8_t x[4];
	x[0] = char2segments(text4char[0]);
	x[1] = char2segments(text4char[1]);
	x[2] = char2segments(text4char[2]);
	x[3] = char2segments(text4char[3]);
	TM1637_display_MyData(x, 8);
}

uint8_t h1, h0, m1, m0, s1, s0;
void TM1637_Time_SET(uint8_t hour, uint8_t minut, uint8_t second)
{
	if (hour > 23)
		hour = 0; // minut-60;
	if (minut > 59)
		minut = 0; // minut-60;
	if (second > 59)
		second = 0;
	h1 = hour / 10;
	h0 = hour - h1 * 10;
	m1 = minut / 10;
	m0 = minut - m1 * 10;
	s1 = second / 10;
	s0 = second - s1 * 10;
}
void TM1637_Time_GET(uint8_t *hour, uint8_t *minut, uint8_t *second)
{
	hour[0] = h1 * 10 + h0;
	minut[0] = m1 * 10 + m0;
	second[0] = s1 * 10 + s0;
}
void TM1637_Show_Time_MMSS(uint8_t minut, uint8_t second)
{
	uint8_t x[4];
	if (minut > 59)
		minut = 0; // minut-60;
	if (second > 59)
		second = 0;
	m1 = minut / 10;
	m0 = minut - m1 * 10;
	s1 = second / 10;
	s0 = second - s1 * 10;
	x[0] = char2segments(m1 + 48);
	x[1] = char2segments(m0 + 48) + 0x80; //+dvojtecka
	x[2] = char2segments(s1 + 48);
	x[3] = char2segments(s0 + 48);
	TM1637_display_MyData(x, 8);
}

uint8_t Show_Time_paused;
void TM1637_Show_Time_PAUSE(void)
{
	Show_Time_paused = 1;
}

void TM1637_Show_Time_RESUME(void)
{
	Show_Time_paused = 0;
}

void next_ONE_sec() // prida jednu sekundu
{
	s0++;
	if (s0 == 10)
	{
		s0 = 0;
		s1++;
		if (s1 == 6)
		{
			s1 = 0;
			m0++;
			if (m0 == 10)
			{
				m0 = 0;
				m1++;
				if (m1 == 6)
				{
					m1 = 0;
					h0++;
					if (h0 == 10)
					{
						h0 = 0;
						h1++;
					}
					if (h1 == 2)
					{
						if (h0 == 4)
						{
							h0 = 0;
							h1 = 0;
						}
					}
				}
			}
		}
	}
}
void TM1637_Show_Time_MMSS_Next(void)
{
	uint8_t x[4];
	next_ONE_sec();
	x[0] = char2segments(m1 + 48);
	x[1] = char2segments(m0 + 48) + 0x80; //+dvojtecka
	x[2] = char2segments(s1 + 48);
	x[3] = char2segments(s0 + 48);
	if (!Show_Time_paused)
		TM1637_display_MyData(x, 8);
}

void TM1637_Show_Time_HHMM_Next(void)
{
	uint8_t x[4];
	next_ONE_sec();
	x[0] = char2segments(h1 + 48);
	if (s0 & 0x01)
		x[1] = char2segments(h0 + 48) + 0x80; //+dvojtecka
	else
		x[1] = char2segments(h0 + 48);
	x[2] = char2segments(m1 + 48);
	x[3] = char2segments(m0 + 48);
	if (!Show_Time_paused)
		TM1637_display_MyData(x, 8);
}
