/*
 * Copyright (c) 2021 by Future Design Systems
 * All rights reserved.
 * http://www.future-ds.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "defines.h"
#include "etc.h"
#include "confmc.h"
#include "hw_defines.h"
#include "load_params.h"

// PARAMS @ AID ///////////////////////////////////////
#if !defined(DTYPE)
#define DTYPE float
#endif

#if !defined(QTYPE)
#define QTYPE long long
#endif

#define DDR3_MEMORY_ADDRESS    0x80000000

#define DBUG            // for debug @ AID
///////////////// QUANTIZATION BITS //////////////////
// weights
extern const int w_nbit = 8;
extern const int w_ibit = 0;
extern const int w_fbit = 8;
// bias, scale
extern const int s_nbit = 16;
extern const int b_nbit = 16;
const int s_ibit = 8;
const int s_fbit = 8;
const int b_ibit = 8;
const int b_fbit = 8;
// activation
extern const int a_nbit = 8;
extern const int a_ibit = 4;
extern const int a_fbit = 4;
//////////////////////////////////////////////////////////
extern const int conv_in_channel  [9] = {  3, 16, 32, 64,128,256, 512,1024, 512};
extern const int conv_out_channel [9] = { 16, 32, 64,128,256,512,1024, 512,  35};
extern const int weight_offset[9] = {0,   432,  5040,  23472, 97200,  392112,  1571760, 6290352, 11008944};
extern const int bias_offset[9]   = {0,   16,   48,    112,   240,    496,     1008,    2032,    2544    };
extern const int bias_size[9]     = {16,  32,   64,    128,   256,    512,     1024,    512,     35      };
//////////////////////////////////////////////////////////

/* @brief do_load_params() download parameters to the HW.
 *
 * @param[out]   global
 *
 * @return 0 on success, othewise return negative number.
 */
int do_load_params( global_t *global )
{
    // Quantization & Reordering Start  @ AID
    QTYPE *weight = NULL;
    weight = (QTYPE*)calloc(global->param_size_weights/sizeof(DTYPE)+256, sizeof(QTYPE));
    QTYPE *bias = NULL;
    bias = (QTYPE*)calloc(global->param_size_biases/sizeof(DTYPE)+256, sizeof(QTYPE));
    QTYPE *scale = NULL;
    scale = (QTYPE*)calloc(global->param_size_scales/sizeof(DTYPE)+256, sizeof(QTYPE));
    
    uniform_quantize(global->param_value_weights, weight, global->param_size_weights);
    fold_and_quantize(global->param_value_biases, global->param_value_scales, global->param_value_means
                      , global->param_value_variances, bias, scale);

    // Reordering 
    int8_t* reorder_weight;
    reorder_weight = (int8_t*)calloc(11041856, sizeof(int8_t));
    short* scale_bias_output;
    scale_bias_output = (short*)calloc(10316, sizeof(short));
    unsigned int weight_size = 0;
    unsigned int norm_size = 0;

    for(int stage = 0; stage < 9; stage++){
        int To = (stage==0) ? 16 : 32;
	    int nTo = (stage==8) ? conv_out_channel[stage]/To + 1 : conv_out_channel[stage]/To;
        int Ti = (conv_in_channel[stage] < 32) ? conv_in_channel[stage] : 32;
	    int nTi = conv_in_channel[stage]/Ti;
        int kernel = (stage==8) ? 1 : 9;
        int start;
        int last = 0;
        
        // Weight
	    for (int i = 0; i < nTo; i++){
		    if (conv_out_channel[stage]%To && i == nTo-1) last = 1;
		    for (int j = 0; j < nTi; j++){
		        //printf("last:%d, now nTi:%d, nTi:%d\n", last, j, nTi);
		        for(int k = 0; k < kernel; k ++){
			        for (int m = 0; m < To; m++){
			            for (int n = 0; n < Ti; n++){
				            if (stage == 0){
				                start = weight_offset[stage] + k + kernel*n + (kernel*3*m);
				                reorder_weight[weight_size] = (weight[start] - 1) / 2;
				                if (n == 2){
					                weight_size ++; 
				                    reorder_weight[weight_size] = 0;
                                }
                            }
				            else if (last && m > 2){
				                reorder_weight[weight_size] = 0; 
				            }
				            else {
				                start = weight_offset[stage] + k + (kernel*n) + (kernel*conv_in_channel[stage]*m) + (j*kernel*Ti) + (i*kernel*conv_in_channel[stage]*To);
				                reorder_weight[weight_size] = (weight[start] - 1) / 2;
                            }
				            weight_size ++;
	                    }
                    }
                }
            }
        }

        // Scale && Bias
        for (int i = 0; i < conv_out_channel[stage]; i++){
            if(stage == 8){
                scale_bias_output[norm_size+2*i] = 1;
                scale_bias_output[norm_size+2*i+1] = bias[bias_offset[stage]+i];
            }
            else{
                scale_bias_output[norm_size+2*i] = scale[bias_offset[stage]+i];
                scale_bias_output[norm_size+2*i+1] = bias[bias_offset[stage]+i];
            }
        }
        norm_size += conv_out_channel[stage]*2;
        // [Layer 9] output channel set (35 + 29)
        if(stage == 8){
            for (int i = 0; i < 29; i ++){
                scale_bias_output[norm_size + 2*i] = 0;
                scale_bias_output[norm_size + 2*i+1] = 0;
            }
            norm_size += 29*2;
        }
    }

#ifdef DBUG
    printf("\nDEBUG MODE >>>> Write weight and scale_bias (./debug/*bin)\n");
    printf("weight_size : %d, norm_size : %d\n", weight_size, norm_size);
    FILE *fp;
    fp = fopen("debug/weight_kernel.bin", "wb");
    fwrite(reorder_weight, weight_size*sizeof(int8_t), 1, fp);
    fclose(fp);

    fp = fopen("debug/scale_bias.bin", "wb");
    fwrite(scale_bias_output, norm_size*sizeof(short), 1, fp);
    fclose(fp);
#endif
    // End @ AID

    int burst_len = 512;
    confmc_burst_write((unsigned int*)reorder_weight,  DDR3_MEMORY_ADDRESS + 0x0,   weight_size/4,   burst_len);
    confmc_burst_write((unsigned int*)scale_bias_output,  DDR3_MEMORY_ADDRESS + 0x03000000,   norm_size/2,   burst_len);
    // confmc_burst_write((unsigned int*)(global->param_value_weights),  MEM_ADDR_WEIGHTS,   global->param_size_weights/sizeof(unsigned int),   burst_len);
    // confmc_burst_write((unsigned int*)(global->param_value_biases),   MEM_ADDR_BIASES,    global->param_size_biases/sizeof(unsigned int),    burst_len);
    // confmc_burst_write((unsigned int*)(global->param_value_scales),   MEM_ADDR_SCALES,    global->param_size_scales/sizeof(unsigned int),    burst_len);
    // confmc_burst_write((unsigned int*)(global->param_value_means),    MEM_ADDR_MEANS,     global->param_size_means/sizeof(unsigned int),     burst_len);
    // confmc_burst_write((unsigned int*)(global->param_value_variances),MEM_ADDR_VARIANCES, global->param_size_variances/sizeof(unsigned int), burst_len);

#if defined(RIGOR)
if (0) {
    int diff;
    diff = confmc_compare((unsigned int*)(global->param_value_weights),  MEM_ADDR_WEIGHTS,   global->param_size_weights/sizeof(unsigned int));
    myInfo("weights  : %d items diff\n", diff);
    diff = confmc_compare((unsigned int*)(global->param_value_biases),   MEM_ADDR_BIASES,    global->param_size_biases/sizeof(unsigned int));
    myInfo("biases   : %d items diff\n", diff);
    diff = confmc_compare((unsigned int*)(global->param_value_scales),   MEM_ADDR_SCALES,    global->param_size_scales/sizeof(unsigned int));
    myInfo("scales   : %d items diff\n", diff);
    diff = confmc_compare((unsigned int*)(global->param_value_means),    MEM_ADDR_MEANS,     global->param_size_means/sizeof(unsigned int));
    myInfo("means    : %d items diff\n", diff);
    diff = confmc_compare((unsigned int*)(global->param_value_variances),MEM_ADDR_VARIANCES, global->param_size_variances/sizeof(unsigned int));
    myInfo("variances: %d items diff\n", diff);
}

    #if 0
    myInfo("weight addr=0x%08X size=%d\n", MEM_ADDR_WEIGHTS,   global->param_size_weights/sizeof(unsigned int));
    myInfo("bias   addr=0x%08X size=%d\n", MEM_ADDR_BIASES,    global->param_size_biases/sizeof(unsigned int));
    myInfo("scale  addr=0x%08X size=%d\n", MEM_ADDR_SCALES,    global->param_size_scales/sizeof(unsigned int));
    myInfo("mean   addr=0x%08X size=%d\n", MEM_ADDR_MEANS,     global->param_size_means/sizeof(unsigned int));
    myInfo("var    addr=0x%08X size=%d\n", MEM_ADDR_VARIANCES, global->param_size_variances/sizeof(unsigned int));
    #endif
#endif

    return 0;
}


/*
 * Revision history
 *
 * 2021.04.13: started by Ando Ki (adki@future-ds.com)
 */

void uniform_quantize(DTYPE* weight_d, QTYPE* weight_q, int weight_size){

    DTYPE step = powf(2, -1 * w_fbit);
    QTYPE pos_end = powf(2, w_nbit) - 1; //255
    QTYPE neg_end = -1 * pos_end; // -255

    int i = 0;
    while(i < weight_size/4){
	    weight_q[i] = 2 * (QTYPE)roundf(weight_d[i] / step + 0.5) - 1;
        if(weight_q[i] > pos_end)
            weight_q[i] = pos_end;
        else if(weight_q[i] < neg_end)
            weight_q[i] = neg_end;
	    i++;
    }
}

void fold_and_quantize(DTYPE* bias_d, DTYPE* scale_d, DTYPE* rolling_mean_d, DTYPE* rolling_variance_d, QTYPE* bias_q, QTYPE* scale_q){
    // BN folding
    int i = 0;
    while(i < 8){       // from layer 1 ~ layer 8 (layer 9 excluded: no batch norm)
        for(int j = 0; j < bias_size[i]; j++){
            DTYPE scale_new;
            scale_new = scale_d[bias_offset[i] + j] / sqrtf(rolling_variance_d[bias_offset[i] + j] + 1e-5);
            bias_d[bias_offset[i] + j] = bias_d[bias_offset[i] + j] - scale_d[bias_offset[i] + j] * rolling_mean_d[bias_offset[i] + j] / sqrtf(rolling_variance_d[bias_offset[i] + j] + 1e-5);
            scale_d[bias_offset[i] + j] = scale_new;
        }
	    i++;
    }

    // scale, bias quantization
    QTYPE s_qfactor = powf(2, s_fbit);
    QTYPE s_nfactor = powf(2, s_nbit);
    QTYPE s_max = s_nfactor / 2 - 1;
    QTYPE s_min = s_max - s_nfactor + 1;

    QTYPE b_qfactor = powf(2, b_fbit);
    QTYPE b_nfactor = powf(2, b_nbit);
    QTYPE b_max = b_nfactor / 2 - 1;
    QTYPE b_min = b_max - b_nfactor + 1;

    int k = 0;
    while(k < bias_size[8]+bias_offset[8]){       // total 2579 (including convolution bias) - scale has trash values for convolution bias index
	    
        scale_q[k] = (QTYPE)roundf(scale_d[k] * s_qfactor);
        bias_q[k] = (QTYPE)roundf(bias_d[k] * b_qfactor);

        if(scale_q[k] > s_max)
            scale_q[k] = s_max;
        if(scale_q[k] < s_min)
            scale_q[k] = s_min;
            
        if(bias_q[k] > b_max)
            bias_q[k] = b_max;
        if(bias_q[k] < b_min)
            bias_q[k] = b_min;

	    k++;
    }
}