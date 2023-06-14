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
 *  Domain Class: Cartesian grid
 * ------------------------------------------
 *
 */
#ifndef HIPIMS_DOMAIN_CARTESIAN_CDOMAINCARTESIAN_H_
#define HIPIMS_DOMAIN_CARTESIAN_CDOMAINCARTESIAN_H_

#include "opencl.h"

 // TODO: Make a CLocation class
class CDomainCartesian;
class COCLDevice;
class COCLBuffer;
class CScheme;


/*
 *  DOMAIN CLASS
 *  CDomainCartesian
 *
 *  Stores the relevant information required for a
 *  domain based on a simple cartesian grid.
 */
class CDomainCartesian
{

	public:
		CDomainCartesian(CModel* cModel);													// 
		~CDomainCartesian( void );												// Destructor


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
		// - Replacements for CDomain stubs
		virtual	unsigned char	getType()										{ return model::domainStructureTypes::kStructureCartesian; };	// Fetch a type code
		void			logDetails();											// Log details about the domain
		void			readDomain();											// Read Output from gpu
		double*			readDomain_opt_h();										// Read Output flow hieght from gpu to double*
		double*			readDomain_opt_dsdt();									// Read Output dsdt value from gpu to double*
		double*			readDomain_vx();										// Read Output velocity in x-direction from gpu to double*
		double*			readDomain_vy();										// Read Output velocity in y-direction from gpu to double*
		// - Specific to cartesian grids
		void			setRowsCount( unsigned long );							// Set number of cells in a row
		void			setColsCount( unsigned long );							// Set number of cells in a columns
		void			setRealOffset( double, double );						// Set real domain offset (X, Y) for lower-left corner
		void			getRealOffset( double*, double* );						// Fetch real domain offset (X, Y) for lower-left corner
		void			setCellResolution( double );							// Set cell resolution
		void			getCellResolution( double* );							// Fetch cell resolution
		unsigned long	getRows();												// Get the number of rows in the domain
		unsigned long	getCols();												// Get the number of columns in the domain
		double			getVolume();											// Calculate the amount of volume in all the cells

		DomainSummary	getSummary();							// Fetch summary information for this domain
		bool						isInitialised();												// X Is the domain ready to be used
		unsigned	long			getCellCount();													// X Return the total number of cells
		unsigned int				getID() { return uiID; }						// Get the ID number
		void						setID(unsigned int i) { uiID = i; }							// Set the ID number
		unsigned long		getCellID(unsigned long, unsigned long);						// Get the cell ID using an X and Y index
		virtual void 				setDataProgress(mpiSignalDataProgress a) { pDataProgress = a; };	// Set some data on this domain's progress
		void setLogger(CLog* log);




		virtual		bool			isRemote() { return false; };						// Is this domain on this node?
		void						createStoreBuffers(void**, void**, void**, void**, void**, void**, unsigned char);	// Allocates memory and returns pointers to the three arrays (Promaides)
		void						handleInputData(unsigned long, double, unsigned char, unsigned char);	// Handle input data for varying state/static cell variables 
		void						setBedElevation(unsigned long, double);						// Sets the bed elevation for a cell
		void						setManningCoefficient(unsigned long, double);					// Sets the manning coefficient for a cell
		void						setBoundaryCondition(unsigned long, double);					// Sets the boundary condition for a cell
		void						resetBoundaryCondition();										// Resets the boundary condition for a cell
		void						setCouplingCondition(unsigned long, double);					// Sets the coupling condtion for a cell
		void						setdsdt(unsigned long, double);									// Sets the dsdt value for a cell
		void						setFlowStatesValue(unsigned long, model::FlowStates);			// Sets the flow states for a cell
		void						setStateValue(unsigned long, unsigned char, double);			// Sets a state variable
		bool						isDoublePrecision() { return (ucFloatSize == 8); };				// Are we using double-precision?
		double						getBedElevation(unsigned long);								// Gets the bed elevation for a cell
		double						getManningCoefficient(unsigned long);							// Gets the manning coefficient for a cell
		double						getStateValue(unsigned long, unsigned char);					// Gets a state variable
		double						getdsdt(unsigned long);					// Gets dsdt variable
		virtual mpiSignalDataProgress getDataProgress();											// Fetch some data on this domain's progress

		void						setScheme(CScheme*);											// Set the scheme running for this domain
		CScheme* getScheme();													// Get the scheme running for this domain
		void						setDevice(COCLDevice*);										// Set the device responsible for running this domain
		COCLDevice* getDevice();													// Get the device responsible for running this domain
		CExecutorControlOpenCL* cExecutorControlOpenCL;

	private:

		// Private variables
		CLog* logger;
		double			dCellResolution;
		unsigned long	ulRows;
		unsigned long	ulCols;

		unsigned char		ucFloatSize;															// Size of floats used for cell data (bytes)
		char* cSourceDir;																// Data source dir
		char* cTargetDir;																// Output target dir
		cl_double* dCellStates;															// Heap for cell state data
		cl_double* dBedElevations;															// Heap for bed elevations
		cl_double* dManningValues;															// Heap for manning values
		cl_double2* dBoundCoup;																// Heap for boundary and coupling condition values
		cl_double* ddsdt;																// Heap for dsdt promaides varible
		cl_float* fCellStates;															// Heap for cell state date (single)
		cl_float* fBedElevations;															// Heap for bed elevations (single)
		cl_float* fManningValues;															// Heap for manning values (single)
		cl_float2* fBoundCoup;																// Heap for boundary and coupling condition values (single)
		cl_float* fdsdt;																// Heap for dsdt promaides varible (single)
		model::FlowStates* cFlowStates;															// Heap for Flow States values

		cl_double			dMinFSL;																// Min and max FSLs in the domain used for rendering
		cl_double			dMaxFSL;
		cl_double			dMinTopo;																// Min and max topographic levels above datum
		cl_double			dMaxTopo;
		cl_double			dMinDepth;																// Min and max depths 
		cl_double			dMaxDepth;

		CScheme* pScheme;																// Scheme we are running for this particular domain
		COCLDevice* pDevice;																// Device responsible for running this domain

		// Private variables
		unsigned int		uiID;																	// Domain ID
		unsigned int		uiRollbackLimit;														// Iterations after which a rollback is required
		unsigned long		ulCellCount;															// Total number of cells
		mpiSignalDataProgress pDataProgress;														// Data on this domain's progress

};

#endif
