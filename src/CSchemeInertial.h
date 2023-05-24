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
 *  Inertial formulation (i.e. simplified)
 * ------------------------------------------
 *
 */
#ifndef HIPIMS_SCHEMES_CSCHEMEINERTIAL_H_
#define HIPIMS_SCHEMES_CSCHEMEINERTIAL_H_

#include "CSchemeGodunov.h"

namespace model {

// Kernel configurations
namespace schemeConfigurations{ 
namespace inertialFormula { enum inertialFormula {
	kCacheNone						= 0,		// No caching
	kCacheEnabled					= 1			// Cache cell state data
}; }  }

namespace cacheConstraints{ 
namespace inertialFormula { enum inertialFormula {
	kCacheActualSize				= 0,		// LDS of actual size
	kCacheAllowOversize				= 1,		// Allow LDS oversizing to avoid bank conflicts
	kCacheAllowUndersize			= 2			// Allow LDS undersizing to avoid bank conflicts
}; }  }

}

/*
 *  SCHEME CLASS
 *  CSchemeInertial
 *
 *  Stores and invokes relevant methods for the actual
 *  hydraulic computations.
 */
class CSchemeInertial : public CSchemeGodunov
{

	public:
		CSchemeInertial(void);											// Constructor
		CSchemeInertial(CLog* logger);											// Constructor
		virtual ~CSchemeInertial( void );									// Destructor

		// Public functions
		virtual void		logDetails();									// Write some details about the scheme
		virtual void		prepareAll();									// Prepare absolutely everything for a model run
		void				setCacheMode( unsigned char );					// Set the cache configuration
		unsigned char		getCacheMode();									// Get the cache configuration
		void				setCacheConstraints( unsigned char );			// Set LDS cache size constraints
		unsigned char		getCacheConstraints();							// Get LDS cache size constraints

	protected:

		// Private functions
		virtual bool		prepareCode();									// Prepare the code required
		virtual void		releaseResources();								// Release OpenCL resources consumed
		bool				prepareInertialKernels();						// Prepare the kernels required
		bool				prepareInertialConstants();						// Assign constants to the executor
		void				releaseInertialResources();						// Release OpenCL resources consumed

};

#endif
