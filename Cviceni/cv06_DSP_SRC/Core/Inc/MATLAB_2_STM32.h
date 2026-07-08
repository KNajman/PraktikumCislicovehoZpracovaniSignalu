/*
 * MATLAB_2_STM32.h
 *
 *  Created on: Oct 15, 2024
 *      Author: Mirek
 */

#ifndef INC_MATLAB_2_STM32_H_
#define INC_MATLAB_2_STM32_H_

#include "main.h"

#define MAX_TXRX_DATA 2*1024
uint32_t buf_M_RX[MAX_TXRX_DATA + 1];//+4bytes for head
uint32_t buf_M_TX[MAX_TXRX_DATA + 1];
uint32_t m2s_buf[MAX_TXRX_DATA];//
int s2m_Status;// transmit to Matlab
int m2s_Status;// received status 0...waiting for incomming data, 1...call m2s_Process, -1... init receiving, 2...n data , 3... xData
int m2s_ID;
int m2s_nData_in_bytes;
UART_HandleTypeDef *huart2MTLB;


extern void DataReceive_MTLB_Callback(uint16_t iD, uint32_t * xData, uint16_t nData_in_values);

int DataTransmit2MTLB(uint16_t iD, uint8_t * xData, uint16_t nData_in_values)
{
	if(s2m_Status) return -1;
	if((sizeof(buf_M_TX)-4)<(nData_in_values*4)) return -2;
	s2m_Status=1;
	((uint16_t *) buf_M_TX)[0] = iD;
	((uint16_t *) buf_M_TX)[1] = nData_in_values;
	if(nData_in_values>0) memcpy(buf_M_TX+1, xData, nData_in_values*4);
	HAL_UART_Transmit_DMA(huart2MTLB,(uint8_t *) buf_M_TX, nData_in_values*4 + 4);

	return 0;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{	if(huart == huart2MTLB)	{		s2m_Status=0;	} }

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if( huart== huart2MTLB)
  {
	  if(m2s_Status==0)//new message
	  {
		  m2s_ID = ((uint16_t *) buf_M_RX)[0] ;
		  if(m2s_ID == 0)
		  {
			  m2s_Status= -1;
			  return;
		  }
		  m2s_nData_in_bytes = ((uint16_t *) buf_M_RX)[1] *4; //
		  if(m2s_nData_in_bytes == 0)
		  {
			  m2s_Status=1;
			  return;
		  }
		  m2s_Status=2;//wait for xData
		  return;
	  }
	  if(m2s_Status==3)//xData
	  {
		  memcpy(m2s_buf, buf_M_RX, m2s_nData_in_bytes);
		  m2s_Status=1;
		  return;
	  }
  }
}

void m2s_Process(void)//called from inf. loop
{
	if(!m2s_Status) return;//the most often ....
	if(m2s_Status==1)
	{
		DataReceive_MTLB_Callback(m2s_ID, m2s_buf, m2s_nData_in_bytes/4);
		HAL_UART_Receive_DMA(huart2MTLB,( uint8_t *) buf_M_RX, 4);
		m2s_Status = 0;
		return;
	}
	if(m2s_Status==2)
	{
		if(m2s_nData_in_bytes> (MAX_TXRX_DATA * sizeof(uint32_t)))
		{
			m2s_Status= -1;//ERROR
			m2s_Status= -2;//ERROR
			return;
		}
		m2s_Status= 3;
		HAL_UART_Receive_DMA(huart2MTLB,( uint8_t *) buf_M_RX, m2s_nData_in_bytes);
		return;
	}

	if(m2s_Status== -1)//init receiving new message from matlab
	{
		HAL_UART_Receive_DMA(huart2MTLB,( uint8_t *) buf_M_RX, 4);
		m2s_Status = 0;
		return;
	}

}





#endif /* INC_MATLAB_2_STM32_H_ */
