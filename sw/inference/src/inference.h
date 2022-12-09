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

// carryout inference
extern int do_inference( char *dir_results
                       , global_t *global );

// set up inferenc HW for specific stage
extern int do_run_engine( int idx, int stage, global_t *global );

#ifdef __cplusplus
}
#endif

/*
 * Revision history
 *
 * 2021.04.13: started by Ando Ki (adki@future-ds.com)
 */
