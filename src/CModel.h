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
 *  Main model container
 * ------------------------------------------
 *
 */
#define CL_TARGET_OPENCL_VERSION 300
#ifndef HIPIMS_CMODEL_H_
#define HIPIMS_CMODEL_H_

#include "opencl.h"
#include "CBenchmark.h"
#include <vector>

// Some classes we need to know about...
class CDomainCartesian;
class CExecutorControl;
class CExecutorControlOpenCL;
class CScheme;
class CLog;
class CMPIManager;


/*
 *  APPLICATION CLASS
 *  CModel
 *
 *  Is a singleton class in reality, but need not be enforced.
 */
class CModel
{
	public:

		// Public functions
		CModel(void);															// Constructor
		~CModel(void);															// Destructor

		bool					setExecutor(CExecutorControl*);					// Sets the type of executor to use for the model
		bool					setExecutorToDefaultGPU();						// Sets the type of executor to a Default GPU One
		CExecutorControlOpenCL*	getExecutor(void);								// Gets the executor object currently in use
		CMPIManager*			getMPIManager(void);							// Gets the MPI manager

		bool					runModel(void);									// Execute the model
		void					runModelPrepare(void);							// Prepare for model run
		void					runModelPrepareDomains(void);					// Prepare domains and domain links
		void					runModelMain(void);								// Main model run loop
		void					runModelDomainAssess( bool*, bool* );			// Assess domain states
		void					runModelDomainExchange(void);					// Exchange domain data
		void					runModelUpdateTarget(double);					// Calculate a new target time
		void					runModelSync(void);								// Synchronise domain and timestep data
		void					runModelMPI(void);								// Process MPI queue etc.
		void					runModelSchedule(bool * );	// Schedule work
		void					runModelUI( CBenchmark::sPerformanceMetrics * );// Update progress data etc.
		void					runModelRollback(void);							// Rollback simulation
		void					runModelBlockGlobal(void);						// Block all domains until all are done
		void					runModelBlockNode(void);						// Block further processing on this node only
		void					runModelCleanup(void);							// Clean up after a simulation completes/aborts
		void					runNext(double);								// Run simulation until a certain time is hit

		void					logDetails();									// Spit some info out to the log
		double					getSimulationLength();							// Get total length of simulation
		void					setSimulationLength( double );					// Set total length of simulation
		//unsigned long			getRealStart();									// Get the real world start time (relative to epoch)
		//void					setRealStart( char*, char* = NULL );			// Set the real world start time
		double					getOutputFrequency();							// Get the output frequency
		void					setOutputFrequency( double );					// Set the output frequency
		void					setFloatPrecision( unsigned char );				// Set floating point precision
		unsigned char			getFloatPrecision();							// Get floating point precision
		void					setName( std::string );							// Sets the name
		void					setDescription( std::string );					// Sets the description
		void					logProgress( CBenchmark::sPerformanceMetrics* );			// Write the progress bar etc.
		static void CL_CALLBACK	visualiserCallback( cl_event, cl_int, void * );				// Callback event used when memory reads complete, for visualisation updates
		void					setCourantNumber(double);									// Set the Courant number
		double					getCourantNumber();											// Get the Courant number
		void					setFrictionStatus(bool);									// Enable/disable friction effects
		bool					getFrictionStatus();										// Get enabled/disabled for friction
		void					setCachedWorkgroupSize(unsigned char);						// Set the work-group size
		void					setCachedWorkgroupSize(unsigned char, unsigned char);		// Set the work-group size
		void					setNonCachedWorkgroupSize(unsigned char);					// Set the work-group size
		void					setNonCachedWorkgroupSize(unsigned char, unsigned char);	// Set the work-group size
		CLog*					getLogger();												// Gets a pointer to the logger class
		void					setSelectedDevice(unsigned int id);							// Select a gpu device to build the kernel on
		unsigned int			getSelectedDevice();							// Select a gpu device to build the kernel on
		void					setModelUpdateTarget(double newTarget);			//Set a target time for the simulation to run to
		void					setDomain(CDomainCartesian*);
		CDomainCartesian*		getDomain();

		// Public variables
		CLog*					log;											// Handle for the log singular class
		int						forcedAbort;									//Check if user has force an abort
		double					dCourantNumber;									// Courant number for CFL condition
		bool					bFrictionEffects;														// Activate friction effects?
		cl_ulong				ulCachedWorkgroupSizeX, ulCachedWorkgroupSizeY;
		cl_ulong				ulNonCachedWorkgroupSizeX, ulNonCachedWorkgroupSizeY;
		unsigned int			selectedDevice;
		double					dGlobalTimestep;								//

		bool bSyncReady;
		bool bIdle;
		double dCellRate;
		unsigned char ucRounding;

	private:

		// Private functions
		void					visualiserUpdate();								// Update 3D stuff 

		// Private variables
		CExecutorControlOpenCL*	execController;									// Handle for the executor controlling class
		CDomainCartesian*		domain;
		CMPIManager*			mpiManager;										// Handle for the MPI manager class
		std::string				sModelName;										// Short name for the model
		std::string				sModelDescription;								// Short description of the model
		bool					bDoublePrecision;								// Double precision enabled?
		double					dSimulationTime;								// Total length of simulations
		double					dCurrentTime;									// Current simulation time
		double					dVisualisationTime;								// Current visualisation time
		double					dProcessingTime;								// Total processing time
		double					dOutputFrequency;								// Frequency of outputs
		double					dLastSyncTime;									//
		double					dLastOutputTime;								//
		double					dLastProgressUpdate;							//
		double					dTargetTime;									// 
		double					dEarliestTime;									//
		unsigned long			ulRealTimeStart;
		bool					bRollbackRequired;								// 
		bool					bAllIdle;										//
		bool					bWaitOnLinks;									//
		bool					bSynchronised;									//
		unsigned char			ucFloatSize;									// Size of single/double precision floats used
		cursorCoords			pProgressCoords;								// Buffer coords of the progress output

};

#endif
