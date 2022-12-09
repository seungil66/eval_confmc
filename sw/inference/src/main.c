/*
 * Copyright (c) 2021 by Future Design Systems
 * All rights reserved.
 * http://www.future-ds.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "defines.h"
#include "etc.h"
#include "confmc.h"
#include "preprocess.h"
#include "load_params.h"
#include "inference.h"
#include "postprocess.h"

static int arg_parser( int argc, char **argv, global_t *global );
static int help( int argc, char **argv, int all, global_t *global );
       void sig_handle(int);

static char   version[]="2021.04.13";
static int    rigor=0;
static int    verbose=0;
static int    evaluation=0; // 0=inference only, 1=inference and evaluation

static char *file_images =NULL; // file containing list of images
static char *file_gts=NULL; // file containing list of GT corresponding to the images
static char *file_params=NULL; // file containing list of parameters
static char *dir_results=NULL; // directory to store results
#define DIR_RESULTS "results"
static char *file_object_names=NULL;
#define FILE_OBJECT_NAMES  "library/face-mask-detection.names"
static char *dir_alphabets=NULL;
#define DIR_ALPHABETS  "library/alphabets"

global_t *global=NULL;

int main( int argc, char *argv[] )
{
    if ((signal(SIGINT, sig_handle)==SIG_ERR)
         #if defined(SIGQUIT)
            ||(signal(SIGQUIT, sig_handle)==SIG_ERR)
         #endif
            ) {
          fprintf(stderr, "Error: signal error\n");
          exit(1);
    }

    if ((global=(global_t *)calloc(1, sizeof(global_t)))==NULL) goto cleanup;

    if (arg_parser( argc, argv, global )!=0) goto cleanup;
    if (confmc_setup()!=0) goto cleanup;

    if (do_preprocess( file_images
                     , file_params
                     , file_gts
                     , dir_results
                     , global )!=0) goto cleanup;
    if (do_load_params( global )!=0) goto cleanup;  

    global->time_elapse = 0;
    global->time_start = timestamp();
    if (do_inference( dir_results
                    , global )!=0) goto cleanup;
    global->time_end = timestamp();

    if (do_postprocess( dir_results
                      , file_object_names
                      , dir_alphabets
                      , global )!=0) goto cleanup;

cleanup:
    if (file_images!=NULL) free(file_images);
    if (file_params!=NULL) free(file_params);
    if (global!=NULL){
        if (global->images_uchar!=NULL) {
            for (int idx=0; idx<global->images_num; idx++) {
                 if (global->images_uchar[idx].data!=NULL) free(global->images_uchar[idx].data);
            }
            free(global->images_uchar);
        }
        if (global->images_float!=NULL) {
            for (int idx=0; idx<global->images_num; idx++) {
                 if (global->images_float[idx].data!=NULL) free(global->images_float[idx].data);
            }
            free(global->images_float);
        }
        if (global->gt_values!=NULL) free(global->gt_values);
        if (global->gt_values_num!=NULL) free(global->gt_values_num);
        if (global->results!=NULL) {
            for (int idx=0; idx<global->results_num; idx++) {
                if (global->results[idx]!=NULL) free(global->results[idx]);
            }
            free(global->results);
        }
        if (global->results_name!=NULL) {
            for (int idx=0; idx<global->results_num; idx++) {
                if (global->results_name[idx]!=NULL) free(global->results_name[idx]);
            }
            free(global->results_name);
        }
        if (global->param_value_biases!=NULL) free(global->param_value_biases);
        if (global->param_value_weights!=NULL) free(global->param_value_weights);
        if (global->param_value_scales!=NULL) free(global->param_value_scales);
        if (global->param_value_means!=NULL) free(global->param_value_means);
        if (global->param_value_variances!=NULL) free(global->param_value_variances);
        if (global->object_names!=NULL) free(global->object_names);
        if (global->alphabets!=NULL) free(global->alphabets);
    }
    confmc_wrappup();
    return 0;
}

/*
 * arsing command-line arguments
 *
 * return 0 on success, otherwize return negative number.
 */
static int arg_parser( int argc, char **argv, global_t *global )
{
    static struct option long_options[] = {
           {"images"       , required_argument, 0,   'i'},
           {"params"       , required_argument, 0,   'p'},
           {"ground"       , required_argument, 0,   'g'},
           {"results"      , required_argument, 0,   't'},

           {"labels"       , required_argument, 0,   'l'},
           {"alphabets"    , required_argument, 0,   'a'},
           {"evaluation"   , no_argument      , 0,   'e'},

           {"nms"          , required_argument, 0,   'A'},
           {"thresh"       , required_argument, 0,   'B'},
           {"iou_thresh"   , required_argument, 0,   'C'},
           {"hier_thresh"  , required_argument, 0,   'D'},
           {"eval_thresh"  , required_argument, 0,   'E'},

           {"rigor"        , no_argument      , 0,   'r'},
           {"verbose"      , no_argument      , 0,   'b'},
           {"help"         , no_argument      , 0,   'h'},
           {"help_all"     , no_argument      , 0,   'H'},
           {"version"      , no_argument      , 0,   'v'},
           {0              , 0                , 0,    0 }
    };

    global->nms = 0.45;
    global->thresh = 0.5;
    global->iou_thresh = 0.5;
    global->hier_thresh = 0.5;
    global->eval_thresh = 0.005;

    int longind=0;
    while (optind<argc) {
        int opt=getopt_long(argc,argv,"i:p:g:t:l:a:erbhHvA:B:C:D:",long_options,&longind);
        if (opt==-1) break;
        switch (opt) {
        case 0:   myPrint("option %s", long_options[longind].name);
                  if (optarg) myPrint(" with arg %s", optarg);
                  myPrint("\n");
                  break;
        case 'i': file_images = (char*)calloc(strlen(optarg)+1, sizeof(char));
                  strcpy(file_images, optarg);
                  break;
        case 'p': file_params =  (char*)calloc(strlen(optarg)+1, sizeof(char));
                  strcpy(file_params, optarg);
                  break;
        case 'g': file_gts =  (char*)calloc(strlen(optarg)+1, sizeof(char));
                  strcpy(file_gts, optarg);
                  break;
        case 't': dir_results =  (char*)calloc(strlen(optarg)+1, sizeof(char));
                  strcpy(dir_results, optarg);
                  break;
        case 'l': file_object_names =  (char*)calloc(strlen(optarg)+1, sizeof(char));
                  strcpy(file_object_names, optarg);
                  break;
        case 'a': dir_alphabets =  (char*)calloc(strlen(optarg)+1, sizeof(char));
                  strcpy(dir_alphabets, optarg);
                  break;
        case 'e': global->mode = 1;
                  break;
        case 'r': rigor=1;
                  break;
        case 'b': verbose=1;
                  break;
        case 'h': help(argc, argv, 0, global);
                  return -1;
        case 'H': help(argc, argv, 1, global);
                  return -1;
        case 'v': myPrint("%s\n", version);
                  return -1;
        case 'A': global->nms = (float)atof(optarg); break;
        case 'B': global->thresh = (float)atof(optarg); break;
        case 'C': global->iou_thresh = (float)atof(optarg); break;
        case 'D': global->hier_thresh = (float)atof(optarg); break;
        case 'E': global->eval_thresh = (float)atof(optarg); break;
        default:  myError("%s unknown option -%c\n", argv[0], opt);
                  return -1;
        }
    }
    while (optind<argc) {
        myError("unknown option \"%s\"\n", argv[optind]);
        optind++;
    }
    if (dir_results==NULL) {
        dir_results = (char *)calloc(strlen(DIR_RESULTS)+1, sizeof(char));
        strcpy(dir_results, DIR_RESULTS);
    }
    if (file_object_names==NULL) {
        file_object_names = (char *)calloc(strlen(FILE_OBJECT_NAMES)+1, sizeof(char));
        strcpy(file_object_names, FILE_OBJECT_NAMES);
    }
    if (dir_alphabets==NULL) {
        dir_alphabets = (char *)calloc(strlen(DIR_ALPHABETS)+1, sizeof(char));
        strcpy(dir_alphabets, DIR_ALPHABETS);
    }
    if (file_images==NULL) {
        file_images = (char *)calloc(strlen("test_image_list.txt")+1, sizeof(char));
        strcpy(file_images, "test_image_list.txt");
    }
    if (file_params==NULL) {
        file_params = (char *)calloc(strlen("test_param_list.txt")+1, sizeof(char));
        strcpy(file_params, "test_param_list.txt");
    }
    if (file_gts==NULL) {
        file_gts = (char *)calloc(strlen("test_gt_list.txt")+1, sizeof(char));
        strcpy(file_gts, "test_gt_list.txt");
    }

    return 0;
}

static int help( int argc, char **argv, int all, global_t *global )
{
       myPrint("[Usage] %s [options]\n", argv[0]);
       myPrint("    -i|--images=file    file containing list of input images\n");
       myPrint("    -p|--params=file    file containing list of parameters\n");
       myPrint("    -t|--results=dir    directoy to store results (default: %s)\n", DIR_RESULTS);
       myPrint("    -l|--labels=file    file containing object names (default: %s)\n", FILE_OBJECT_NAMES);
       myPrint("    -a|--alphabets=dir  directory of labels (default: %s)\n", DIR_ALPHABETS);
       myPrint("    -r|--rigor          check rigorously\n");
       myPrint("    -b|--verbose        set verbose mode\n");
       myPrint("    -h|--help           help\n");
       myPrint("    -v|--version        version\n");

       if (all) {
       myPrint("    -A|--nms=float (%f)\n",          global->nms);
       myPrint("    -B|--thresh=float (%f)\n",       global->thresh);
       myPrint("    -C|--iou_thresh=float (%f)\n",   global->iou_thresh);
       myPrint("    -D|--hier_thresh=float (%f)\n",  global->hier_thresh);
       myPrint("    -E|--eval_thresh=float  (%f)\n", global->eval_thresh);
       }
       return 0;
}

void sig_handle(int sig) {
  extern void cleanup();
  switch (sig) {
  case SIGINT:
  #if !defined(WIN32)&&!defined(_MSC_VER)
  case SIGQUIT:
  #endif
       fflush(stdout); fflush(stderr);
       exit(0);
       break;
  }
}

/*
 * Revision history
 *
 * 2021.04.13: started by Ando Ki (adki@future-ds.com)
 */
