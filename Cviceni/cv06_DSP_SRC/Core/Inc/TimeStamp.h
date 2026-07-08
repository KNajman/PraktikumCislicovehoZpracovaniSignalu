/*
 * verze 1.1 26.9.2023 
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TIMESTAMP_H
#define __TIMESTAMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

//uint32_t xTime01, xTime02;
//uint32_t GetTime(void) { xTime01=xTime02; xTime02=htim5.Instance->CNT; ntt++; tt[ntt]=xTime02-xTime01; return tt[ntt];}

//volatile uint32_t TimeStamp_tt[256];//time in ticks
#define TIME_STAMP_BUF_LEN 256   //1024    //256
#define TIME_STAMP_BUF_MASK TIME_STAMP_BUF_LEN - 1
uint32_t TimeStamp_tt[TIME_STAMP_BUF_LEN];//time in ticks
uint8_t  TimeStamp_aa[TIME_STAMP_BUF_LEN];//stamp for current time
uint_fast8_t  TimeStamp_ntt;    // actual position in tt array
uint_fast8_t  TimeStamp_naa;    // actual value in aa array
TIM_HandleTypeDef *TimeStamp_htim;
volatile uint32_t *TimeStamp_CNT;

// par. timer
void TimeStamp_RESET()
{
	TimeStamp_ntt = 0;
	TimeStamp_naa = 0;
}
HAL_StatusTypeDef TimeStamp_INIT(TIM_HandleTypeDef *htim)
{
	TimeStamp_RESET();
	TimeStamp_htim= htim;
	TimeStamp_CNT = &TimeStamp_htim->Instance->CNT;
	HAL_StatusTypeDef s = HAL_TIM_Base_Start_IT(htim);// for time measure
	return(s);
}

//volatile uint32_t* TimeStamp_GET_Buff_Time()
uint32_t* TimeStamp_GET_Buff_Time()
{	return(TimeStamp_tt); }

uint8_t* TimeStamp_GET_Buff_Stamp(){
	return(TimeStamp_aa);}


volatile void TimeStamp_SET(){
	TimeStamp_tt[(TimeStamp_ntt++)&0x000000FF]= *TimeStamp_CNT;
	//TimeStamp_aa[(TimeStamp_ntt) & TIME_STAMP_BUF_MASK]= TimeStamp_naa++;
	//TimeStamp_tt[(TimeStamp_ntt++) & TIME_STAMP_BUF_MASK]= *TimeStamp_CNT;

}

void TimeStamp_SET_A(){
	TimeStamp_tt[(TimeStamp_ntt)&0x000000FF] = *TimeStamp_CNT;
	TimeStamp_aa[(TimeStamp_ntt++)&0x000000FF] = 'A';
}

void TimeStamp_SET_B(){	TimeStamp_tt[(TimeStamp_ntt)&0x000000FF] = *TimeStamp_CNT;	TimeStamp_aa[(TimeStamp_ntt++)&0x000000FF] = 'B';}
void TimeStamp_SET_C(){	TimeStamp_tt[(TimeStamp_ntt)&0x000000FF] = *TimeStamp_CNT;	TimeStamp_aa[(TimeStamp_ntt++)&0x000000FF] = 'C';}
void TimeStamp_SET_D(){	TimeStamp_tt[(TimeStamp_ntt)&0x000000FF] = *TimeStamp_CNT;	TimeStamp_aa[(TimeStamp_ntt++)&0x000000FF] = 'D';}
void TimeStamp_SET_E(){	TimeStamp_tt[(TimeStamp_ntt)&0x000000FF] = *TimeStamp_CNT;	TimeStamp_aa[(TimeStamp_ntt++)&0x000000FF] = 'E';}
void TimeStamp_SET_F(){	TimeStamp_tt[(TimeStamp_ntt)&0x000000FF] = *TimeStamp_CNT;	TimeStamp_aa[(TimeStamp_ntt++)&0x000000FF] = 'F';}

void TimeStamp_SET_char(char A){
	TimeStamp_tt[(TimeStamp_ntt)&0x000000FF] = *TimeStamp_CNT;
	TimeStamp_aa[(TimeStamp_ntt++)&0x000000FF] = A;
}

void TimeStamp_SET_uint8(uint8_t A){
	TimeStamp_tt[(TimeStamp_ntt)&0x000000FF] = *TimeStamp_CNT;
	TimeStamp_aa[(TimeStamp_ntt++)&0x000000FF] = A;
}

uint32_t FindTimeStamp(uint8_t A)//by Mahr
{
	uint_fast8_t saved_pos = (TimeStamp_ntt-1)&0x000000FF; //save current position in case of interrupt and convert to index
	uint_fast8_t x = saved_pos;
	do
	{
		if(TimeStamp_aa[x&0x000000FF] == A) return TimeStamp_tt[x&0x000000FF];
	}
	while((--x&0x000000FF) != saved_pos);
	return 0;
}


// example for 8 stamps for 24 tick for each
// TimeStamp_tt[(TimeStamp_ntt++)&0x000000FF]= *TimeStamp_CNT;
// TimeStamp_tt[(TimeStamp_ntt++)&0x000000FF]= *TimeStamp_CNT;
// TimeStamp_tt[(TimeStamp_ntt++)&0x000000FF]= *TimeStamp_CNT;
// TimeStamp_tt[(TimeStamp_ntt++)&0x000000FF]= *TimeStamp_CNT;
// TimeStamp_tt[(TimeStamp_ntt++)&0x000000FF]= *TimeStamp_CNT;
// TimeStamp_tt[(TimeStamp_ntt++)&0x000000FF]= *TimeStamp_CNT;
// TimeStamp_tt[(TimeStamp_ntt++)&0x000000FF]= *TimeStamp_CNT;
// TimeStamp_tt[(TimeStamp_ntt++)&0x000000FF]= *TimeStamp_CNT;

// old versions of functions...
//void TimeStamp_SET_B(){	TimeStamp_tt[TimeStamp_ntt] = TimeStamp_htim->Instance->CNT;	TimeStamp_aa[TimeStamp_ntt] = 'B';	TimeStamp_ntt++; }
//void TimeStamp_SET_C(){	TimeStamp_tt[TimeStamp_ntt] = TimeStamp_htim->Instance->CNT;	TimeStamp_aa[TimeStamp_ntt] = 'C';	TimeStamp_ntt++; }
//void TimeStamp_SET_D(){	TimeStamp_tt[TimeStamp_ntt] = TimeStamp_htim->Instance->CNT;	TimeStamp_aa[TimeStamp_ntt] = 'D';	TimeStamp_ntt++; }
//void TimeStamp_SET_E(){	TimeStamp_tt[TimeStamp_ntt] = TimeStamp_htim->Instance->CNT;	TimeStamp_aa[TimeStamp_ntt] = 'E';	TimeStamp_ntt++; }
//void TimeStamp_SET_F(){	TimeStamp_tt[TimeStamp_ntt] = TimeStamp_htim->Instance->CNT;	TimeStamp_aa[TimeStamp_ntt] = 'F';	TimeStamp_ntt++; }
//
//void TimeStamp_SET_char(char A){
//	TimeStamp_tt[TimeStamp_ntt] = TimeStamp_htim->Instance->CNT;
//	TimeStamp_aa[TimeStamp_ntt] = A;
//	TimeStamp_ntt++;
//}
//
//void TimeStamp_SET_uint8(uint8_t A){
//	TimeStamp_tt[TimeStamp_ntt] = TimeStamp_htim->Instance->CNT;
//	TimeStamp_aa[TimeStamp_ntt] = A;
//	TimeStamp_ntt++;
//}



#ifdef __cplusplus
}
#endif

#endif /* __TIMESTAMP_H */
