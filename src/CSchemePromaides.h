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
 *  Promaides formulation (i.e. simplified)
 * ------------------------------------------
 *
 */
#pragma once

#include "CSchemeGodunov.h"


/*
 *  SCHEME CLASS
 *  CSchemePromaides
 *
 *  Stores and invokes relevant methods for the actual
 *  hydraulic computations.
 */
class CSchemePromaides : public CSchemeGodunov
{

	public:

		CSchemePromaides( void );											// Constructor
		virtual ~CSchemePromaides( void );									// Destructor

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
		bool				preparePromaidesKernels();						// Prepare the kernels required
		void				releasePromaidesResources();						// Release OpenCL resources consumed

};
