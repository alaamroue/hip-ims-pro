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

#pragma once

// Windows-specific includes
#include <Windows.h>					// QueryPerformanceCounter etc


// Windows-specific includes
#ifndef NAN
#define NAN	  _Nan._Double
#endif


#ifndef _CONSOLE
namespace model {
	namespace cli {
		const WORD		colourTimestamp = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN;
		const WORD		colourError = FOREGROUND_RED | FOREGROUND_INTENSITY;
		const WORD		colourHeader = 0x03;
		const WORD		colourMain = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		const WORD		colourInfoBlock = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
	}
}
#else
namespace model {
namespace cli {
	const WORD		colourTimestamp		= FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | BACKGROUND_BLUE;
	const WORD		colourError			= FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_BLUE;
	const WORD		colourHeader		= 0x03 | BACKGROUND_BLUE;
	const WORD		colourMain			= FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_BLUE;
	const WORD		colourInfoBlock		= FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_BLUE;
}
}
#endif

