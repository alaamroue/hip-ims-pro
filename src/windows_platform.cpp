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
 *  Utility functions for Windows only, etc.
 * ------------------------------------------
 *
 */

#include "common.h"


char*	Util::getFileResource( const char * sName, const char * sType )
{
	HMODULE hModule = GetModuleHandle( NULL );


	if ( hModule == NULL )
		model::doError(
			"The module handle could not be obtained.",
			model::errorCodes::kLevelFatal
		);

	HRSRC hResource = FindResource( hModule, static_cast<LPCSTR>(sName), static_cast<LPCSTR>(sType) );

	if ( hModule == NULL )
	{
		model::doError(
			"Could not obtain a requested resource.",
			model::errorCodes::kLevelWarning
		);
		return "";
	}

	HGLOBAL hData = LoadResource( hModule, hResource );

	if ( hData == NULL )
	{
		model::doError(
			"Could not load a requested resource value.",
			model::errorCodes::kLevelWarning
		);
		return "";
	}

	DWORD	dwSize		= SizeofResource( hModule, hResource );
	char*	cResource	= new char[ dwSize + 1 ];

	LPVOID	lpData		= LockResource( hData );
	char*	cSource		= static_cast<char *>(lpData);

	if ( lpData == NULL )
	{
		model::doError(
			"Could not obtain a pointer for a requested resource.",
			model::errorCodes::kLevelWarning
		);
		return "";
	}

	memcpy( cResource, cSource, dwSize );
	cResource[ dwSize ] = 0;

	return cResource;
}

/*
 *  Identify the cursor location in the console, mostly so we can remove it
 *  and overwrite later.
 */
cursorCoords	Util::getCursorPosition()
{
	CONSOLE_SCREEN_BUFFER_INFO	pBufferInfo;
	cursorCoords				pCoordReturn;
	HANDLE						hOut			= GetStdHandle( STD_OUTPUT_HANDLE );

	pCoordReturn.sX = -1;
	pCoordReturn.sY = -1;

	if ( GetConsoleScreenBufferInfo( hOut, &pBufferInfo ) == 0 ) 
		return pCoordReturn;

	pCoordReturn.sX = pBufferInfo.dwCursorPosition.X;
	pCoordReturn.sY = pBufferInfo.dwCursorPosition.Y;

	return pCoordReturn;
}

/*
 *  Identify the cursor location in the console, mostly so we can remove it
 *  and overwrite later.
 */
void	Util::setCursorPosition( cursorCoords pLocation )
{
	CONSOLE_SCREEN_BUFFER_INFO	pBufferInfo;
	HANDLE						hOut			= GetStdHandle( STD_OUTPUT_HANDLE );

	if ( GetConsoleScreenBufferInfo( hOut, &pBufferInfo ) == 0 ) 
		return;	

	pLocation.sY = max(0, pLocation.sY);
	pLocation.sY = min(pBufferInfo.dwSize.Y - 1, pLocation.sY);

	COORD newCoords;
	newCoords.X = (short)pLocation.sX;
	newCoords.Y = (short)pLocation.sY;

	SetConsoleCursorPosition( hOut, newCoords );
}


