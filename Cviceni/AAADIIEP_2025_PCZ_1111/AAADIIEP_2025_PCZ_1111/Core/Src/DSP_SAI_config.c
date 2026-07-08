/*
 * DSP_SAI_config.h
 *
 *  Created on: Feb 23, 2024
 *      Author: Mirek
 *
 *      2025-04-02 uprava na .h a .c
 *      DSP_SAI_config.c
 *
 */

#ifndef INC_DSP_SAI_CONFIG_C_
#define INC_DSP_SAI_CONFIG_C_

#include "DSP_SAI_config.h"

SAI_HandleTypeDef *hsaiTr, *hsaiRe;

int32_t buf_SAI_RX[2*2*BLOCK_SIZE_MAX];//input buffer - read samples - stereo signal (2 samples - Left and Right), circular mode - two callbacks
int32_t buf_SAI_TX[2*2*BLOCK_SIZE_MAX];//output buffer same like input
float sample_L[BLOCK_SIZE_MAX];
float sample_R[BLOCK_SIZE_MAX];
float sample_L1[BLOCK_SIZE_MAX];
float sample_R1[BLOCK_SIZE_MAX];
//float sample_L2[BLOCK_SIZE_MAX];
//float sample_R2[BLOCK_SIZE_MAX];

// pro Fs=16kHz zaznam leveho kanalu 1s (vyroba ozveny)
#define N_SAMPLE_L_LONG BLOCK_SIZE_MAX + 16000
float sample_L_Long[N_SAMPLE_L_LONG];
int n_Sample_L_Long; // pozice aktualniho konce bufferu

int status_SAI_Restart;
uint32_t BlockSize;

arm_status status;
float pState_FIR_L[BLOCK_SIZE_MAX + MAX_TAP_LEN +1];
float pState_FIR_R[BLOCK_SIZE_MAX + MAX_TAP_LEN +1];
float pState_IIR_L[MAX_SOS_LEN * 4];
float pState_IIR_R[MAX_SOS_LEN * 4];

uint8_t status_DSP_OVERLOAD;

int i, i2;
int Status_Process_DSP_Complete;
int Status_Proces_Type_L;//0... FIR, 1... IIR, 2....FFT
int32_t tmp_sample;

int BL1, BR1;
float B1_R[MAX_TAP_LEN];//float B1[5]={0.2, 0.2, 0.2, 0.2, 0.2};
float B1_L[MAX_TAP_LEN];
float IIR_coef_L[MAX_SOS_LEN * 5];
float IIR_coef_R[MAX_SOS_LEN * 5];

arm_fir_instance_f32 S_L_FIR, S_R_FIR;
arm_biquad_casd_df1_inst_f32 S_L_IIR, S_R_IIR;

DSP_SAI_ConfigDef hDSC1;

__weak void DSP_SAI_ProcessSamples(DSP_SAI_ConfigDef *hDSC)
{
	hDSC->Status_Process_DSP_Complete[0]=0;
	//left samples
	arm_fir_f32(hDSC->S_L_FIR, hDSC->sample_L_in, hDSC->sample_L_out, hDSC->BlockSize[0]);
	//	//right samples
	arm_fir_f32(hDSC->S_R_FIR, hDSC->sample_R_in, hDSC->sample_R_out, hDSC->BlockSize[0]);

	hDSC->Status_Process_DSP_Complete[0]=1;

}

void DSP_SAI_Start(void)
{
	HAL_SAI_Transmit_DMA(hsaiTr,(uint8_t *) buf_SAI_TX, 4*BlockSize);//Size is n samples (Data wdth is word (4 bytes - 32 bits))
	HAL_SAI_Receive_DMA(hsaiRe,(uint8_t *) buf_SAI_RX, 4*BlockSize); //
}
void DSP_SAI_Stop(void)
{
	HAL_SAI_DMAStop(hsaiTr);
	HAL_SAI_DMAStop(hsaiRe);
}
DSP_SAI_ConfigDef* DSP_SAI_Get_ConfigDef(void)
{
	return (&hDSC1);
}
DSP_SAI_ConfigDef* DSP_SAI_Init(uint32_t BlockSize_, SAI_HandleTypeDef *hsaiTr_, SAI_HandleTypeDef *hsaiRe_)//, SAI_HandleTypeDef hsai_Tr, SAI_HandleTypeDef hsai_Re)
{
	if(!BlockSize_) BlockSize_=1;
	if(BlockSize_ > BLOCK_SIZE_MAX) BlockSize_ = BLOCK_SIZE_MAX;
	BlockSize=BlockSize_;
	BL1=1;
	B1_L[0]=1; B1_R[0]=1;//float B1[5]={0.2, 0.2, 0.2, 0.2, 0.2};
	arm_fir_init_f32(&S_L_FIR, BL1, B1_L, pState_FIR_L, BlockSize);//init filter before processsing
	arm_fir_init_f32(&S_R_FIR, BL1, B1_R, pState_FIR_R, BlockSize);//init filter before processsing

	IIR_coef_L[0]=1; IIR_coef_L[1]=0; IIR_coef_L[2]=0;IIR_coef_L[3]=0;IIR_coef_L[4]=0;
	arm_biquad_cascade_df1_init_f32(&S_L_IIR, 1, IIR_coef_L, pState_IIR_L );

	status_SAI_Restart=0; n_Sample_L_Long=0;
	Status_Proces_Type_L=0;

	hsaiTr=hsaiTr_; hsaiRe=hsaiRe_;

	hDSC1.BlockSize = &BlockSize;
	hDSC1.S_L_FIR = &S_L_FIR;
	hDSC1.S_R_FIR = &S_R_FIR;
	hDSC1.S_L_IIR = &S_L_IIR;
	hDSC1.S_R_IIR = &S_R_IIR;
	hDSC1.sample_L_in = sample_L;
	hDSC1.sample_L_out = sample_L1;
	hDSC1.sample_R_in = sample_R;
	hDSC1.sample_R_out = sample_R1;
	hDSC1.Status_Process_DSP_Complete = &Status_Process_DSP_Complete;

	DSP_SAI_Start();
	return(&hDSC1);
}

int DSP_SAI_Set_FIR(float *B_coef_Left, int B_len_Left, float *B_coef_Right, int B_len_Right )
{
	if(B_len_Left > MAX_TAP_LEN) return -1;
	if(B_len_Right > MAX_TAP_LEN) return -2;
	BL1 = B_len_Left;
	BR1 = B_len_Right;
	memcpy(B1_L, B_coef_Left, BL1 * sizeof(float));
	memcpy(B1_R, B_coef_Right, BR1 * sizeof(float));
	arm_fir_init_f32(&S_L_FIR, BL1, B1_L, pState_FIR_L, BlockSize);//init filter before processsing
	arm_fir_init_f32(&S_R_FIR, BR1, B1_R, pState_FIR_R, BlockSize);//init filter before processsing
	return 0;
}
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
	status_DSP_OVERLOAD++; DSP_SAI_ProcessSamples(&hDSC1);
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
	status_DSP_OVERLOAD--; DSP_SAI_ProcessSamples(&hDSC1);
}

void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{	UNUSED(hsai);
	if(Status_Process_DSP_Complete)
	{
		for(i=0, i2=0; i<BlockSize; i++, i2++, i2++)
		{
			buf_SAI_TX[i2] = sample_L1[i];
			buf_SAI_TX[i2+1] = sample_R1[i];
		}
		Status_Process_DSP_Complete=0;
	}
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

void DSP_SAI_Set_Sample_L_Long(void)
{
	int n = BlockSize;
	if((n+n_Sample_L_Long)>N_SAMPLE_L_LONG)
		n = N_SAMPLE_L_LONG - n_Sample_L_Long;
	memcpy(sample_L_Long + n_Sample_L_Long, sample_L, n * sizeof(float));
	if(n<BlockSize)
	{
		memcpy(sample_L_Long, sample_L + n, (BlockSize - n) * sizeof(float));
		n_Sample_L_Long = BlockSize - n;
	}
	else
		n_Sample_L_Long = n_Sample_L_Long + n;


}

float DSP_SAI_Get_Sample_L_Long(uint32_t nX)
{
	if(nX>N_SAMPLE_L_LONG) return 0;
	if(nX<n_Sample_L_Long) return sample_L_Long[n_Sample_L_Long - nX];
	if(nX == n_Sample_L_Long) return sample_L_Long[n_Sample_L_Long - nX];
	return sample_L_Long[N_SAMPLE_L_LONG + n_Sample_L_Long - nX];
}

#endif /* INC_DSP_SAI_CONFIG_C_ */
