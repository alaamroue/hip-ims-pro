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

	// Flow States
	struct FlowStates
	{
		bool isFlowElement;
		bool noflow_x;
		bool noflow_y;
		bool noflow_nx;
		bool noflow_ny;
		bool opt_pol_x;
		bool opt_pol_y;
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
				kFroudeNumber = 11,		// Froude number
			};
		};
	};

	// Model domain structure types
	namespace domainValueIndices {
		enum domainValueIndices {
			kValueFreeSurfaceLevel = 0,	// Free-surface level
			kValueMaxFreeSurfaceLevel = 1,	// Max free-surface level
			kValueDischargeX = 2,	// Discharge X
			kValueDischargeY = 3		// Discharge Y
		};
	}
	// Model domain structure types
	namespace domainStructureTypes {
		enum domainStructureTypes {
			kStructureCartesian = 0,	// Cartesian
			kStructureRemote = 1,	// Remotely held domain
			kStructureInvalid = 255	// Error state, cannot work with this type of domain
		};
	}



	extern	CModel*			cModel;
	void					doError( std::string, unsigned char );
}




// Variables in use throughput
//using	model::pManager;
