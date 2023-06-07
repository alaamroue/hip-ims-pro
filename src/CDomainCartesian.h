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

#include "CDomain.h"

/*
 *  DOMAIN CLASS
 *  CDomainCartesian
 *
 *  Stores the relevant information required for a
 *  domain based on a simple cartesian grid.
 */
class CDomainCartesian : public CDomain 
{

	public:
		CDomainCartesian(CModel* cModel);													// 
		~CDomainCartesian( void );												// Destructor

		// Public variables
		// ...

		// Public functions
		// - Replacements for CDomain stubs
		virtual	unsigned char	getType()										{ return model::domainStructureTypes::kStructureCartesian; };	// Fetch a type code
		virtual	CDomainBase::DomainSummary getSummary();						// Fetch summary information for this domain
		bool			validateDomain( bool );									// Verify required data is available
		bool		    loadInitialConditions( );					// Load the initial condition raster/constant data
		bool			loadOutputDefinitions();					// Load the output file definitions
		void			prepareDomain();										// Create memory structures etc.
		void			logDetails();											// Log details about the domain
		void			readDomain();											// Read Output from gpu
		double*			readDomain_opt_h();										// Read Output flow hieght from gpu to double*
		double*			readDomain_opt_dsdt();									// Read Output dsdt value from gpu to double*
		double*			readDomain_vx();										// Read Output velocity in x-direction from gpu to double*
		double*			readDomain_vy();										// Read Output velocity in y-direction from gpu to double*
		// - Specific to cartesian grids
		void			imposeBoundaryModification(unsigned char, unsigned char); // Adjust the topography to impose boundary conditions
		void			setRealDimensions( double, double );					// Set real domain dimensions (X, Y)
		void			getRealDimensions( double*, double* );					// Fetch real domain dimensions (X, Y)
		void			setRealOffset( double, double );						// Set real domain offset (X, Y) for lower-left corner
		void			getRealOffset( double*, double* );						// Fetch real domain offset (X, Y) for lower-left corner
		void			setRealExtent( double, double, double, double );		// Set real domain extent (Clockwise: N, E, S, W)
		void			getRealExtent( double*, double*, double*, double* );	// Fetch real domain extent (Clockwise: N, E, S, W)
		void			setCellResolution( double );							// Set cell resolution
		void			getCellResolution( double* );							// Fetch cell resolution
		void			setUnits( char* );										// Set the units
		char*			getUnits();												// Get the units
		void			setProjectionCode( unsigned long );						// Set the EPSG projection code
		unsigned long	getProjectionCode();									// Get the EPSG projection code
		unsigned long	getRows();												// Get the number of rows in the domain
		unsigned long	getCols();												// Get the number of columns in the domain
		virtual unsigned long	getCellID( unsigned long, unsigned long );		// Get the cell ID using an X and Y index
		double			getVolume();											// Calculate the amount of volume in all the cells
		#ifdef _WINDLL
		virtual void	sendAllToRenderer();									// Allows the renderer to read off the bed elevations
		#endif

		enum axis
		{
			kAxisX	= 0,
			kAxisY	= 1
		};

		enum edge
		{
			kEdgeN	= 0,
			kEdgeE	= 1,
			kEdgeS	= 2,
			kEdgeW	= 3
		};

		enum boundaryTreatment
		{
			kBoundaryOpen = 0,
			kBoundaryClosed = 1
		};

		struct pDataset {
			CLog* logger;
			bool bAvailable;
			unsigned long	ulColumns;			
			unsigned long	ulRows;				
			unsigned int	uiBandCount;		
			double			dResolutionX;		
			double			dResolutionY;		
			double			dOffsetX;								
			double			dOffsetY;
			void logDetails() {
				logger->writeDivide();
				logger->writeLine("Dataset band count:  " + std::to_string(this->uiBandCount));
				logger->writeLine("Cell dimensions:     [" + std::to_string(this->ulColumns) + ", " + std::to_string(this->ulRows) + "]");
				logger->writeLine("Cell resolution:     [" + std::to_string(this->dResolutionX) + ", " + std::to_string(this->dResolutionY) + "]");
				logger->writeLine("Lower-left offset:   [" + std::to_string(this->dOffsetX) + ", " + std::to_string(this->dOffsetY) + "]");
				logger->writeDivide();
			}
		};

	private:

		// Private structures
		struct	sDataSourceInfo {
			char*			cSourceType;
			char*			cFileValue;
			unsigned char	ucValue;
		};
		struct	sDataTargetInfo
		{
			char*			cType;
			char*			cFormat;
			unsigned char	ucValue;
			std::string		sTarget;
		};

		// Private variables
		double			dRealDimensions[2];
		double			dRealOffset[2];
		double			dRealExtent[4];
		double			dCellResolution;
		unsigned long	ulRows;
		unsigned long	ulCols;
		unsigned long	ulProjectionCode;
		char			cUnits[2];
		std::vector<sDataTargetInfo>	pOutputs;									// Structure of details about the outputs

		// Private functions
		void			addOutput( sDataTargetInfo );								// Adds a new output 
		void			updateCellStatistics();										// Update the number of rows, cols, etc.

};

#endif
