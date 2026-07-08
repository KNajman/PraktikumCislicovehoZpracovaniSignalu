/* * ELM_Pool.c * *  Created on: Nov 26, 2024 *
 * VERZE M8o - Matlab - 8x PIN jako GPIO_OUT
 *           - komunikace s Matlabem
 */
//#ifndef INC_ELM_POOL_C_
//#define INC_ELM_POOL_C_
#include "ELM_Pool.h"
#include "math.h"
/* USER CODE BEGIN 1 */
uint32_t nB1_on;

void ELM_POOL_INIT(void) //
{
	nB1_on=0;
}

void ELM_Callback_PIN6(void)
{

}

void ELM_Callback_PIN7(void)
{

}
void ELM_Callback_B1(void)// reaguje na nabeznou i sestupnou hranu, B1 je neosetrene
{
	if(ELM_Read_B1())
	{
		nB1_on=1;
	}
	else
	{
		if(nB1_on>2)
		{
			nB1_on = nB1_on-1;
			DataTransmit2MTLB(10003,(uint8_t *) &nB1_on, 1);
			nB1_on = 0;
		}
	}
}

void ELM_Callback_TIM6_100ms(void)
{

}

void ELM_Callback_TIM6_500ms(void)
{

}

void ELM_Callback_TIM6_1s(void)
{
	ELMP_Toggle_LED(LED_GREEN);
	if(nB1_on)nB1_on++;
}

void ELM_Callback_TIM7_100us(void)//10kHz... 100us
{
	return;
}

void EML_Matlab_Callback_10001(void)
{
	//priprava dat na odpoved na udalost 10001
	char t[12]="Hello world!";
	uint32_t t32[12];
	for(int i=0; i<12; i++)
		t32[i]= t[i];
	//odeslani
	DataTransmit2MTLB(10001,(uint8_t *) &t32[0], 12);
	return;
}

void EML_Matlab_Callback_10002(void)
{
		float x;// posle 100 vzorku signalu v 8-bitovem rozliseni
		uint32_t x1[100];
		float Fs=10000;
		for(int n=0; n<100; n++)
		{
			x=sin(2*M_PI*200*n/Fs);// -1...1
			x=x+1;// 0...2
			x=x/2;// 0...1
			x=x*255; //0...255
			x1[n]=x;
		}

		DataTransmit2MTLB(10002,(uint8_t *) &x1[0], 100);
}

int nX;
uint32_t xADC[1000], xxADC;
void ELM_ADC_Callback_1ms(uint32_t x)
{
	xADC[nX]=x;
	nX++;
	if(nX>999)
	{
		nX=0;
		//xxADC=0;
		//for(int i=0; i<1000; i++)
		//	xxADC = xxADC + xADC[i];
		//xxADC = xxADC/1000;
		//DataTransmit2MTLB(10004,(uint8_t *) &xxADC, 1);
		if(ADC_Get_Start())
			DataTransmit2MTLB(10005,(uint8_t *) &xADC, 1000);
	}
}
/* USER CODE END 1 */
//#endif /* INC_ELM_POOL_C_ */
