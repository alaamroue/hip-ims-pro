/*
 * This file is a modified version of the code originally created by Luke S. Smith and Qiuhua Liang.
 * Modifications: Project structure changes (Compare with original for exact changes)
 * Modified by: Alaa Mroue
 * Date of Modification: 04.2023
 *
 * Find the orignal code in OriginalSourceCode.zip
 * OriginalSourceCode.zip: Is a snapshot of the src folder from https://github.com/lukeshope/hipims-ocl based on 1e62acf6b9b480e08646b232361b68c1827d91ae
 */

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

