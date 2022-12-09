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

// This routine downloads parameters to the hardware.
// Some data transformation can be applied.
extern int do_load_params( global_t *global );

// Setup the inference engine for each stage.
extern int do_engine_setup( int stage
                          , global_t *global );

#ifdef __cplusplus
}
#endif

/*
 * Revision history
 *
 * 2021.04.13: started by Ando Ki (adki@future-ds.com)
 */
