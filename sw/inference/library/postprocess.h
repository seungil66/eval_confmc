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

// This routine processes inference results.
extern int do_postprocess( char     *dir_results
                         , char     *file_class_names
                         , char     *dir_alphabets
                         , global_t *global );

#ifdef __cplusplus
}
#endif

/*
 * Revision history
 *
 * 2021.04.13: started by Ando Ki (adki@future-ds.com)
 */
