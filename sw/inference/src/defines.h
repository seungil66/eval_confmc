#pragma once
/*
 * Copyright (c) 2021 by Future Design Systems
 * All rights reserved.
 * http://www.future-ds.com
 */

#ifdef __cplusplus
extern "C" {
#endif

#define IMAGE_COLORS  3
#define IMAGE_WIDTH   416
#define IMAGE_HEIGHT  416
#define IMAGE_SIZE   (IMAGE_COLORS*IMAGE_HEIGHT*IMAGE_WIDTH)

#define NUM_COLS      IMAGE_WIDTH
#define NUM_ROWS      IMAGE_HEIGHT
#define SIZE_IMG      IMAGE_SIZE
#define NUM_IMAGES    100

#define NUM_BBOX      5
#define NUM_OBJS      2
#define NUM_ANCHORS   4
#define NUM_OBJECTS   NUM_OBJS

#define RESULT_WIDTH  13
#define RESULT_HEIGHT 13
#define NUM_CLASSES   NUM_BBOX*(5+NUM_OBJS)
#define RESULT_SIZE  (NUM_CLASSES*RESULT_HEIGHT*RESULT_WIDTH)

#ifndef IMAGE_T
#define IMAGE_T
typedef struct {
    int w;
    int h;
    int c;
    float *data;
} image_float_t;
#endif
typedef struct {
    int w;
    int h;
    int c;
    unsigned char *data;
} image_uchar_t;

typedef struct ground_truth {
    int   masked; // 0=no-masked, 1=masked
    float x, y;
    float w, h;
} ground_truth_t;

typedef struct global {
    int              mode; // 0=inference only, 1=inference and evaluation
    image_uchar_t   *images_uchar; // pointers to  [IMAGE_COLORS][IMAGE_HEIGHT][IMAGE_WIDTH]
    image_float_t   *images_float; // pointers to [IMAGE_COLORS][IMAGE_HEIGHT][IMAGE_WIDTH]
    int              images_num; // number of images
    char           **images_name; // keep input image file names including path
    // ground truth table
    ground_truth_t **gt_values;
    int             *gt_values_num; // array to keep the number of GT elements
    int              gt_num; // number of gt_values
    // inference results
    float          **results; // pointers to [NUM_CLASSES][RESULT_HEIGHT][RESULT_WIDTH]
    int              results_num;
    char           **results_name; // keep image file path+basenames (.dat)
    int              results_done; // keep track of completion of inferences
    // trained parameters
    float           *param_value_biases;
    float           *param_value_weights;
    float           *param_value_scales;
    float           *param_value_means;
    float           *param_value_variances;
    int              param_size_biases; // bytes
    int              param_size_weights;
    int              param_size_scales;
    int              param_size_means;
    int              param_size_variances;
    // post-processing related
    char           **object_names; // post processing
    int              object_num; // num of classes, it should be NUM_OBJS
    image_float_t  **alphabets; // post processing
    float            nms; // non-maximum supression
    float            thresh;
    float            iou_thresh;
    float            hier_thresh;
    float            eval_thresh;
    long long        time_start;
    long long        time_end;
    long long        time_elapse;
} global_t;

#ifdef __cplusplus
}
#endif

/*
 * Revision history
 *
 * 2021.04.13: started by Ando Ki (adki@future-ds.com)
 */
