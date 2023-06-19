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
 *  Utility functions (some of which are platform-specific)
 * ------------------------------------------
 *
 */
#ifndef HIPIMS_UTIL_H_
#define HIPIMS_UTIL_H_

// Includes
#include <sstream>
#include "opencl.h"

// Structure definitions
struct cursorCoords {
	int	sX;
	int	sY;
};

/*
 *  FUNCTIONS
 */
namespace Util
{
	// String conversions
	std::string		secondsToTime(double);

	// Resource handling
	char* getFileResource(const char*, const char*);
	cursorCoords	getCursorPosition();
	void			getHostname(char*);
	void			setCursorPosition(cursorCoords);
	double			round(double, unsigned char);
	char* toLowercase(const char*);
	void			toLowercase(char**, const char*);
	void			toNewString(char**, const char*);
	bool			fileExists(const char*);
	std::string     to_string_exact(double);

}
#endif
