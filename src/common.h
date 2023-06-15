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
#include "platforms.h"

//#define DEBUG_MPI 1

#define toString(s) std::to_string(s)

// Windows-specific includes
#include <tchar.h>
#include <direct.h>

#include "CLog.h"
#include "CModel.h"

using tinyxml2::XMLElement;

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


// Application author details
const std::string appName = " _    _   _   _____    _____   __  __    _____  \n"
" | |  | | (_) |  __ \\  |_   _| |  \\/  |  / ____| \n"
" | |__| |  _  | |__) |   | |   | \\  / | | (___   \n"
" |  __  | | | |  ___/    | |   | |\\/| |  \\___ \\  \n"
" | |  | | | | | |       _| |_  | |  | |  ____) | \n"
" |_|  |_| |_| |_|      |_____| |_|  |_| |_____/  \n"
"   High-performance Integrated Modelling System   ";
const std::string appAuthor = "Luke S. Smith and Qiuhua Liang";
const std::string appContact = "luke@smith.ac";
const std::string appUnit = "School of Civil Engineering and Geosciences";
const std::string appOrganisation = "Newcastle University";
const std::string appRevision = "$Revision: 717 $";

// Application version details
const unsigned int appVersionMajor = 0;	// Major 
const unsigned int appVersionMinor = 2;	// Minor
const unsigned int appVersionRevision = 0;	// Revision

// Application structure for argument names
struct modelArgument {
	const char		cShort[3];
	const char* cLong;
	const char* cDescription;
};

//MUSCL
// Kernel configurations
namespace schemeConfigurations {
	namespace musclHancock {
		enum musclHancock {
			kCacheNone = 10,		// Option B in dissertation: No local memory used
			kCachePrediction = 11,		// Option C in dissertation: Only the prediction step uses caching
			kCacheMaximum = 12		// Option D in dissertation: All stages use cache memory
		};
	}
}

namespace cacheConstraints {
	namespace musclHancock {
		enum musclHancock {
			kCacheActualSize = 10,		// LDS of actual size
			kCacheAllowOversize = 11,		// Allow LDS oversizing to avoid bank conflicts
			kCacheAllowUndersize = 12		// Allow LDS undersizing to avoid bank conflicts
		};
	}
}

//Inertial
// Kernel configurations
namespace schemeConfigurations {
	namespace inertialFormula {
		enum inertialFormula {
			kCacheNone = 0,		// No caching
			kCacheEnabled = 1			// Cache cell state data
		};
	}
}

namespace cacheConstraints {
	namespace inertialFormula {
		enum inertialFormula {
			kCacheActualSize = 0,		// LDS of actual size
			kCacheAllowOversize = 1,		// Allow LDS oversizing to avoid bank conflicts
			kCacheAllowUndersize = 2			// Allow LDS undersizing to avoid bank conflicts
		};
	}
}

//Godunov
// Kernel configurations
namespace schemeConfigurations {
	namespace godunovType {
		enum godunovType {
			kCacheNone = 0,		// No caching
			kCacheEnabled = 1			// Cache cell state data
		};
	}
}

namespace cacheConstraints {
	namespace godunovType {
		enum godunovType {
			kCacheActualSize = 0,		// LDS of actual size
			kCacheAllowOversize = 1,		// Allow LDS oversizing to avoid bank conflicts
			kCacheAllowUndersize = 2			// Allow LDS undersizing to avoid bank conflicts
		};
	}
}


//Domain Base:

// Model domain structure types
namespace domainStructureTypes {
	enum domainStructureTypes {
		kStructureCartesian = 0,	// Cartesian
		kStructureRemote = 1,	// Remotely held domain
		kStructureInvalid = 255	// Error state, cannot work with this type of domain
	};
}

//CDomain
// Model domain structure types
namespace domainValueIndices {
	enum domainValueIndices {
		kValueFreeSurfaceLevel = 0,	// Free-surface level
		kValueMaxFreeSurfaceLevel = 1,	// Max free-surface level
		kValueDischargeX = 2,	// Discharge X
		kValueDischargeY = 3		// Discharge Y
	};
}

extern	CModel*			pManager;
extern  char*			configFile;
extern  char*			codeDir;
void					doError( std::string, unsigned char );
}

// Variables in use throughput
using	model::pManager;

#endif
