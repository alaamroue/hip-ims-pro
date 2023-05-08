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
#ifndef HIPIMS_COMMON_H_
#define HIPIMS_COMMON_H_

// Base includes
#include "util.h"




#include "Platforms/windows_platform.h"

//#define DEBUG_MPI 1

//#define std::to_string(s) std::to_string(s)

// Windows-specific includes
#include <tchar.h>
#include <direct.h>

#include "General/CLog.h"
#include "CModel.h"


// Basic functions and variables used throughout
namespace model
{
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

extern	CModel*			pManager;
extern  char*			codeDir;
void					doError( std::string, unsigned char );
}




// Variables in use throughput
using	model::pManager;

#endif
