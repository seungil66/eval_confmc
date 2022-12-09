#pragma once
/*
 * Copyright (c) 2021 by Future Design Systems.
 * All right reserved.
 * http://www.future-ds.com
 *
 * @file etc.h
 * @brief This file contains functions for print out.
 * @author Ando Ki
 * @date 2021.03.13.
 */

#ifdef __cplusplus
extern "C" {
#endif

extern long long timestamp(); // usec unit

#define  myError(...)   myErrorCore(__FILE__, __LINE__, __func__, ##__VA_ARGS__)
extern int myErrorCore( const char *file
                      , const int   line
                      , const char *func
                      , const char *fmt, ...);
#define  myWarn(...)   myWarnCore(__FILE__, __LINE__, __func__, ##__VA_ARGS__)
extern int myWarnCore( const char *file
                     , const int   line
                     , const char *func
                     , const char *fmt, ...);
#define  myInfo(...)   myInfoCore(__FILE__, __LINE__, __func__, ##__VA_ARGS__)
extern int myInfoCore( const char *file
                     , const int   line
                     , const char *func
                     , const char *fmt, ...);
extern int myPrint( const char *fmt, ...);

#ifdef __cplusplus
}
#endif

/*
 * Revision history
 *
 * 2021.03.13: Started by Ando Ki (adki@future-ds.com)
 */
