/*
 * DSP_SAI_config.h
 *
 *  Created on: Feb 23, 2024
 *      Author: Mirek
 */

#ifndef INC_DSP_SAI_CONFIG_H_
#define INC_DSP_SAI_CONFIG_H_

#define BLOCK_SIZE_MAX 2048*2

int32_t buf_SAI_RX[2*2*BLOCK_SIZE_MAX];//input buffer - read samples - stereo signal (2 samples - Left and Right), circular mode - two callbacks
int32_t buf_SAI_TX[2*2*BLOCK_SIZE_MAX];//output buffer same like input
float sample_L[BLOCK_SIZE_MAX];
float sample_R[BLOCK_SIZE_MAX];
float sample_L1[BLOCK_SIZE_MAX];
float sample_R1[BLOCK_SIZE_MAX];
float sample_L2[BLOCK_SIZE_MAX];
float sample_R2[BLOCK_SIZE_MAX];

int status_SAI_Restart;

uint32_t BlockSize;

//#define MAX_TAP_LEN 100
//arm_fir_instance_f32 S_L_FIR, S_R_FIR;
//arm_biquad_casd_df1_inst_f32 S_L_IIR;
//
//arm_status status;
//float pState_FIR_L[BLOCK_SIZE_MAX + MAX_TAP_LEN +1];
//float pState_FIR_R[BLOCK_SIZE_MAX + MAX_TAP_LEN +1];
#define STATUS_DSP_OVERLOAD_DEFAULT 128;
uint8_t status_DSP_OVERLOAD;


extern void ProcessSamples();

int i, i2;
int Status_Process_DSP_Complete;
int32_t tmp_sample;
void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{  UNUSED(hsai);
	for(i=0, i2=0; i<BlockSize; i++, i2++, i2++)
	{
		//left samples
		if(0x00800000 & buf_SAI_RX[i2]) tmp_sample = (0xFF000000 | buf_SAI_RX[i2]); else tmp_sample = (0x00FFFFFF & buf_SAI_RX[i2]);
		sample_L[i]=tmp_sample;
		//right samples
		if(0x00800000 & buf_SAI_RX[i2+1]) tmp_sample=(0xFF000000 | buf_SAI_RX[i2+1]); else tmp_sample=(0x00FFFFFF & buf_SAI_RX[i2+1]);
		sample_R[i]=tmp_sample;
	}
	status_DSP_OVERLOAD++; ProcessSamples();
}
void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)// only one SAI used - checking is not necessary
{  UNUSED(hsai);// for compiler - no warnings :-]
	for(i=0, i2 = 2*BlockSize; i<BlockSize; i++, i2++, i2++)
	{
		//left samples
		if(0x00800000 & buf_SAI_RX[i2]) tmp_sample=(0xFF000000 | buf_SAI_RX[i2]); else tmp_sample=(0x00FFFFFF & buf_SAI_RX[i2]);
		sample_L[i]=tmp_sample;
		//right samples
		if(0x00800000 & buf_SAI_RX[i2+1]) tmp_sample=(0xFF000000 | buf_SAI_RX[i2+1]); else tmp_sample=(0x00FFFFFF & buf_SAI_RX[i2+1]);
		sample_R[i]=tmp_sample;
	}
	status_DSP_OVERLOAD--; ProcessSamples();
}
void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{	UNUSED(hsai);
	HAL_GPIO_WritePin(PIN2_GPIO_Port, PIN2_Pin, GPIO_PIN_SET);
	if(Status_Process_DSP_Complete)
	{
		for(i=0, i2=0; i<BlockSize; i++, i2++, i2++)
		{
			buf_SAI_TX[i2] = sample_L1[i];
			buf_SAI_TX[i2+1] = sample_R1[i];
		}
		Status_Process_DSP_Complete=0;
	}
	HAL_GPIO_WritePin(PIN2_GPIO_Port, PIN2_Pin, GPIO_PIN_RESET);
}
void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{	UNUSED(hsai);
	if(Status_Process_DSP_Complete)
	{
		for(i=0, i2= 2*BlockSize; i<BlockSize; i++, i2++, i2++)
		{
			buf_SAI_TX[i2] = sample_L1[i];
			buf_SAI_TX[i2+1] = sample_R1[i];
		}
		Status_Process_DSP_Complete=0;
	}
}



#endif /* INC_DSP_SAI_CONFIG_H_ */
