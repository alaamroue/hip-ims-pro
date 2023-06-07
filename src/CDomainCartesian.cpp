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
#include "CExecutorControlOpenCL.h"
#include "CDomainCartesian.h"
#include "Normalplain.h"

/*
 *  Constructor
 */

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
	strcpy_s(&this->cUnits[0],sizeof(&this->cUnits[0]), cUnits);
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
			value = this->getStateValue(ulCellID, model::domainValueIndices::kValueFreeSurfaceLevel);
			//value = this->getBedElevation(ulCellID);
			//value = this->getBedElevation(ulCellID);
			//np->setBedElevation(ulCellID, value*pow(10,3));
		}
	}

	ulCellID = this->getCellID(3, 3);
	//std::cout << this->getStateValue(ulCellID, model::domainValueIndices::kValueFreeSurfaceLevel) << std::endl;
	//np->outputShape();

}

double*	CDomainCartesian::readDomain_opt_h()
{
	unsigned long	ulCellID;
	double* values = new double[this->getRows()* this->getCols()];

	// Read the data back first...
	// TODO: Review whether this is necessary, isn't it a sync point anyway?
	pDevice->blockUntilFinished();
	pScheme->readDomainAll();
	pDevice->blockUntilFinished();


	for (unsigned long iRow = 0; iRow < this->getRows(); ++iRow) {
		for (unsigned long iCol = 0; iCol < this->getCols(); ++iCol) {
			ulCellID = this->getCellID(iCol, iRow);
			values[ulCellID] = this->getStateValue(ulCellID, model::domainValueIndices::kValueFreeSurfaceLevel) - this->getBedElevation(ulCellID);
			//std::cout << this->getStateValue(ulCellID, model::domainValueIndices::kValueDischargeX)*100000 << std::endl;
		}
	}

	return values;

}

double* CDomainCartesian::readDomain_opt_dsdt()
{
	unsigned long	ulCellID;
	double* values = new double[this->getRows() * this->getCols()];

	// Read the data back first...
	// TODO: Review whether this is necessary, isn't it a sync point anyway?
	pDevice->blockUntilFinished();
	pScheme->readDomainAll();
	pDevice->blockUntilFinished();

	for (unsigned long iRow = 0; iRow < this->getRows(); ++iRow) {
		for (unsigned long iCol = 0; iCol < this->getCols(); ++iCol) {
			ulCellID = this->getCellID(iCol, iRow);
			values[ulCellID] = this->getdsdt(ulCellID);
			//std::cout << this->getStateValue(ulCellID, model::domainValueIndices::kValueDischargeX)*100000 << std::endl;
		}
	}

	return values;

}

double* CDomainCartesian::readDomain_vx()
{
	unsigned long	ulCellID;
	double* values = new double[this->getRows() * this->getCols()];

	// Read the data back first...
	// TODO: Review whether this is necessary, isn't it a sync point anyway?
	pDevice->blockUntilFinished();
	pScheme->readDomainAll();
	pDevice->blockUntilFinished();


	//Normalplain* np = new Normalplain(this->getCols(), this->getRows());
	for (unsigned long iRow = 0; iRow < this->getRows(); ++iRow) {
		for (unsigned long iCol = 0; iCol < this->getCols(); ++iCol) {
			ulCellID = this->getCellID(iCol, iRow);
			values[ulCellID] = this->getStateValue(ulCellID, model::domainValueIndices::kValueDischargeX);
		}
	}

	return values;

}

double* CDomainCartesian::readDomain_vy()
{
	unsigned long	ulCellID;
	double* values = new double[this->getRows() * this->getCols()];

	// Read the data back first...
	// TODO: Review whether this is necessary, isn't it a sync point anyway?
	pDevice->blockUntilFinished();
	pScheme->readDomainAll();
	pDevice->blockUntilFinished();


	//Normalplain* np = new Normalplain(this->getCols(), this->getRows());
	for (unsigned long iRow = 0; iRow < this->getRows(); ++iRow) {
		for (unsigned long iCol = 0; iCol < this->getCols(); ++iCol) {
			ulCellID = this->getCellID(iCol, iRow);
			values[ulCellID] = this->getStateValue(ulCellID, model::domainValueIndices::kValueDischargeY);
		}
	}

	return values;

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

