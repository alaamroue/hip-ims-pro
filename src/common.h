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
 *  Common header file
 * ------------------------------------------
 *
 */
#pragma once

// Base includes
#include "util.h"

#include "windows_platform.h"

// Windows-specific includes
#include <tchar.h>
#include <direct.h>

#include "CLog.h"
#include "CModel.h"


// Basic functions and variables used throughout
namespace model
{
	// Application author details
	const std::string appName = "High-performance Integrated Modelling System";
	const std::string appAuthor = "Luke S. Smith and Qiuhua Liang";
	const std::string appContact = "luke@smith.ac";
	const std::string appUnit = "School of Civil Engineering and Geosciences";
	const std::string appOrganisation = "Newcastle University";
	const std::string appRevision = "$Revision: 717 $";

	// Application version details
	const unsigned int appVersionMajor = 0;	// Major 
	const unsigned int appVersionMinor = 2;	// Minor
	const unsigned int appVersionRevision = 0;	// Revision

	// Data structures used in interop
	struct DomainData
	{
		double			dResolution;
		double			dWidth;
		double			dHeight;
		double			dCornerWest;
		double			dCornerSouth;
		unsigned long	ulCellCount;
		unsigned long	ulRows;
		unsigned long	ulCols;
		unsigned long	ulBoundaryCells;
		unsigned long	ulBoundaryOthers;
	};

// Application return codes
namespace appReturnCodes{ enum appReturnCodes {
	kAppSuccess							= 0,	// Success
	kAppInitFailure						= 1,	// Initialisation failure
	kAppFatal							= 2		// Fatal error
}; }

// Error type codes
namespace errorCodes { enum errorCodes {
	kLevelFatal							= 1,	// Fatal error, no continuation
	kLevelModelStop						= 2,	// Error that requires the model to abort
	kLevelModelContinue					= 4,	// Error for which the model can continue
	kLevelWarning						= 8,	// Display a warning message
	kLevelInformation					= 16	// Just provide some information
}; }

// Floating point precision
namespace floatPrecision{
	enum floatPrecision {
		kSingle = 0,	// Single-precision
		kDouble = 1		// Double-precision
	};
}

//extern	CModel*			pManager;
void					doError( std::string, unsigned char );
}




// Variables in use throughput
//using	model::pManager;
