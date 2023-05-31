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
 *  Raster dataset handling class
 *  Uses and invokves GDAL functionality
 * ------------------------------------------
 *
 */
#ifndef HIPIMS_DATASETS_CRASTERDATASET_H_
#define HIPIMS_DATASETS_CRASTERDATASET_H_

#include "CDomain.h"
#include "CDomainCartesian.h"
#include "CBoundaryGridded.h"

/*
 *  RASTER DATASET CLASS
 *  CRasterDataset
 *
 *  Provides access for loading and saving
 *  raster datasets. Uses GDAL.
 */
class CRasterDataset
{

	public:

		CRasterDataset( void );																				// Constructor
		~CRasterDataset( void );																			// Destructor

		// Public variables
		// ...

		// Public functions
		static void		registerAll();																		// Register types for use, must be called first
		static void		cleanupAll();																		// Cleanup memory after use. Not perfect... 
		//static bool		domainToRaster( const char*, std::string, CDomainCartesian*, unsigned char );		// Open a file as the dataset for writing
		//bool			openFileRead( std::string );														// Open a file as the dataset for reading
		//void			readMetadata();																		// Read metadata for the dataset
		void			logDetails();																// Write details (mainly metdata) to the log
		bool			applyDimensionsToDomain( CDomainCartesian*, CLog* log);								// Applies the dimensions, offset and scaling to a domain
		//bool			applyDataToDomain( unsigned char, CDomainCartesian* );								// Applies first band of data in the raster to a domain variable
		CBoundaryGridded::SBoundaryGridTransform* createTransformationForDomain(CDomainCartesian*);			// Create a transformation to match the domain
		//double*			createArrayForBoundary(CBoundaryGridded::SBoundaryGridTransform*);					// Create an array for a boundary condition
		//GDALDataset* gdDataset;																			    // Pointer to the dataset
		bool			bAvailable;																			// Raster successfully open and available?
		unsigned long	ulColumns;																			// Number of columns
		unsigned long	ulRows;																				// Number of rows
		unsigned int	uiBandCount;																		// Number of bands in the file
		double			dResolutionX;																		// Cell resolution in X-direction
		double			dResolutionY;																		// Cell resolution in Y-direction
		double			dOffsetX;																			// LL corner offset X
		double			dOffsetY;																			// LL corner offset Y
		unsigned int	uiEPSGCode;																			// EPSG projection code (0 if none)
		void setLogger(CLog* log);
		CLog* logger;

	private:

		// Private functions
		static void		getValueDetails( unsigned char, std::string* );										// Fetch some data on a value, like its index in the CDomain array
		bool			isDomainCompatible( CDomainCartesian* );											// Is the domain compatible (i.e. row/column count, etc.) with this raster?

		// Private variables
		char*			cProjectionName;																	// String description of the projection
		char*			cUnits;																				// Units used 
																					//Pointer to Log Class

		// Friendships
		// ...

};

#endif