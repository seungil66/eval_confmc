#pragma once
/*
 * Copyright (c) 2021 by Future Design Systems
 * All rights reserved.
 * http://www.future-ds.com
 */

#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

// This routine prepares necessary data structure:
//  - image data in float.
//  - parameter data in float.
//  - ground truth data in float.
extern int do_preprocess( char     *file_images
                        , char     *file_params
                        , char     *file_gts
                        , char     *dir_results
                        , global_t *global );

#ifdef __cplusplus
}
#endif

/*
 * Revision history
 *
 * 2021.04.13: started by Ando Ki (adki@future-ds.com)
 */
