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
#include <limits>
#include <stdio.h>
#include <cstring>

#include "common.h"
#include "CModel.h"
#include "CDomainManager.h"
#include "CDomain.h"
#include "CScheme.h"
#include "CRasterDataset.h"
#include "CExecutorControlOpenCL.h"
#include "CBoundaryMap.h"
#include "CDomainCartesian.h"
#include "CBoundaryUniform.h"
#include "Normalplain.h"

/*
 *  Constructor
 */
CDomainCartesian::CDomainCartesian(void)
{

}

CDomainCartesian::CDomainCartesian(CModel* cModel)
{
	this->logger = cModel->getLogger();
	this->cExecutorControlOpenCL = cModel->getExecutor();
	this->pDevice = cModel->getExecutor()->getDevice();       //TODO: Alaa:HIGH why is it device 1, and wouldn't this cause problems? Maybe review the old code, I changed a lot in this.
	// Default values will trigger errors on validation
	this->dCellResolution			= std::numeric_limits<double>::quiet_NaN();
	this->dRealDimensions[kAxisX]	= std::numeric_limits<double>::quiet_NaN();
	this->dRealDimensions[kAxisY]	= std::numeric_limits<double>::quiet_NaN();
	this->dRealExtent[kEdgeN]		= std::numeric_limits<double>::quiet_NaN();
	this->dRealExtent[kEdgeE]		= std::numeric_limits<double>::quiet_NaN();
	this->dRealExtent[kEdgeS]		= std::numeric_limits<double>::quiet_NaN();
	this->dRealExtent[kEdgeW]		= std::numeric_limits<double>::quiet_NaN();
	this->dRealOffset[kAxisX]		= std::numeric_limits<double>::quiet_NaN();
	this->dRealOffset[kAxisY]		= std::numeric_limits<double>::quiet_NaN();
	this->cUnits[0]					= 0;
	this->ulProjectionCode			= 0;
	this->cTargetDir				= NULL;
	this->cSourceDir				= NULL;
	this->setProjectionCode(0);					// Unknown
	this->setUnits((char*)"m");
}

/*
 *  Destructor
 */
CDomainCartesian::~CDomainCartesian(void)
{
	// ...
}

/*
 *  Configure the domain using the XML file
 */
bool CDomainCartesian::configureDomain()
{

	//Set Up Boundry
	CBoundaryUniform* pNewBoundary = new CBoundaryUniform(this->getBoundaries()->pDomain);
	pNewBoundary->logger = logger;
	pNewBoundary->sName = std::string("TimeSeriesName");
	pNewBoundary->size = 5;
	pNewBoundary->setValue(model::boundaries::uniformValues::kValueRainIntensity);
	pNewBoundary->pTimeseries = new CBoundaryUniform::sTimeseriesUniform[pNewBoundary->size];
	for (int i = 0; i < pNewBoundary->size; i++){
		pNewBoundary->pTimeseries[i].dTime = i*100000;
		pNewBoundary->pTimeseries[i].dComponent = 11.5;
	}

	pNewBoundary->setVariablesBasedonData();

	this->getBoundaries()->mapBoundaries[pNewBoundary->getName()] = pNewBoundary;


	return true;
}

/*
 *  Load all the initial conditions and data from rasters or constant definitions
 */
bool	CDomainCartesian::loadInitialConditions()
{
	logger->writeLine("Numerical scheme reports is ready.");
	logger->writeLine("Progressing to load initial conditions.");

	bool							bSourceVelX			= false, 
									bSourceVelY			= false, 
									bSourceManning		= false;

	sDataSourceInfo					pDataDEM;
	sDataSourceInfo					pDataDepth;
	std::vector<sDataSourceInfo>	pDataOther;

	sDataSourceInfo pDataInfo;

	//VelocityX
	pDataInfo.cSourceType = "constant";
	pDataInfo.cFileValue = "0.0";
	pDataInfo.ucValue = this->getDataValueCode("velocityx");
	pDataOther.push_back(pDataInfo);

	//VelocityY
	pDataInfo.cSourceType = "constant";
	pDataInfo.cFileValue = "0.0";
	pDataInfo.ucValue = this->getDataValueCode("velocityy");
	pDataOther.push_back(pDataInfo);

	//Depth
	pDataInfo.cSourceType = "constant";
	pDataInfo.cFileValue = "0.0";
	pDataInfo.ucValue = this->getDataValueCode("depth");
	pDataDepth = pDataInfo;

	//manning
	pDataInfo.cSourceType = "constant";
	pDataInfo.cFileValue = "0.0286";
	pDataInfo.ucValue = this->getDataValueCode("manningcoefficient");
	pDataOther.push_back(pDataInfo);
	bSourceManning = true;

	//Bed
	pDataInfo.cSourceType = "raster";
	pDataInfo.cFileValue = "large_fp_topography_imp_v2_with_mountain_WGS.tif";
	pDataInfo.ucValue = this->getDataValueCode("structure,dem");
	pDataDEM = pDataInfo;

	char* cSourceDir = "C:\\Users\\abaghdad\\Desktop\\hipims\\hipims-ocl\\bin\\win32\\debug\\test\\data\\topography\\";

	///////////////////////For Loading Depth Map Grid///////////////////////
	if ( !this->loadInitialConditionSource( pDataDEM, cSourceDir))
	{
		model::doError(
			"Could not load DEM data.",
			model::errorCodes::kLevelWarning
		);
		return false;
	}


	///////////////////////For Loading Water depth Grid///////////////////////
	if ( !this->loadInitialConditionSource( pDataDepth,	cSourceDir ) )
	{
		model::doError(
			"Could not load depth/FSL data.",
			model::errorCodes::kLevelWarning
		);
		return false;
	}

	///////////////////////For Loading Initial velocity and Manning///////////////////////
	for ( unsigned int i = 0; i < pDataOther.size(); ++i )
	{
		if ( !this->loadInitialConditionSource( pDataOther[i], cSourceDir ) ) 
		{
			model::doError(
				"Could not load initial conditions.",
				model::errorCodes::kLevelWarning
			);
			return false;
		}
	}

	return true;
}

/*
 *  Load the output definitions for what should be written to disk
 */
bool	CDomainCartesian::loadOutputDefinitions()
{

	sDataTargetInfo	pOutput;

	pOutput.cFormat = "GTiff";
	pOutput.cType = "raster";
	pOutput.sTarget = "C:\\Users\\abaghdad\\Desktop\\hipims\\hipims-ocl\\bin\\win32\\debug\\test\\data\\output\\depth_%t.img";
	pOutput.ucValue = this->getDataValueCode("depth");

	addOutput( pOutput );

	logger->writeLine( "Identified " + std::to_string( this->pOutputs.size() ) + " output file definition(s)." );

	return true;
}

/*
 *  Read a data source raster or constant using the pre-parsed data held in the structure
 */
bool	CDomainCartesian::loadInitialConditionSource( sDataSourceInfo pDataSource, char* cDataDir )
{
	if ( strcmp( pDataSource.cSourceType, "raster" ) == 0 )
	{
		CRasterDataset	pDataset;
		unsigned long	ulCellID;
		double			dValue;
		unsigned char	ucRounding = 4;			// decimal places

		std::string sFilename = "C:\\Users\\abaghdad\\Desktop\\hipims\\hipims-ocl\\bin\\win32\\debug\\test\\data\\topography\\large_fp_topography_imp_v2_with_mountain_WGS.tif";
		//pDataset.gdDataset = static_cast<GDALDataset*>(GDALOpen(sFilename.c_str(), GA_ReadOnly));
		pDataset.bAvailable = true;
		pDataset.ulColumns = 100;
		pDataset.ulRows = 100;
		pDataset.uiBandCount = 1;
		pDataset.dResolutionX = 100.0;
		pDataset.dResolutionY = 100.00;
		pDataset.dOffsetX = 0.00;
		pDataset.dOffsetY = 0.00;

		//TODO: Alaa:MEDIUM Rows vs Coloumns which is correct?
		Normalplain np = Normalplain(pDataset.ulRows, pDataset.ulColumns);
		np.SetBedElevationMountain();

		for (unsigned long iRow = 0; iRow < pDataset.ulRows; iRow++)
		{
			for (unsigned long iCol = 0; iCol < pDataset.ulColumns; iCol++)
			{
				ulCellID = this->getCellID(iCol, pDataset.ulRows - iRow - 1);		// Scan lines start in the top left
				dValue = np.getBedElevation(ulCellID);
				this->handleInputData(
					ulCellID,
					dValue,
					pDataSource.ucValue,
					ucRounding
				);
			}
		}

	} 
	else if ( strcmp( pDataSource.cSourceType, "constant" ) == 0 )
	{
		double	dValue = atof(pDataSource.cFileValue);

		// NOTE: The outer layer of cells are exempt here, as they are not computed
		// but there may be some circumstances where we want a value here?
		for( unsigned long i = 0; i < this->getCols(); i++ )
		{
			for( unsigned long j = 0; j < this->getRows(); j++ )
			{
				unsigned long ulCellID = this->getCellID( i, j );
				if (i <= 0 || 
					j <= 0 ||
					i >= this->getCols() - 1 || 
					j >= this->getRows() - 1)
				{
					double dEdgeValue = 0.0;
					if (pDataSource.ucValue == model::rasterDatasets::dataValues::kFreeSurfaceLevel)
						dEdgeValue = this->getBedElevation( ulCellID );
					this->handleInputData(
						ulCellID,
						dEdgeValue,
						pDataSource.ucValue,
						4	// TODO: Allow rounding to be configured for source constants
					);
				}
				else 
				{
					this->handleInputData(
						ulCellID,
						dValue,
						pDataSource.ucValue,
						4	// TODO: Allow rounding to be configured for source constants
					);
				}
			}
		}
	} 
	else 
	{
		model::doError(
			"Unrecognised data source type.",
			model::errorCodes::kLevelWarning
		);
		return false;
	}

	return true;
}

/*
 *  Does the domain contain all of the required data yet?
 */
bool	CDomainCartesian::validateDomain( bool bQuiet )
{
	// Got a resolution?
	if ( this->dCellResolution == NAN )
	{
		if ( !bQuiet ) model::doError(
			"Domain cell resolution not defined",
			model::errorCodes::kLevelWarning
		);
		return false;
	}

	// Got a size?
	if ( ( isnan( this->dRealDimensions[ kAxisX ] ) || 
		   isnan( this->dRealDimensions[ kAxisY ] ) ) &&
		 ( isnan( this->dRealExtent[ kEdgeN ] ) || 
		   isnan( this->dRealExtent[ kEdgeE ] ) || 
		   isnan( this->dRealExtent[ kEdgeS ] ) || 
		   isnan( this->dRealExtent[ kEdgeW ] ) ) )
	{
		if ( !bQuiet ) model::doError(
			"Domain extent not defined",
			model::errorCodes::kLevelWarning
		);
		return false;
	}

	// Got an offset?
	if ( isnan( this->dRealOffset[ kAxisX ] ) ||
		 isnan( this->dRealOffset[ kAxisY ] ) )
	{
		if ( !bQuiet ) model::doError(
			"Domain offset not defined",
			model::errorCodes::kLevelWarning
		);
		return false;
	}

	// Valid extent?
	if ( this->dRealExtent[ kEdgeE ] <= this->dRealExtent[ kEdgeW ] ||
		 this->dRealExtent[ kEdgeN ] <= this->dRealExtent[ kEdgeS ] )
	{
		if ( !bQuiet ) model::doError(
			"Domain extent is not valid",
			model::errorCodes::kLevelWarning
		);
		return false;
	}

	return true;
}

/*
 *  Create the required data structures etc.
 */
void	CDomainCartesian::prepareDomain()
{
	if ( !this->validateDomain( true ) ) 
	{
		model::doError(
			"Cannot prepare the domain. Invalid specification.",
			model::errorCodes::kLevelModelStop
		);
		return;
	}

	this->bPrepared = true;
	this->logDetails();
}

/*
 *  Write useful information about this domain to the log file
 */
void	CDomainCartesian::logDetails()
{
	logger->writeDivide();
	unsigned short	wColour			= model::cli::colourInfoBlock;

	logger->writeLine( "REGULAR CARTESIAN GRID DOMAIN", true, wColour );
	if ( this->ulProjectionCode > 0 ) 
	{
		logger->writeLine( "  Projection:        EPSG:" + std::to_string( this->ulProjectionCode ), true, wColour );
	} else {
		logger->writeLine( "  Projection:        Unknown", true, wColour );
	}
	logger->writeLine("  Device number:     " + std::to_string(this->pDevice->uiDeviceNo), true, wColour);
	logger->writeLine("  Cell count:        " + std::to_string(this->ulCellCount), true, wColour);
	logger->writeLine("  Cell resolution:   " + std::to_string(this->dCellResolution) + this->cUnits, true, wColour);
	logger->writeLine("  Cell dimensions:   [" + std::to_string(this->ulCols) + ", " +
		std::to_string(this->ulRows) + "]", true, wColour);
	logger->writeLine("  Real dimensions:   [" + std::to_string(this->dRealDimensions[kAxisX]) + this->cUnits + ", " +
		std::to_string(this->dRealDimensions[kAxisY]) + this->cUnits + "]", true, wColour);

	logger->writeDivide();
}

/*
 *  Set real domain dimensions (X, Y)
 */
void	CDomainCartesian::setRealDimensions(double dSizeX, double dSizeY)
{
	this->dRealDimensions[kAxisX] = dSizeX;
	this->dRealDimensions[kAxisY] = dSizeY;
	this->updateCellStatistics();
}

/*
 *  Fetch real domain dimensions (X, Y)
 */
void	CDomainCartesian::getRealDimensions(double* dSizeX, double* dSizeY)
{
	*dSizeX = this->dRealDimensions[kAxisX];
	*dSizeY = this->dRealDimensions[kAxisY];
}

/*
 *  Set real domain offset (X, Y) for lower-left corner
 */
void	CDomainCartesian::setRealOffset(double dOffsetX, double dOffsetY)
{
	this->dRealOffset[kAxisX] = dOffsetX;
	this->dRealOffset[kAxisY] = dOffsetY;
}

/*
 *  Fetch real domain offset (X, Y) for lower-left corner
 */
void	CDomainCartesian::getRealOffset(double* dOffsetX, double* dOffsetY)
{
	*dOffsetX = this->dRealOffset[kAxisX];
	*dOffsetY = this->dRealOffset[kAxisY];
}

/*
 *  Set real domain extent (Clockwise: N, E, S, W)
 */
void	CDomainCartesian::setRealExtent(double dEdgeN, double dEdgeE, double dEdgeS, double dEdgeW)
{
	this->dRealExtent[kEdgeN] = dEdgeN;
	this->dRealExtent[kEdgeE] = dEdgeE;
	this->dRealExtent[kEdgeS] = dEdgeS;
	this->dRealExtent[kEdgeW] = dEdgeW;
	//this->updateCellStatistics();
}

/*
 *  Fetch real domain extent (Clockwise: N, E, S, W)
 */
void	CDomainCartesian::getRealExtent(double* dEdgeN, double* dEdgeE, double* dEdgeS, double* dEdgeW)
{
	*dEdgeN = this->dRealExtent[kEdgeN];
	*dEdgeE = this->dRealExtent[kEdgeE];
	*dEdgeS = this->dRealExtent[kEdgeS];
	*dEdgeW = this->dRealExtent[kEdgeW];
}

/*
 *  Set cell resolution
 */
void	CDomainCartesian::setCellResolution(double dResolution)
{
	this->dCellResolution = dResolution;
	this->updateCellStatistics();
}

/*
 *   Fetch cell resolution
 */
void	CDomainCartesian::getCellResolution(double* dResolution)
{
	*dResolution = this->dCellResolution;
}

/*
 *  Set domain units
 */
void	CDomainCartesian::setUnits(char* cUnits)
{
	if (std::strlen(cUnits) > 2)
	{
		model::doError(
			"Domain units can only be two characters",
			model::errorCodes::kLevelWarning
		);
		return;
	}

	// Store the units (2 char max!)
	std::strcpy(&this->cUnits[0], cUnits);
}

/*
 *  Return a couple of characters representing the units in use
 */
char* CDomainCartesian::getUnits()
{
	return &this->cUnits[0];
}

/*
 *  Set the EPSG projection code currently in use
 *  0 = Not defined
 */
void	CDomainCartesian::setProjectionCode(unsigned long ulProjectionCode)
{
	this->ulProjectionCode = ulProjectionCode;
}

/*
 *  Return the EPSG projection code currently in use
 */
unsigned long	CDomainCartesian::getProjectionCode()
{
	return this->ulProjectionCode;
}

/*
 *  Update basic statistics on the number of cells etc.
 */
void	CDomainCartesian::updateCellStatistics()
{
	// Do we have enough information to proceed?...

	// Got a resolution?
	if (this->dCellResolution == NAN)
	{
		return;
	}

	// Got a size?
	if ((isnan(this->dRealDimensions[kAxisX]) ||
		isnan(this->dRealDimensions[kAxisY])) &&
		(isnan(this->dRealExtent[kEdgeN]) ||
			isnan(this->dRealExtent[kEdgeE]) ||
			isnan(this->dRealExtent[kEdgeS]) ||
			isnan(this->dRealExtent[kEdgeW])))
	{
		return;
	}

	// We've got everything we need...
	this->ulRows = static_cast<unsigned long>(
		this->dRealDimensions[kAxisY] / this->dCellResolution
		);
	this->ulCols = static_cast<unsigned long>(
		this->dRealDimensions[kAxisX] / this->dCellResolution
		);
	this->ulCellCount = this->ulRows * this->ulCols;
}

/*
 *  Return the total number of rows in the domain
 */
unsigned long	CDomainCartesian::getRows()
{
	return this->ulRows;
}

/*
 *  Return the total number of columns in the domain
 */
unsigned long	CDomainCartesian::getCols()
{
	return this->ulCols;
}

/*
 *  Get a cell ID from an X and Y index
 */
unsigned long	CDomainCartesian::getCellID(unsigned long ulX, unsigned long ulY)
{
	return (ulY * this->getCols()) + ulX;
}

/*
 *  Get a cell ID from an X and Y coordinate
 */
unsigned long	CDomainCartesian::getCellFromCoordinates(double dX, double dY)
{
	unsigned long ulX = floor((dX - dRealOffset[0]) / dCellResolution);
	unsigned long ulY = floor((dY - dRealOffset[2]) / dCellResolution);
	return getCellID(ulX, ulY);
}

#ifdef _WINDLL
/*
 *  Send the topography to the renderer for visualisation purposes
 */
void	CDomainCartesian::sendAllToRenderer()
{
	// TODO: This needs fixing and changing to account for multi-domain stuff...
	if (pManager->getDomainSet()->getDomainCount() > 1)
		return;

	if (!this->isInitialised() ||
		(this->ucFloatSize == 8 && this->dBedElevations == NULL) ||
		(this->ucFloatSize == 4 && this->fBedElevations == NULL) ||
		(this->ucFloatSize == 8 && this->dCellStates == NULL) ||
		(this->ucFloatSize == 4 && this->fCellStates == NULL))
		return;

	#ifdef _WINDLL
	if (model::fSendTopography != NULL)
		model::fSendTopography(
			this->ucFloatSize == 8 ?
			static_cast<void*>(this->dBedElevations) :
			static_cast<void*>(this->fBedElevations),
			this->ucFloatSize == 8 ?
			static_cast<void*>(this->dCellStates) :
			static_cast<void*>(this->fCellStates),
			this->ucFloatSize,
			this->ulCols,
			this->ulRows,
			this->dCellResolution,
			this->dCellResolution,
			this->dMinDepth,
			this->dMaxDepth,
			this->dMinTopo,
			this->dMaxTopo
		);
	#endif
}
#endif


/*
 *  Calculate the total volume present in all of the cells
 */
double	CDomainCartesian::getVolume()
{
	double dVolume = 0.0;

	for (unsigned int i = 0; i < this->ulCellCount; ++i)
	{
		if (this->isDoublePrecision())
		{
			dVolume += (this->dCellStates[i].s[0] - this->dBedElevations[i]) *
				this->dCellResolution * this->dCellResolution;
		}
		else {
			dVolume += (this->fCellStates[i].s[0] - this->fBedElevations[i]) *
				this->dCellResolution * this->dCellResolution;
		}
	}

	return dVolume;
}

/*
 *  Add a new output
 */
void	CDomainCartesian::addOutput(sDataTargetInfo pOutput)
{
	this->pOutputs.push_back(pOutput);
}

/*
 *  Manipulate the topography to impose boundary conditions
 */
void	CDomainCartesian::imposeBoundaryModification(unsigned char ucDirection, unsigned char ucType)
{
	unsigned long ulMinX, ulMaxX, ulMinY, ulMaxY;

	if (ucDirection == edge::kEdgeE)
	{
		ulMinY = 0; ulMaxY = this->ulRows - 1; ulMinX = this->ulCols - 1; ulMaxX = this->ulCols - 1;
	};
	if (ucDirection == edge::kEdgeW)
	{
		ulMinY = 0; ulMaxY = this->ulRows - 1; ulMinX = 0; ulMaxX = 0;
	};
	if (ucDirection == edge::kEdgeN)
	{
		ulMinY = this->ulRows - 1; ulMaxY = this->ulRows - 1; ulMinX = 0; ulMaxX = this->ulCols - 1;
	};
	if (ucDirection == edge::kEdgeS)
	{
		ulMinY = 0; ulMaxY = 0; ulMinX = 0; ulMaxX = this->ulCols - 1;
	};

	for (unsigned long x = ulMinX; x <= ulMaxX; x++)
	{
		for (unsigned long y = ulMinY; y <= ulMaxY; y++)
		{
			if (ucType == CDomainCartesian::boundaryTreatment::kBoundaryClosed)
			{
				this->setBedElevation(
					this->getCellID(x, y),
					9999.9
				);
			}
		}
	}
}

/*
 *  Write output files to disk
 */
void	CDomainCartesian::writeOutputs()
{
	// Read the data back first...
	// TODO: Review whether this is necessary, isn't it a sync point anyway?
	pDevice->blockUntilFinished();
	pScheme->readDomainAll();
	pDevice->blockUntilFinished();

	for (unsigned int i = 0; i < this->pOutputs.size(); ++i)
	{
		// Replaces %t with the time in the filename, if required
		// TODO: Allow for decimal output filenames
		std::string sFilename = this->pOutputs[i].sTarget;
		std::string sTime = std::to_string( floor( pScheme->getCurrentTime() * 100.0 ) / 100.0 );
		unsigned int uiTimeLocation = sFilename.find( "%t" );
		if ( uiTimeLocation != std::string::npos )
			sFilename.replace( uiTimeLocation, 2, sTime );

		//TODO: Alaa:HIGH Write replacement
		//
		//CRasterDataset::domainToRaster(
		//	this->pOutputs[i].cFormat,
		//	sFilename,
		//	this,
		//	this->pOutputs[i].ucValue
		//);
	}
}


void	CDomainCartesian::readDomain()
{
	unsigned long	ulCellID;
	double value;
	// Read the data back first...
	// TODO: Review whether this is necessary, isn't it a sync point anyway?
	pDevice->blockUntilFinished();
	pScheme->readDomainAll();
	pDevice->blockUntilFinished();


	//Normalplain* np = new Normalplain(this->getCols(), this->getRows());
	for (unsigned long iRow = 0; iRow < this->getRows(); ++iRow) {
		for (unsigned long iCol = 0; iCol < this->getCols(); ++iCol) {
			ulCellID = this->getCellID(iCol, iRow);
			value = this->getStateValue(ulCellID, model::domainValueIndices::kValueDischargeX);
			//value = this->getBedElevation(ulCellID);
			//value = this->getBedElevation(ulCellID);
			//np->setBedElevation(ulCellID, value*pow(10,3));
		}
	}

	ulCellID = this->getCellID(10, 10);
	std::cout << this->getStateValue(ulCellID, model::domainValueIndices::kValueDischargeX) << std::endl;
	//np->outputShape();

}


/*
 *	Fetch summary information for this domain
 */
CDomainBase::DomainSummary CDomainCartesian::getSummary()
{
	CDomainBase::DomainSummary pSummary;
	
	pSummary.bAuthoritative = true;

	pSummary.uiDomainID		= this->uiID;
#ifdef MPI_ON
	pSummary.uiNodeID = pManager->getMPIManager()->getNodeID();
#else
	pSummary.uiNodeID = 0;
#endif
	pSummary.uiLocalDeviceID = this->getDevice()->getDeviceID();
	pSummary.dEdgeNorth		= this->dRealExtent[kEdgeN];
	pSummary.dEdgeEast		= this->dRealExtent[kEdgeE];
	pSummary.dEdgeSouth		= this->dRealExtent[kEdgeS];
	pSummary.dEdgeWest		= this->dRealExtent[kEdgeW];
	pSummary.ulColCount		= this->ulCols;
	pSummary.ulRowCount		= this->ulRows;
	pSummary.ucFloatPrecision = ( this->isDoublePrecision() ? model::floatPrecision::kDouble : model::floatPrecision::kSingle );
	pSummary.dResolution	= this->dCellResolution;

	return pSummary;
}

