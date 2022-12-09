/*
 * Copyright (c) 2021 by Future Design Systems
 * All rights reserved.
 * http://www.future-ds.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include "defines.h"
#include "etc.h"
#include "confmc.h"
#include "hw_defines.h"
#include "inference.h"

// ADD @ AID
#define IMAGE_SIZE_ (IMAGE_HEIGHT*IMAGE_WIDTH*(IMAGE_COLORS+1))
#define RESULT_SIZE_ RESULT_WIDTH*RESULT_HEIGHT*32 

#define DDR3_MEMORY_ADDRESS    0x80000000
static const unsigned int layer_num[9] = {1,2,3,4,5,6,7,8,9};

//#define DBUG

static int do_normalize_image_aid( image_uchar_t *image_uchar
                             , u_int8_t *image_int );
// END @ AID

static int do_normalize_image( image_uchar_t *image_uchar
                             , image_float_t *image_float );
static int do_store_result( char  *file_result // path+basename
                          , float *result);

/* @brief do_inference()
 *   - wait until the engine is ready (i.e., idle)
 *   - fill all registers (i.e., CSR)
 *   - let the engine start
 *   - wait until the engine is done.
 *
 * @return 0 on success, othewise return negative number.
 */
int do_inference( char     *dir_results
                , global_t *global )
{
    //int burst_len = 512;
    int burst_len = 4096;

    image_float_t  image;
    image.w = IMAGE_WIDTH;
    image.h = IMAGE_HEIGHT;
    image.data = (float*)malloc(IMAGE_COLORS*IMAGE_HEIGHT*IMAGE_WIDTH*sizeof(float));

    int framesize = IMAGE_WIDTH*IMAGE_HEIGHT;
    int rframesize = RESULT_WIDTH*RESULT_HEIGHT;
    // END @ AID

    // use your data type not 'float'
    float *results = (float *)malloc(RESULT_SIZE*sizeof(float));
    short results_[10816];          // 13 x 13 x 64   (64 channel output)
    u_int8_t input_image[692224];    // 416 x 416 x 4  (Quantized input image)
    for (int idx=0; idx<global->images_num; idx++) {
        printf("[INFO.] Inferencing image : %s\n", global->images_name[idx]);

        long long time_delta = timestamp();

        // pixel data conversion
        do_normalize_image_aid(&global->images_uchar[idx] // scaled image
                          ,&input_image ); // normalized image

        // Image Qunatization & Reordering @ AID
        #ifdef DBUG
        printf("\nDEBUG MODE >>>> Write input image (./debug/*bin)\n");
        printf("image_size : %d\n", 692224);
        FILE *fp;
        fp = fopen("debug/input_image.bin", "wb");
        fwrite(input_image, 692224*sizeof(int8_t), 1, fp);
        fclose(fp);
        // exit(0);
        #endif

        // Load Image
	
        confmc_burst_write( (unsigned int*)(input_image)
                          , DDR3_MEMORY_ADDRESS + 0x03006000
                          , IMAGE_SIZE_/4 // unit should be 4-word, not num of bytes
                          , burst_len);
        // END @ AID

        // load image
        // confmc_burst_write( (unsigned int*)(image.data)
        //                   , MEM_ADDR_INPUT_IMAGE
        //                   , IMAGE_SIZE // unit should be 4-word, not num of bytes
        //                   , burst_len);

        // carry out inference
        for (int stage=0; stage<9; stage++) {
            do_run_engine_aid( idx, stage, global );
        }

        // Image Qunatization & Reordering @ AID
        // Upload Results
        confmc_burst_read( (unsigned int*)results_
                          , DDR3_MEMORY_ADDRESS + 0x03106000
                          , RESULT_SIZE_ // unit should be 4-word
                          , burst_len);

        // Post-Processing
        int class_cnt = 0;
        for(int nti = 0; nti < 2; nti++){
            for(int i = 0; i < RESULT_HEIGHT; i++){
                for(int j = 0; j < RESULT_WIDTH; j++){
                    for(int k = 0; k < 32; k++){
                        if(nti == 1 && k < 3){
                            results[(k+32*nti)*rframesize + i*RESULT_HEIGHT + j] = (float)results_[class_cnt] / powf(2, 8);
                        }   
                        else if(nti == 0){
                            results[(k+32*nti)*rframesize + i*RESULT_HEIGHT + j] = (float)results_[class_cnt] / powf(2, 8);
                        }   
                        class_cnt++;
                    }
                }
            }
        }

        #ifdef DBUG
        printf("\nDEBUG MODE >>>> Write output results (./debug/*bin)\n");
        printf("results : %d\n", class_cnt);
        // FILE *fp;
        fp = fopen("debug/DEBUG_output.txt", "wb");
        // short res;
        for (int i = 0; i < RESULT_SIZE_*2; i++){
            // res = ((unsigned short*)results_)[i];
            fprintf(fp, "%04x\n", results_[i]);
        }
        fclose(fp);
        // exit(0);
        #endif

        // END @AID
        // upload results
        // confmc_burst_read( (unsigned int*)results
        //                   , MEM_ADDR_RESULT
        //                   , RESULT_SIZE // unit should be 4-word
        //                   , burst_len);

        // results data conversion if required.
        // do_convert_results(); // fixed-point to floating-point
        memcpy((void*)(global->results[idx]), (void*)results, RESULT_SIZE*sizeof(float));

        global->time_elapse += (timestamp() - time_delta);

        // store inference results
        do_store_result( global->results_name[idx] // path+basename
                       , global->results[idx]);
        global->results_done++;
    }

    return 0;
}

/* @brief do_normalize_image()
 *        - get pixe data, which are unsigned char (i.e., 8-bit pre color)
 *        - prepare pixel buffers for float
 *        - noramize pixel data: [0:255]->[0.0:1.0]
 *
 * @param[in]    image_uchar
 * @param[in]    image_flat
 *
 * @return 0 on success, othewise return negative number.
 */
static int do_normalize_image( image_uchar_t *image_uchar
                             , image_float_t *image_float )
{
    int status=0;
    image_float->w = image_uchar->w;
    image_float->h = image_uchar->h;
    image_float->c = image_uchar->c;
    // do not free "float_img" unless error occurs.
    float *float_img = (float *)malloc(sizeof(float)*image_uchar->c*image_uchar->h*image_uchar->w);
    int idx=0;
    float *pt_float = float_img;
    unsigned char *pt_uchar = image_uchar->data;
    for (int idx=0; idx<(image_uchar->c*image_uchar->h*image_uchar->w); idx++) {
         pt_float[idx]  = ((float)pt_uchar[idx])/255.0;
    }

cleanup:
    if (status!=0) {
        free(float_img);
    } else {
        if (image_float!=NULL) {
            image_float->data = (float *)float_img;
        } else {
            free(float_img);
        }
    }
    return status;
}

// store 13x13x... inference results.
static int do_store_result( char  *file_result // path+basename
                          , float *result
                          )
{
    int status = 0;
    FILE *fp = fopen(file_result, "wb");
    if (fp==NULL) {
        myError("%s file open error.\n", file_result);
        status = -1; goto cleanup;
    }
    int ret = fwrite((void*)result, sizeof(float), RESULT_SIZE, fp);
    if (ret!=RESULT_SIZE) {
        myError("%s file write number mis-match.\n", file_result);
        status = -1; goto cleanup;
    }

cleanup:
    if (fp!=NULL) fclose(fp);

    return status;
}

static const unsigned int  modl_in_data     [9] = {MEM_ADDR_INPUT_IMAGE, MEM_ADDR_BUFFER_POOL, MEM_ADDR_BUFFER_POOL, MEM_ADDR_BUFFER_POOL, MEM_ADDR_BUFFER_POOL, MEM_ADDR_BUFFER_POOL, MEM_ADDR_BUFFER_POOL, MEM_ADDR_BUFFER_ACTI, MEM_ADDR_BUFFER_ACTI};
static const unsigned int  conv_buffer      [9] = {MEM_ADDR_BUFFER_CONV, MEM_ADDR_BUFFER_CONV, MEM_ADDR_BUFFER_CONV, MEM_ADDR_BUFFER_CONV, MEM_ADDR_BUFFER_CONV, MEM_ADDR_BUFFER_CONV, MEM_ADDR_BUFFER_CONV, MEM_ADDR_BUFFER_CONV, MEM_ADDR_RESULT};
static const unsigned int  conv_in_size     [9] = {416,208,104, 52, 26, 13,  13,  13,  13};
static const unsigned int  conv_out_size    [9] = {416,208,104, 52, 26, 13,  13,  13,  13};
static const unsigned int  conv_in_channel  [9] = {  3, 16, 32, 64,128,256, 512,1024, 512};
static const unsigned int  conv_out_channel [9] = { 16, 32, 64,128,256,512,1024, 512,  35};

static const unsigned int  norm_buffer      [8] = {MEM_ADDR_BUFFER_NORM, MEM_ADDR_BUFFER_NORM, MEM_ADDR_BUFFER_NORM, MEM_ADDR_BUFFER_NORM, MEM_ADDR_BUFFER_NORM, MEM_ADDR_BUFFER_NORM, MEM_ADDR_BUFFER_NORM, MEM_ADDR_BUFFER_NORM};
static const unsigned int  norm_in_size     [8] = {416*416,208*208,104*104,52*52,26*26,13*13,13*13,13*13};
static const unsigned int  norm_weight_size [8] = { 16, 32, 64,128,256,512,1024,512};
static const unsigned int  norm_bias_size   [8] = { 16, 32, 64,128,256,512,1024,512};
static const unsigned int  norm_in_channel  [8] = { 16, 32, 64,128,256,512,1024,512};

static const unsigned int  acti_buffer      [8] = {MEM_ADDR_BUFFER_ACTI, MEM_ADDR_BUFFER_ACTI, MEM_ADDR_BUFFER_ACTI, MEM_ADDR_BUFFER_ACTI, MEM_ADDR_BUFFER_ACTI, MEM_ADDR_BUFFER_ACTI, MEM_ADDR_BUFFER_ACTI, MEM_ADDR_BUFFER_ACTI};
static const unsigned int  acti_size        [8] = {416*416,208*208,104*104,52*52,26*26,13*13,13*13,13*13};
static const unsigned int  acti_channel     [8] = { 16, 32, 64,128,256,512,1024,512};

static const unsigned int  pool_buffer      [6] = {MEM_ADDR_BUFFER_POOL, MEM_ADDR_BUFFER_POOL, MEM_ADDR_BUFFER_POOL, MEM_ADDR_BUFFER_POOL, MEM_ADDR_BUFFER_POOL, MEM_ADDR_BUFFER_POOL};
static const unsigned int  pool_out_size    [6] = {208,104, 52, 26, 13, 13};
static const unsigned int  pool_in_size     [6] = {416,208,104, 52, 26, 13};
static const unsigned int  pool_channel     [6] = { 16, 32, 64,128,256,512};

static const unsigned int  norm_epsilon=0x3727C5AC; // 1e-5
static const unsigned int  acti_negative_slope=0x3DCCCCCD; // 0.1

static const int  weight_offset[9] = {0,   432,  5040,  23472, 97200,  392112,  1571760, 6290352, 11008944};
static const int  weight_size[9]   = {432, 4608, 18432, 73728, 294912, 1179648, 4718592, 4718592, 17920   };
static const int  bias_offset[9]   = {0,   16,   48,    112,   240,    496,     1008,    2032,    2544    };
static const int  bias_size[9]     = {16,  32,   64,    128,   256,    512,     1024,    512,     35      };
static const int  bn_offset[8]     = {0,   16,   48,    112,   240,    496,     1008,    2032             };
static const int  bn_size[8]       = {16,  32,   64,    128,   256,    512,     1024,    512              };

/* @brief do_csr_setup()
 *   - wait until the engine is ready (i.e., idle)
 *   - fill all registers (i.e., CSR)
 *   - let the engine start
 *   - wait until the engine is done.
 *
 * @return 0 on success, othewise return negative number.
 */
static int do_csr_setup( 
                        unsigned int conv_buffer
                       , unsigned int norm_buffer
                       , unsigned int acti_buffer
                       , unsigned int pool_buffer
                       , unsigned int modl_input_addr
                       , unsigned int conv_weight_addr
                       , unsigned int conv_bias_addr
                       , unsigned int conv_out_size
                       , unsigned int conv_in_size    
                       , unsigned int conv_kernel_size
                       , unsigned int conv_bias_size 
                       , unsigned int conv_in_channel 
                       , unsigned int conv_out_channel
                       , unsigned int conv_stride 
                       , unsigned int conv_padding  
                       , unsigned int norm_mean_addr
                       , unsigned int norm_var_addr
                       , unsigned int norm_scale_addr
                       , unsigned int norm_bias_addr
                       , unsigned int norm_in_size   
                       , unsigned int norm_scale_size
                       , unsigned int norm_bias_size 
                       , unsigned int norm_in_channel
                       , unsigned int norm_epsilon
                       , unsigned int acti_size
                       , unsigned int acti_channel
                       , unsigned int acti_negative_slope
                       , unsigned int pool_out_size
                       , unsigned int pool_in_size
                       , unsigned int pool_kernel_size
                       , unsigned int pool_channel
                       , unsigned int pool_stride
                       , unsigned int pool_padding
                       , unsigned int pool_ceil_mode
                       , __attribute__((unused)) unsigned int time_out
                       )
{
#if 0
    myInfo("CSR_ADDR_CONV_BUFFER_DATA        0x%08X\n", conv_buffer         );
    myInfo("CSR_ADDR_NORM_BUFFER_DATA        0x%08X\n", norm_buffer         );
    myInfo("CSR_ADDR_ACTI_BUFFER_DATA        0x%08X\n", acti_buffer         );
    myInfo("CSR_ADDR_POOL_BUFFER_DATA        0x%08X\n", pool_buffer         );
    myInfo("CSR_ADDR_IN_DATA_DATA            0x%08X\n", modl_input_addr     );
    myInfo("CSR_ADDR_CONV_KERNEL_DATA        0x%08X\n", conv_weight_addr    );
    myInfo("CSR_ADDR_CONV_BIAS_DATA          0x%08X\n", conv_bias_addr      );
    myInfo("CSR_ADDR_CONV_OUT_SIZE_DATA      0x%08X\n", conv_out_size       );
    myInfo("CSR_ADDR_CONV_IN_SIZE_DATA       0x%08X\n", conv_in_size        );
    myInfo("CSR_ADDR_CONV_KERNEL_SIZE_DATA   0x%08X\n", conv_kernel_size    );
    myInfo("CSR_ADDR_CONV_BIAS_SIZE_DATA     0x%08X\n", conv_bias_size      );
    myInfo("CSR_ADDR_CONV_IN_CHANNEL_DATA    0x%08X\n", conv_in_channel     );
    myInfo("CSR_ADDR_CONV_OUT_CHANNEL_DATA   0x%08X\n", conv_out_channel    );
    myInfo("CSR_ADDR_CONV_STRIDE_DATA        0x%08X\n", conv_stride         );
    myInfo("CSR_ADDR_CONV_PADDING_DATA       0x%08X\n", conv_padding        );
    myInfo("CSR_ADDR_NORM_RUNNING_MEAN_DATA  0x%08X\n", norm_mean_addr      );
    myInfo("CSR_ADDR_NORM_RUNNING_VAR_DATA   0x%08X\n", norm_var_addr       );
    myInfo("CSR_ADDR_NORM_SCALE_DATA         0x%08X\n", norm_scale_addr     );
    myInfo("CSR_ADDR_NORM_BIAS_DATA          0x%08X\n", norm_bias_addr      );
    myInfo("CSR_ADDR_NORM_IN_SIZE_DATA       0x%08X\n", norm_in_size        );
    myInfo("CSR_ADDR_NORM_SCALE_SIZE_DATA    0x%08X\n", norm_scale_size     );
    myInfo("CSR_ADDR_NORM_BIAS_SIZE_DATA     0x%08X\n", norm_bias_size      );
    myInfo("CSR_ADDR_NORM_IN_CHANNEL_DATA    0x%08X\n", norm_in_channel     );
    myInfo("CSR_ADDR_NORM_EPSILON_DATA       0x%08X\n", norm_epsilon        );
    myInfo("CSR_ADDR_ACTI_SIZE_DATA          0x%08X\n", acti_size           );
    myInfo("CSR_ADDR_ACTI_CHANNEL_DATA       0x%08X\n", acti_channel        );
    myInfo("CSR_ADDR_ACTI_NEGATIVE_SLOPE_DATA0x%08X\n", acti_negative_slope );
    myInfo("CSR_ADDR_POOL_OUT_SIZE_DATA      0x%08X\n", pool_out_size       );
    myInfo("CSR_ADDR_POOL_IN_SIZE_DATA       0x%08X\n", pool_in_size        );
    myInfo("CSR_ADDR_POOL_KERNEL_SIZE_DATA   0x%08X\n", pool_kernel_size    );
    myInfo("CSR_ADDR_POOL_CHANNEL_DATA       0x%08X\n", pool_channel        );
    myInfo("CSR_ADDR_POOL_STRIDE_DATA        0x%08X\n", pool_stride         );
    myInfo("CSR_ADDR_POOL_PADDING_DATA       0x%08X\n", pool_padding        );
    myInfo("CSR_ADDR_POOL_CEIL_MODE_DATA     0x%08X\n", pool_ceil_mode      );
return 0;
#endif
    // wait until HW is idle
    unsigned int ap_idle=0;
    do { confmc_read(CSR_ADDR_AP_CTRL, &ap_idle);
    } while ((ap_idle&0x4)==0);

    // fill CSR
    confmc_write(CSR_ADDR_CONV_BUFFER_DATA        , &conv_buffer         );
    confmc_write(CSR_ADDR_NORM_BUFFER_DATA        , &norm_buffer         );
    confmc_write(CSR_ADDR_ACTI_BUFFER_DATA        , &acti_buffer         );
    confmc_write(CSR_ADDR_POOL_BUFFER_DATA        , &pool_buffer         );
    confmc_write(CSR_ADDR_IN_DATA_DATA            , &modl_input_addr     );
    confmc_write(CSR_ADDR_CONV_KERNEL_DATA        , &conv_weight_addr    );
    confmc_write(CSR_ADDR_CONV_BIAS_DATA          , &conv_bias_addr      );
    confmc_write(CSR_ADDR_CONV_OUT_SIZE_DATA      , &conv_out_size       );
    confmc_write(CSR_ADDR_CONV_IN_SIZE_DATA       , &conv_in_size        );
    confmc_write(CSR_ADDR_CONV_KERNEL_SIZE_DATA   , &conv_kernel_size    );
    confmc_write(CSR_ADDR_CONV_BIAS_SIZE_DATA     , &conv_bias_size      );
    confmc_write(CSR_ADDR_CONV_IN_CHANNEL_DATA    , &conv_in_channel     );
    confmc_write(CSR_ADDR_CONV_OUT_CHANNEL_DATA   , &conv_out_channel    );
    confmc_write(CSR_ADDR_CONV_STRIDE_DATA        , &conv_stride         );
    confmc_write(CSR_ADDR_CONV_PADDING_DATA       , &conv_padding        );
    confmc_write(CSR_ADDR_NORM_RUNNING_MEAN_DATA  , &norm_mean_addr      );
    confmc_write(CSR_ADDR_NORM_RUNNING_VAR_DATA   , &norm_var_addr       );
    confmc_write(CSR_ADDR_NORM_SCALE_DATA         , &norm_scale_addr     );
    confmc_write(CSR_ADDR_NORM_BIAS_DATA          , &norm_bias_addr      );
    confmc_write(CSR_ADDR_NORM_IN_SIZE_DATA       , &norm_in_size        );
    confmc_write(CSR_ADDR_NORM_SCALE_SIZE_DATA    , &norm_scale_size     );
    confmc_write(CSR_ADDR_NORM_BIAS_SIZE_DATA     , &norm_bias_size      );
    confmc_write(CSR_ADDR_NORM_IN_CHANNEL_DATA    , &norm_in_channel     );
    confmc_write(CSR_ADDR_NORM_EPSILON_DATA       , &norm_epsilon        );
    confmc_write(CSR_ADDR_ACTI_SIZE_DATA          , &acti_size           );
    confmc_write(CSR_ADDR_ACTI_CHANNEL_DATA       , &acti_channel        );
    confmc_write(CSR_ADDR_ACTI_NEGATIVE_SLOPE_DATA, &acti_negative_slope );
    confmc_write(CSR_ADDR_POOL_OUT_SIZE_DATA      , &pool_out_size       );
    confmc_write(CSR_ADDR_POOL_IN_SIZE_DATA       , &pool_in_size        );
    confmc_write(CSR_ADDR_POOL_KERNEL_SIZE_DATA   , &pool_kernel_size    );
    confmc_write(CSR_ADDR_POOL_CHANNEL_DATA       , &pool_channel        );
    confmc_write(CSR_ADDR_POOL_STRIDE_DATA        , &pool_stride         );
    confmc_write(CSR_ADDR_POOL_PADDING_DATA       , &pool_padding        );
    confmc_write(CSR_ADDR_POOL_CEIL_MODE_DATA     , &pool_ceil_mode      );

    unsigned int ap_gie=0x0; // disable interrupt since not used
    confmc_write(CSR_ADDR_GIE, &ap_gie);

    // start the engine
    unsigned int ap_start=0x1;
    confmc_write(CSR_ADDR_AP_CTRL, &ap_start);

#if 0
    // wait until it is done
    unsigned int ap_done=0;
    do { confmc_read(CSR_ADDR_AP_CTRL, &ap_done);
    } while ((ap_done&0x2)==0);
#else
    // wait until it is done
    unsigned int ap_done=0;
    int num_wait=0;
    int pnum=0;
    char prompt[]="-\\|/";
    do { usleep(10); // 1usec sleep
         if ((num_wait%2000)==0) { myPrint("%c\b", prompt[pnum%4]); pnum++; fflush(stdout); } // 10sec
         num_wait++;
         confmc_read(CSR_ADDR_AP_CTRL, &ap_done);
    } while ((ap_done&0x2)==0);
    myPrint("\n"); // 1sec
#endif

    return 0;
}

/* @brief do_run_engine()
 *
 * @return 0 on success, othewise return negative number.
 */
int do_run_engine( int idx, int stage, global_t *global )
{
myInfo("%s working on state=%d/9: ", global->images_name[idx], stage+1);
    return do_csr_setup (
              conv_buffer[stage]                                        // conv_buffer_addr
            , (stage < 8 ? norm_buffer[stage] : 0)                      // norm_buffer_addr
            , (stage < 8 ? acti_buffer[stage] : 0)                      // acti_buffer_addr
            , (stage < 6 ? pool_buffer[stage] : 0)                      // modl_output_addr
            , modl_in_data[stage]                                       // modl_input_addr
            , MEM_ADDR_WEIGHTS                + weight_offset[stage]*sizeof(float)      // conv_weight_addr
            , (stage < 8 ? 0 : MEM_ADDR_BIASES + bias_offset  [stage]*sizeof(float))     // conv_bias_addr
            , conv_out_size[stage]                                      // conv_out_size
            , conv_in_size[stage]                                       // conv_in_size
            , (stage < 8 ? 3 : 1)                                       // conv_kernel_size
            , (stage < 8 ? 0 : conv_out_channel[stage])                 // conv_bias_size
            , conv_in_channel[stage]                                    // conv_in_channel
            , conv_out_channel[stage]                                   // conv_out_channel
            , 1                                                         // conv_stride
            , (stage < 8 ? 1 : 0)                                       // conv_padding
            , (stage < 8 ? MEM_ADDR_MEANS  + bn_offset  [stage]*sizeof(float) : 0)   // norm_mean_addr
            , (stage < 8 ? MEM_ADDR_VARIANCES    + bn_offset  [stage]*sizeof(float) : 0)   // norm_var_addr
            , (stage < 8 ? MEM_ADDR_SCALES  + bn_offset  [stage]*sizeof(float) : 0)   // norm_scale_addr
            , (stage < 8 ? MEM_ADDR_BIASES   + bias_offset[stage]*sizeof(float) : 0)   // norm_bias_addr
            , (stage < 8 ? norm_in_size                  [stage] : 0)   // norm_in_size
            , (stage < 8 ? norm_weight_size              [stage] : 0)   // norm_scale_size
            , (stage < 8 ? norm_bias_size                [stage] : 0)   // norm_bias_size
            , (stage < 8 ? norm_in_channel               [stage] : 0)   // norm_in_channel
            , norm_epsilon                                              // norm_epsilon
            , (stage < 8 ? acti_size[stage] : 0)                        // acti_size 
            , (stage < 8 ? acti_channel[stage] : 0)                     // acti_channel
            , acti_negative_slope                                       // acti_negative_slope
            , (stage < 6 ? pool_out_size[stage] : 0)                    // pool_out_size
            , (stage < 6 ? pool_in_size[stage] : 0)                     // pool_in_size
            , 2                                                         // pool_kernel_size
            , (stage < 6 ? pool_channel[stage] : 0)                     // pool_channel
            , (stage < 5 ? 2 : 1)                                       // pool_stride
            , 0                                                         // pool_padding
            , 0                                                         // pool_ceil_mode
            , 0 // time_out; 0 for blocking
            );
}

/*
 * Revision history
 *
 * 2021.04.13: started by Ando Ki (adki@future-ds.com)
 */

// do_inference @AID
static int do_csr_setup_aid( 
                        unsigned int layer_num
                       , __attribute__((unused)) unsigned int time_out
                       )
{
#if 0
#endif
    // wait until HW is idle
    unsigned int ap_idle=0;
    do { confmc_read(CSR_ADDR_AP_CTRL, &ap_idle);
    } while ((ap_idle&0x4)==0);

    // fill CSR
    confmc_write(0x70000010, &layer_num); 
    unsigned int ap_gie=0x0; // disable interrupt since not used
    confmc_write(CSR_ADDR_GIE, &ap_gie);

    // start the engine
    unsigned int ap_start=0x1;
    confmc_write(CSR_ADDR_AP_CTRL, &ap_start);

#if 0
    // wait until it is done
    unsigned int ap_done=0;
    do { confmc_read(CSR_ADDR_AP_CTRL, &ap_done);
    } while ((ap_done&0x2)==0);
#else
    // wait until it is done
    unsigned int ap_done=0;
    int num_wait=0;
    int pnum=0;
    char prompt[]="-\\|/";
    do { usleep(10); // 1usec sleep
         if ((num_wait%2000)==0) { myPrint("%c\b", prompt[pnum%4]); pnum++; fflush(stdout); } // 10sec
         num_wait++;
         confmc_read(CSR_ADDR_AP_CTRL, &ap_done);
    } while ((ap_done&0x2)==0);
//    myPrint("\n"); // 1sec
#endif
    return 0;
}

int do_run_engine_aid( int idx, int stage, global_t *global )
{
//myInfo("%s working on state=%d/9: ", global->images_name[idx], stage+1);
    return do_csr_setup_aid (
              layer_num[stage]      // layer_num
            , 0                     // time_out; 0 for blocking
            );
}

static int do_normalize_image_aid( image_uchar_t *image_uchar
                                , u_int8_t *image_int)
{
    int status=0;
    // do not free "float_img" unless error occurs.
    int idx=0;
    unsigned char *pt_uchar = image_uchar->data;
    for (int i = 0; i < 416; i ++){
	 for (int j = 0; j < 416; j++){
	      for (int k = 0; k < 3; k++){
		    image_int[idx] = pt_uchar[i*416 + j + k*173056];
		    if (k==2) {
			  idx++;
			  image_int[idx] = 0;
		    }
		    idx++;
	      }
	 }
    }

cleanup:

    return status;
}
// END @ AID