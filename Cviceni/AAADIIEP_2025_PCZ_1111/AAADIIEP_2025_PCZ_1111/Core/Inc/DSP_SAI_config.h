/*
 * DSP_SAI_config.h
 *
 *  Created on: Feb 23, 2024
 *      Author: Mirek
 *
 *      2025-04-02 uprava na .h a .c
 *
 */

#ifndef INC_DSP_SAI_CONFIG_H_
#define INC_DSP_SAI_CONFIG_H_

//#include "main.h"
// hlavicka pro dany kontroler
#include "stm32l4xx_hal.h"
#include "arm_math.h"  // je nutne mit DSP knihovnu (napr. CMSIS)

#define BLOCK_SIZE_MAX 1024 //2048

#define MAX_TAP_LEN 600
#define MAX_SOS_LEN 100
#define STATUS_DSP_OVERLOAD_DEFAULT 128;


typedef struct
{
	float *sample_L_in;
	float *sample_R_in;
	float *sample_L_out;
	float *sample_R_out;
	uint32_t *BlockSize;

	arm_fir_instance_f32 *S_L_FIR;
	arm_fir_instance_f32 *S_R_FIR;
	arm_biquad_casd_df1_inst_f32 *S_L_IIR;
	arm_biquad_casd_df1_inst_f32 *S_R_IIR;

	int *Status_Process_DSP_Complete;

} DSP_SAI_ConfigDef;



void DSP_SAI_ProcessSamples(DSP_SAI_ConfigDef *hDSC);

void DSP_SAI_Start(void);
void DSP_SAI_Stop(void);
DSP_SAI_ConfigDef*  DSP_SAI_Init(uint32_t BlockSize_, SAI_HandleTypeDef *hsaiTr_, SAI_HandleTypeDef *hsaiRe_);//, SAI_HandleTypeDef hsai_Tr, SAI_HandleTypeDef hsai_Re)
int DSP_SAI_Set_FIR(float *B_coef_Left, int B_len_Left, float *B_coef_Right, int B_len_Right );
void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai);
void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai);// only one SAI used - checking is not necessary
void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai);
void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai);

void DSP_SAI_Set_Sample_L_Long(void);
float DSP_SAI_Get_Sample_L_Long(uint32_t nX);

#endif /* INC_DSP_SAI_CONFIG_H_ */
