/*
 * ------------------------------------------
 *
 *  HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 *  Luke S. Smith and Qiuhua Liang
 *  luke@smith.ac
 *
 *  School of Civil Engineering & Geosciences
 *  Newcastle University
 * 
 * ------------------------------------------
 *  This code is licensed under GPLv3. See LICENCE
 *  for more information.
 * ------------------------------------------
 *  Windows-specific code and utility functions
 * ------------------------------------------
 *
 */

#ifndef HIPIMS_PLATFORMS_WINDOWS_PLATFORM_H_
#define HIPIMS_PLATFORMS_WINDOWS_PLATFORM_H_

#include "common.h"



// Forward conditionals
#define PLATFORM_WIN

// Windows-specific includes
#include <Windows.h>					// QueryPerformanceCounter etc

// Windows-specific includes
//#define isnan _isnan
#ifndef NAN
#define NAN	  _Nan._Double
#endif

/*
 *  OS PORTABILITY CONSTANTS
 */


#endif
