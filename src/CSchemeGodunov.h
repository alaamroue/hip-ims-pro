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
 *  Godunov-type scheme class
 * ------------------------------------------
 *
 */
#ifndef HIPIMS_SCHEMES_CSCHEMEGODUNOV_H_
#define HIPIMS_SCHEMES_CSCHEMEGODUNOV_H_

#include "CScheme.h"
#include "Normalplain.h"
#include <mutex>

/*
 *  SCHEME CLASS
 *  CSchemeGodunov
 *
 *  Stores and invokes relevant methods for the actual
 *  hydraulic computations using a first-order scheme
 */
class CSchemeGodunov : public CScheme
{

	public:
		CSchemeGodunov(void);
		CSchemeGodunov(CModel* cmodel);															// Default constructor
		virtual ~CSchemeGodunov( void );											// Destructor

		// Public functions
		virtual void		setupFromConfig();									// Set up the scheme
		virtual void		logDetails();											// Write some details about the scheme
		virtual void		prepareAll();											// Prepare absolutely everything for a model run
		virtual void		scheduleIteration( bool,								// Schedule an iteration of the scheme
											   COCLDevice*,
											   CDomain* );					
		double				proposeSyncPoint( double );								// Propose a synchronisation point
		void				forceTimestep( double );								// Force a specific timestep
		void				setDryThreshold( double );								// Set the dry cell threshold depth
		double				getDryThreshold();										// Get the dry cell threshold depth
		void				setReductionWavefronts( unsigned int );					// Set number of wavefronts used in reductions
		unsigned int		getReductionWavefronts();								// Get number of wavefronts used in reductions
		void				setRiemannSolver( unsigned char );						// Set the Riemann solver to use
		unsigned char		getRiemannSolver();										// Get the Riemann solver in use
		void				setCacheMode( unsigned char );							// Set the cache configuration
		unsigned char		getCacheMode();											// Get the cache configuration
		void				setCacheConstraints( unsigned char );					// Set LDS cache size constraints
		unsigned char		getCacheConstraints();									// Get LDS cache size constraints
		void				setCachedWorkgroupSize( unsigned char );				// Set the work-group size
		void				setCachedWorkgroupSize( unsigned char, unsigned char );	// Set the work-group size
		void				setNonCachedWorkgroupSize( unsigned char );				// Set the work-group size
		void				setNonCachedWorkgroupSize( unsigned char, unsigned char );	// Set the work-group size
		void				setTargetTime( double );								// Set the target sync time
		double				getAverageTimestep();									// Get batch average timestep
		virtual COCLBuffer*	getLastCellSourceBuffer();								// Get the last source cell state buffer
		virtual COCLBuffer*	getNextCellSourceBuffer();								// Get the next source cell state buffer

		static DWORD		Threaded_runBatchLaunch(LPVOID param);
		void				runBatchThread();
		void				Threaded_runBatch();

		virtual void		readDomainAll();										// Read back all domain data
		virtual void		importLinkZoneData();									// Load in data
		virtual void		prepareSimulation();									// Set everything up to start running for this domain
		virtual void		readKeyStatistics();									// Fetch the key details back to the right places in memory
		virtual void		runSimulation( double, double);						// Run this simulation until the specified time
		virtual void		cleanupSimulation();									// Dispose of transient data and clean-up this domain
		virtual void		saveCurrentState();										// Save current cell states
		virtual void		forceTimeAdvance();										// Force time advance (when synced will stall)
		virtual void		rollbackSimulation( double, double );					// Roll back cell states to the last successful round
		virtual bool		isSimulationFailure( double );							// Check whether we successfully reached a specific time
		virtual bool		isSimulationSyncReady( double );						// Are we ready to synchronise? i.e. have we reached the set sync time?

		COCLKernel*			oclKernelTimestepUpdate;
		COCLKernel*			oclKernelResetCounters;
		COCLKernel*			oclKernelTimestepReduction;
		COCLBuffer*			oclBufferCellStates;
		COCLBuffer*			oclBufferCellStatesAlt;

		double				dLastSyncTime;											// What was the last synchronisation time?
		COCLBuffer*			oclBufferTime;
		COCLBuffer*			oclBufferTimeTarget;
		bool				bUseForcedTimeAdvance;									// Force the timestep to be advanced next time?
		COCLKernel*			oclKernelFullTimestep;
		COCLKernel*			oclKernelFriction;
		COCLKernel*			oclKernelTimeAdvance;
		Normalplain*		np;
		bool				bDownloadLinks;											// Download dependent links?
	protected:

		// Private variables
		cl_ulong			ulCachedWorkgroupSizeX, ulCachedWorkgroupSizeY;
		cl_ulong			ulNonCachedWorkgroupSizeX, ulNonCachedWorkgroupSizeY;
		cl_ulong			ulCachedGlobalSizeX, ulCachedGlobalSizeY;
		cl_ulong			ulNonCachedGlobalSizeX, ulNonCachedGlobalSizeY;
		cl_ulong			ulBoundaryCellWorkgroupSize;
		cl_ulong			ulBoundaryCellGlobalSize;
		cl_ulong			ulReductionWorkgroupSize;
		cl_ulong			ulReductionGlobalSize;

		unsigned char		ucConfiguration;										// Kernel configuration in-use
		unsigned char		ucCacheConstraints;										// Kernel LDS cache constraints
		unsigned char		ucSolverType;											// Code for the Riemann solver type
		unsigned char		ucSyncMethod;											// Synchronisation method employed
		double				dThresholdVerySmall;									// Threshold value for 'very small'
		double				dThresholdQuiteSmall;									// Threshold value for 'quite small'
		bool				bDebugOutput;											// Debug output enabled in the scheme?
		bool				bFrictionInFluxKernel;									// Process friction in the flux kernel?
		bool				bUseAlternateKernel;									// Use the alternate kernel configurations (for memory)
		bool				bOverrideTimestep;										// Force set the timestep next time?
		bool				bUpdateTargetTime;										// Update the target time?
		bool				bImportLinks;											// Import link data?
		bool				bIncludeBoundaries;										// Boundary condition kernel is required?
		bool				bCellStatesSynced;										// Are the host cell states synchronised with the compute device?
		unsigned int		uiDebugCellX;											// Debug info cell X
		unsigned int		uiDebugCellY;											// Debug info cell Y
		unsigned int		uiTimestepReductionWavefronts;							// Number of wavefronts used in reduction
		cl_double4*			dBoundaryTimeSeries;									// Boundary time series data
		cl_float4*			fBoundaryTimeSeries;									// Boundary time series data
		cl_ulong*			ulBoundaryRelationCells;								// Boundary to cell relations
		cl_uint*			uiBoundaryRelationSeries;								// Target series for the boundary to cell relations
		cl_uint*			uiBoundaryParameters;									// Boundary parameters bitmask
		
		// Private functions
		virtual bool		prepareCode();											// Prepare the code required
		virtual void		releaseResources();										// Release OpenCL resources consumed
		virtual bool		prepareBoundaries();									// Prepare the boundary conditions and time series
		bool				prepareGeneralKernels();								// Prepare the general kernels required
		bool				prepare1OKernels();										// Prepare the kernels required
		bool				prepare1OConstants();									// Assign constants to the executor
		bool				prepare1OMemory();										// Prepare memory buffers required
		bool				prepare1OExecDimensions();								// Size the problem for execution
		void				release1OResources();									// Release 1st-order OpenCL resources consumed

		// OpenCL elements
		COCLProgram*		oclModel;
		COCLBuffer*			oclBufferCellManning;
		COCLBuffer*			oclBufferCellBed;
		COCLBuffer*			oclBufferTimestep;
		COCLBuffer*			oclBufferTimeHydrological;
		COCLBuffer*			oclBufferTimestepReduction;
		COCLBuffer*			oclBufferBatchTimesteps;
		COCLBuffer*			oclBufferBatchSuccessful;
		COCLBuffer*			oclBufferBatchSkipped;

};

#endif