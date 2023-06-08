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
 *  Domain Class
 * ------------------------------------------
 *
 */
#ifndef HIPIMS_DOMAIN_CDOMAINBASE_H_
#define HIPIMS_DOMAIN_CDOMAINBASE_H_

#include <vector>

#include "opencl.h"

namespace model {

// Model domain structure types
namespace domainStructureTypes{ enum domainStructureTypes {
	kStructureCartesian						= 0,	// Cartesian
	kStructureRemote						= 1,	// Remotely held domain
	kStructureInvalid						= 255	// Error state, cannot work with this type of domain
}; }

}

/*
 *  DOMAIN BASE CLASS
 *  CDomainBase
 *
 *  Core base class for domains, even ones which do not reside on this system.
 */
class CDomain;
class CDomainLink;
class CDomainBase
{

	public:

		CDomainBase(void);																			// Constructor
		~CDomainBase(void);																			// Destructor

		// Public structures
		struct DomainSummary
		{
			bool			bAuthoritative;
			unsigned int	uiDomainID;
			unsigned int	uiNodeID;
			unsigned int	uiLocalDeviceID;
			double			dResolution;
			unsigned long	ulRowCount;
			unsigned long	ulColCount;
			unsigned char	ucFloatPrecision;
		};

		struct mpiSignalDataProgress
		{
			unsigned int 	uiDomainID;
			double			dCurrentTimestep;
			double			dCurrentTime;
			double			dBatchTimesteps;
			cl_uint			uiBatchSkipped;
			cl_uint			uiBatchSuccessful;
			unsigned int	uiBatchSize;
		};

		// Public variables
		// ...

		// Public functions
		virtual		DomainSummary	getSummary();													// Fetch summary information for this domain
		virtual		bool			isRemote()				{ return true; };						// Is this domain on this node?

		bool						isInitialised();												// X Is the domain ready to be used
		unsigned	long			getCellCount();													// X Return the total number of cells
		unsigned int				getRollbackLimit()		{ return uiRollbackLimit; }				// How many iterations before a rollback is required?
		unsigned int				getID()					{ return uiID; }						// Get the ID number
		void						setID( unsigned int i ) { uiID = i; }							// Set the ID number
		void						markLinkStatesInvalid();										// When a rollback is initiated, link data becomes invalid
		void						setRollbackLimit();												// Automatically identify a rollback limit
		void						setRollbackLimit( unsigned int i ) { uiRollbackLimit = i; }		// Set the number of iterations before a rollback is required
		virtual unsigned long		getCellID(unsigned long, unsigned long);						// Get the cell ID using an X and Y index
		virtual mpiSignalDataProgress getDataProgress()		{ return pDataProgress; };				// Fetch some data on this domain's progress
		virtual void 				setDataProgress( mpiSignalDataProgress a )	{ pDataProgress = a; };	// Set some data on this domain's progress
		void setLogger(CLog* log);
		CLog* logger;



	protected:

		// Private variables
		unsigned int		uiID;																	// Domain ID
		unsigned int		uiRollbackLimit;														// Iterations after which a rollback is required
		unsigned long		ulCellCount;															// Total number of cells
		mpiSignalDataProgress pDataProgress;														// Data on this domain's progress


};

#endif
