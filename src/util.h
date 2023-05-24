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
 *  Utility functions (some of which are platform-specific)
 * ------------------------------------------
 *
 */
#ifndef HIPIMS_UTIL_H_
#define HIPIMS_UTIL_H_

// Includes
#include <sstream>

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
	std::string secondsToTime( double );

	// Resource handling
	char*			getFileResource( const char *, const char * );
	cursorCoords	getCursorPosition();
	void			getHostname(char*);
	void			setCursorPosition( cursorCoords );
	double			round( double, unsigned char );
	char*			toLowercase( const char * );
	void			toLowercase( char**, const char * );
	void			toNewString( char**, const char * );
	//unsigned long	toTimestamp( const char *, const char * = NULL );
	//const char *	fromTimestamp( unsigned long, const char * = NULL );
	bool			fileExists( const char * );
}

#endif
