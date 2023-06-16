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
#include <Windows.h>					// QueryPerformanceCounter etc


//#define DEBUG_MPI 1

#define toString(s) std::to_string(s)

// Windows-specific includes
#include <tchar.h>
#include <direct.h>

#include "CLog.h"
#include "CModel.h"

// TODO: Alaa check if these includes are needed. They have been removed wih no problems in the other branches (see master branch and gpu branch)
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>
#include <math.h>
#include <cmath>
#include <stdexcept>
#include <thread>

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

// Executor states
namespace model {
	namespace executorStates {
		enum executorStates {
			executorReady = 1,				// This executor can be used
			executorError = 0					// This executor cannot be used
		};
	}

	// Executor types
	namespace executorTypes {
		enum executorTypes {
			executorTypeOpenCL = 0					// OpenCL-based executor
		};
	}
}

// Device-type filers
namespace model {
	namespace filters {
		namespace devices {
			enum devices {
				devicesGPU = 1,					// Graphics processors
				devicesCPU = 2,					// Standard processors
				devicesAPU = 4						// Accelerated processors
			};
		};
	};
}


namespace model {

	// Model scheme types
	namespace rasterDatasets {
		namespace dataValues {
			enum dataValues {
				kBedElevation = 0,		// Bed elevation
				kDepth = 1,		// Depth
				kFreeSurfaceLevel = 2,		// Free surface level
				kVelocityX = 3,		// Initial velocity X
				kVelocityY = 4,		// Initial velocity Y
				kDischargeX = 5,		// Initial discharge X
				kDischargeY = 6,		// Initial discharge Y
				kManningCoefficient = 7,		// Manning coefficient
				kDisabledCells = 8,		// Disabled cells
				kMaxDepth = 9,		// Max depth
				kMaxFSL = 10,		// Max FSL
				kFroudeNumber = 11		// Froude number
			};
		};
	};
};

namespace model {

	// Model scheme types
	namespace schemeTypes {
		enum schemeTypes {
			kGodunov = 0,	// Godunov (first-order)
			kMUSCLHancock = 1,	// MUSCL-Hancock (second-order)
			kInertialSimplification = 2		// Inertial simplification
		};
	}

	// Riemann solver types
	namespace solverTypes {
		enum solverTypes {
			kHLLC = 0		// HLLC approximate
		};
	}

	// Queue mode
	namespace queueMode {
		enum queueMode {
			kAuto = 0,	// Automatic
			kFixed = 1		// Fixed
		};
	}

	// Timestep mode
	namespace timestepMode {
		enum timestepMode {
			kCFL = 0,	// CFL constrained
			kFixed = 1		// Fixed
		};
	}

	// Timestep mode
	namespace syncMethod {
		enum syncMethod {
			kSyncTimestep = 0,						// Timestep synchronised
			kSyncForecast = 1						// Timesteps forecast
		};
	}

}

namespace model
{
	int						loadConfiguration();
	int						commenceSimulation();
	int						closeConfiguration();
	void					outputVersion();
	void					doPause();
	int						doClose(int);

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

	struct SchemeSettings 
	{
		double CourantNumber = 0.5;
		double DryThreshold = 1e-5;
		unsigned char TimestepMode = model::timestepMode::kCFL;
		//unsigned char TimestepMode = model::timestepMode::kFixed;
		double Timestep = 0.01;
		unsigned int ReductionWavefronts = 200;
		bool FrictionStatus = false;
		unsigned char RiemannSolver = model::solverTypes::kHLLC;
		unsigned char CachedWorkgroupSize[2] = { 8, 8 };
		unsigned char NonCachedWorkgroupSize[2] = { 8, 8 };
		unsigned char CacheMode = model::schemeConfigurations::godunovType::kCacheNone;
		//unsigned char CacheMode = model::schemeConfigurations::godunovType::kCacheEnabled;
		unsigned char CacheConstraints = model::cacheConstraints::godunovType::kCacheActualSize;
		//unsigned char CacheConstraints = model::cacheConstraints::godunovType::kCacheAllowOversize;
		//unsigned char CacheConstraints = model::cacheConstraints::godunovType::kCacheAllowUndersize;
		bool ExtrapolatedContiguity = false;
	
	};

	//Todo: Alaa Remove model:: dependency and allow for safe error logging
	extern	CModel* pManager;		// Global Model Class
	extern	CLog* log;				// Global logger class
}

// Platform constant
namespace model {
	namespace env {
		const std::string	platformCode = "WIN";
		const std::string	platformName = "Microsoft Windows";
	}
}

namespace model {
	namespace cli {
		const WORD		colourTimestamp = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN;
		const WORD		colourError = FOREGROUND_RED | FOREGROUND_INTENSITY;
		const WORD		colourHeader = 0x03;
		const WORD		colourMain = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		const WORD		colourInfoBlock = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
	}
}

#endif
