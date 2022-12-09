#pragma once
/*
 * Copyright (c) 2021 by Future Design Systems.
 * All right reserved.
 * http://www.future-ds.com
 *
 * @file confmc.h
 * @brief This file contains CON-FMC related
 * @author Ando Ki
 * @date 2021.03.13.
 */

extern int  confmc_setup( void );
extern void confmc_wrappup( void );

extern int confmc_write( unsigned int  addr
                       , unsigned int *data );

extern int confmc_read ( unsigned int  addr
                       , unsigned int *data );

extern int confmc_burst_read ( unsigned int *buffer
                             , unsigned int  addr
                             , unsigned int  len  // num of 4-byte words
                             , unsigned int  burst_len );
extern int confmc_burst_write( unsigned int *buffer
                             , unsigned int  addr
                             , unsigned int  len  // num of 4-byte words
                             , unsigned int  burst_len );
extern int confmc_compare( unsigned int *buffer
                         , unsigned int  addr
                         , unsigned int  len ); // num of 4-byte words

/*
 * Revision history
 *
 * 2021.03.13: Started by Ando Ki (adki@future-ds.com)
 */
