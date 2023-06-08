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
	this->pDevice = cModel->getExecutor()->getDevice();

	this->dCellResolution			= std::numeric_limits<double>::quiet_NaN();

	this->ulRows = 0;
	this->ulCols = 0;
}

/*
 *  Destructor
 */
CDomainCartesian::~CDomainCartesian(void)
{
	// ...
}


/*
 *  Write useful information about this domain to the log file
 */
void	CDomainCartesian::logDetails()
{
	logger->writeDivide();
	unsigned short	wColour			= model::cli::colourInfoBlock;

	logger->writeLine( "REGULAR CARTESIAN GRID DOMAIN", true, wColour );
	logger->writeLine("  Device number:     " + std::to_string(this->pDevice->uiDeviceNo), true, wColour);
	logger->writeLine("  Cell count:        " + std::to_string(this->ulCellCount), true, wColour);
	logger->writeLine("  Cell resolution:   " + std::to_string(this->dCellResolution) + "m", true, wColour);
	logger->writeLine("  Cell dimensions:   [" + std::to_string(this->ulCols) + ", " +
		std::to_string(this->ulRows) + "]", true, wColour);

	logger->writeDivide();
}

/*
 *  Set the number of cell in a row
 */
void	CDomainCartesian::setRowsCount(unsigned long count)
{
	this->ulRows = count;
	this->ulCellCount = this->ulRows * this->ulCols;
}

/*
 *  Set the number of cell in a columns
 */
void	CDomainCartesian::setColsCount(unsigned long count)
{
	this->ulCols = count;
	this->ulCellCount = this->ulRows * this->ulCols;
}

/*
 *  Set cell resolution
 */
void	CDomainCartesian::setCellResolution(double dResolution)
{
	this->dCellResolution = dResolution;
}

/*
 *   Fetch cell resolution
 */
void	CDomainCartesian::getCellResolution(double* dResolution)
{
	*dResolution = this->dCellResolution;
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
CDomain::DomainSummary CDomainCartesian::getSummary()
{
	CDomain::DomainSummary pSummary;
	
	pSummary.bAuthoritative = true;

	pSummary.uiDomainID		= this->uiID;
#ifdef MPI_ON
	pSummary.uiNodeID = pManager->getMPIManager()->getNodeID();
#else
	pSummary.uiNodeID = 0;
#endif
	pSummary.uiLocalDeviceID = this->getDevice()->getDeviceID();
	pSummary.ulColCount		= this->ulCols;
	pSummary.ulRowCount		= this->ulRows;
	pSummary.ucFloatPrecision = ( this->isDoublePrecision() ? model::floatPrecision::kDouble : model::floatPrecision::kSingle );
	pSummary.dResolution	= this->dCellResolution;

	return pSummary;
}

