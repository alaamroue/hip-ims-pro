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

	//CDomain Stuff
	this->pScheme = NULL;
	this->ucFloatSize = 0;
	this->dMinFSL = 9999.0;
	this->dMaxFSL = -9999.0;
	this->dMinTopo = 9999.0;
	this->dMaxTopo = -9999.0;
	this->dMinDepth = 9999.0;
	this->dMaxDepth = -9999.0;

	pDataProgress.dBatchTimesteps = 0;
	pDataProgress.dCurrentTime = 0.0;
	pDataProgress.dCurrentTimestep = 0.0;
	pDataProgress.uiBatchSize = 0;
	pDataProgress.uiBatchSkipped = 0;
	pDataProgress.uiBatchSuccessful = 0;
}

/*
 *  Destructor
 */
CDomainCartesian::~CDomainCartesian(void)
{
	if (this->ucFloatSize == 4)
	{
		delete[] this->fCellStates;
		delete[] this->fBedElevations;
		delete[] this->fManningValues;
	}
	else if (this->ucFloatSize == 8) {
		delete[] this->dCellStates;
		delete[] this->dBedElevations;
		delete[] this->dManningValues;
	}

	if (this->pScheme != NULL)     delete pScheme;

	delete[] this->cSourceDir;
	delete[] this->cTargetDir;

	logger->writeLine("All domain memory has been released.");
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
CDomainCartesian::DomainSummary CDomainCartesian::getSummary()
{
	CDomainCartesian::DomainSummary pSummary;

	pSummary.bAuthoritative = true;

	pSummary.uiDomainID = this->uiID;
	pSummary.uiNodeID = 0;
	pSummary.uiLocalDeviceID = this->getDevice()->getDeviceID();
	pSummary.ulColCount = this->ulCols;
	pSummary.ulRowCount = this->ulRows;
	pSummary.ucFloatPrecision = (this->isDoublePrecision() ? model::floatPrecision::kDouble : model::floatPrecision::kSingle);
	pSummary.dResolution = this->dCellResolution;

	return pSummary;
}


void	CDomainCartesian::createStoreBuffers(
	void** vArrayCellStates,
	void** vArrayBedElevations,
	void** vArrayManningCoefs,
	void** vArrayFlowStates,
	void** vArrayBoundCoup,
	void** vArrayDsDt,
	unsigned char	ucFloatSize
)
{

	this->ucFloatSize = ucFloatSize;

	try {
		if (ucFloatSize == sizeof(cl_float))
		{
			// Single precision
			this->fCellStates = new cl_float4[this->ulCellCount];
			this->fBedElevations = new cl_float[this->ulCellCount];
			this->fManningValues = new cl_float[this->ulCellCount];
			this->fBoundCoup = new cl_float2[this->ulCellCount];
			this->fdsdt = new cl_float[this->ulCellCount];

			// Double precision
			this->dCellStates = (cl_double4*)(this->fCellStates);
			this->dBedElevations = (cl_double*)(this->fBedElevations);
			this->dManningValues = (cl_double*)(this->fManningValues);
			this->dBoundCoup = (cl_double2*)(this->fBoundCoup);
			this->ddsdt = (cl_double*)(this->fdsdt);

			// Both
			this->cFlowStates = new model::FlowStates[this->ulCellCount];

			*vArrayCellStates = static_cast<void*>(this->fCellStates);
			*vArrayBedElevations = static_cast<void*>(this->fBedElevations);
			*vArrayManningCoefs = static_cast<void*>(this->fManningValues);
			*vArrayFlowStates = static_cast<void*>(this->cFlowStates);
			*vArrayBoundCoup = static_cast<void*>(this->fBoundCoup);
			*vArrayDsDt = static_cast<void*>(this->fdsdt);
		}
		else {
			// Double precision
			this->dCellStates = new cl_double4[this->ulCellCount];
			this->dBedElevations = new cl_double[this->ulCellCount];
			this->dManningValues = new cl_double[this->ulCellCount];
			this->dBoundCoup = new cl_double2[this->ulCellCount];
			this->ddsdt = new cl_double[this->ulCellCount];


			this->fCellStates = (cl_float4*)(this->dCellStates);
			this->fBedElevations = (cl_float*)(this->dBedElevations);
			this->fManningValues = (cl_float*)(this->dManningValues);
			this->fBoundCoup = (cl_float2*)(this->dBoundCoup);
			this->fdsdt = (cl_float*)(this->ddsdt);

			this->cFlowStates = new model::FlowStates[this->ulCellCount];

			*vArrayCellStates = static_cast<void*>(this->dCellStates);
			*vArrayBedElevations = static_cast<void*>(this->dBedElevations);
			*vArrayManningCoefs = static_cast<void*>(this->dManningValues);
			*vArrayFlowStates = static_cast<void*>(this->cFlowStates);
			*vArrayBoundCoup = static_cast<void*>(this->dBoundCoup);
			*vArrayDsDt = static_cast<void*>(this->ddsdt);
		}
	}
	catch (std::bad_alloc)
	{
		model::doError(
			"Domain memory allocation failure. Probably out of memory.",
			model::errorCodes::kLevelFatal
		);
		return;
	}
}

/*
 *  Sets the bed elevation for a given cell
 */
void	CDomainCartesian::setBedElevation(unsigned long ulCellID, double dElevation)
{
	if (this->ucFloatSize == 4)
	{
		this->fBedElevations[ulCellID] = static_cast<float>(dElevation);
	}
	else {
		this->dBedElevations[ulCellID] = dElevation;
	}
}

/*
 *  Sets the Manning coefficient for a given cell
 */
void	CDomainCartesian::setManningCoefficient(unsigned long ulCellID, double dCoefficient)
{
	if (this->ucFloatSize == 4)
	{
		this->fManningValues[ulCellID] = static_cast<float>(dCoefficient);
	}
	else {
		this->dManningValues[ulCellID] = dCoefficient;
	}
}

/*
 *  Sets the Flow States Values (noFlow, noFlowX, noFlowY, poliniX, poliniY)
 */
void	CDomainCartesian::setFlowStatesValue(unsigned long ulCellID, model::FlowStates state)
{
	this->cFlowStates[ulCellID] = state;
}


/*
 *  Sets the Manning coefficient for a given cell
 */
void	CDomainCartesian::setBoundaryCondition(unsigned long ulCellID, double value)
{
	if (this->ucFloatSize == 4)
	{
		this->fBoundCoup[ulCellID].s[0] = static_cast<float>(value);
	}
	else {
		this->dBoundCoup[ulCellID].s[0] = value;
	}
}

/*
 *  Sets the Manning coefficient for a given cell
 */
void	CDomainCartesian::resetBoundaryCondition()
{
	if (this->ucFloatSize == 4)
	{
		memset(fBoundCoup, 0, sizeof(cl_float2) * this->ulCellCount);
	}
	else {
		memset(dBoundCoup, 0, sizeof(cl_double2) * this->ulCellCount);
	}
}

/*
 *  Sets the Manning coefficient for a given cell
 */
void	CDomainCartesian::setCouplingCondition(unsigned long ulCellID, double value)
{
	if (this->ucFloatSize == 4)
	{
		this->fBoundCoup[ulCellID].s[1] = static_cast<float>(value);
	}
	else {
		this->dBoundCoup[ulCellID].s[1] = value;
	}
}

/*
 *  Sets the dsdt for a given cell
 */
void	CDomainCartesian::setdsdt(unsigned long ulCellID, double value)
{
	if (this->ucFloatSize == 4)
	{
		this->fdsdt[ulCellID] = static_cast<float>(value);
	}
	else {
		this->ddsdt[ulCellID] = value;
	}
}
/*
 *  Sets a state variable for a given cell
 */
void	CDomainCartesian::setStateValue(unsigned long ulCellID, unsigned char ucIndex, double dValue)
{
	if (this->ucFloatSize == 4)
	{
		this->fCellStates[ulCellID].s[ucIndex] = static_cast<float>(dValue);
	}
	else {
		this->dCellStates[ulCellID].s[ucIndex] = dValue;
	}
}

/*
 *  Gets the bed elevation for a given cell
 */
double	CDomainCartesian::getBedElevation(unsigned long ulCellID)
{
	if (this->ucFloatSize == 4)
		return static_cast<double>(this->fBedElevations[ulCellID]);
	return this->dBedElevations[ulCellID];
}

/*
 *  Gets the Manning coefficient for a given cell
 */
double	CDomainCartesian::getManningCoefficient(unsigned long ulCellID)
{
	if (this->ucFloatSize == 4)
		return static_cast<double>(this->fManningValues[ulCellID]);
	return this->dManningValues[ulCellID];
}

/*
 *  Gets a state variable for a given cell
 */
double	CDomainCartesian::getStateValue(unsigned long ulCellID, unsigned char ucIndex)
{
	if (this->ucFloatSize == 4)
		return static_cast<double>(this->fCellStates[ulCellID].s[ucIndex]);
	return this->dCellStates[ulCellID].s[ucIndex];
}

/*
 *  Gets a state variable for a given cell
 */
double	CDomainCartesian::getdsdt(unsigned long ulCellID)
{
	if (this->ucFloatSize == 4)
		return static_cast<double>(this->fdsdt[ulCellID]);
	return this->ddsdt[ulCellID];
}

/*
 *  Handle initial conditions input data for a cell (usually from a raster dataset)
 */
void	CDomainCartesian::handleInputData(
	unsigned long	ulCellID,
	double			dValue,
	unsigned char	ucValue,
	unsigned char	ucRounding
)
{

	switch (ucValue)
	{
	case model::rasterDatasets::dataValues::kBedElevation:
		this->setBedElevation(
			ulCellID,
			Util::round(dValue, ucRounding)
		);
		this->setStateValue(
			ulCellID,
			model::domainValueIndices::kValueFreeSurfaceLevel,
			Util::round(dValue, ucRounding)
		);
		if (dValue < dMinTopo && dValue != -9999.0) dMinTopo = dValue;
		if (dValue > dMaxTopo && dValue != -9999.0) dMaxTopo = dValue;
		break;
	case model::rasterDatasets::dataValues::kFreeSurfaceLevel:
		this->setStateValue(
			ulCellID,
			model::domainValueIndices::kValueFreeSurfaceLevel,
			Util::round(dValue, ucRounding)
		);
		this->setStateValue(
			ulCellID,
			model::domainValueIndices::kValueMaxFreeSurfaceLevel,
			Util::round(dValue, ucRounding)
		);
		if (dValue - this->getBedElevation(ulCellID) < dMinDepth && this->getBedElevation(ulCellID) > -9999.0 && dValue > -9999.0) dMinDepth = dValue;
		if (dValue - this->getBedElevation(ulCellID) > dMaxDepth && this->getBedElevation(ulCellID) > -9999.0 && dValue > -9999.0) dMaxDepth = dValue;
		if (dValue < dMinFSL && this->getBedElevation(ulCellID) > -9999.0 && dValue > -9999.0) dMinFSL = dValue;
		if (dValue > dMaxFSL && this->getBedElevation(ulCellID) > -9999.0 && dValue > -9999.0) dMaxFSL = dValue;
		break;
	case model::rasterDatasets::dataValues::kDepth:
		this->setStateValue(
			ulCellID,
			model::domainValueIndices::kValueFreeSurfaceLevel,
			Util::round((this->getBedElevation(ulCellID) + dValue), ucRounding)
		);
		this->setStateValue(
			ulCellID,
			model::domainValueIndices::kValueMaxFreeSurfaceLevel,
			Util::round((this->getBedElevation(ulCellID) + dValue), ucRounding)
		);
		if (dValue + this->getBedElevation(ulCellID) < dMinFSL && this->getBedElevation(ulCellID) > -9999.0 && dValue > -9999.0) dMinFSL = dValue;
		if (dValue + this->getBedElevation(ulCellID) > dMaxFSL && this->getBedElevation(ulCellID) > -9999.0 && dValue > -9999.0) dMaxFSL = dValue;
		if (dValue < dMinDepth && this->getBedElevation(ulCellID) > -9999.0 && dValue > -9999.0) dMinDepth = dValue;
		if (dValue > dMaxDepth && this->getBedElevation(ulCellID) > -9999.0 && dValue > -9999.0) dMaxDepth = dValue;
		break;
	case model::rasterDatasets::dataValues::kDisabledCells:
		// Cells are disabled using a free-surface level of -9999
		if (dValue > 1.0 && dValue < 9999.0)
		{
			this->setStateValue(
				ulCellID,
				model::domainValueIndices::kValueMaxFreeSurfaceLevel,
				Util::round((-9999.0), ucRounding)
			);
		}
		break;
	case model::rasterDatasets::dataValues::kDischargeX:
		this->setStateValue(
			ulCellID,
			model::domainValueIndices::kValueDischargeX,
			Util::round(dValue, ucRounding)
		);
		break;
	case model::rasterDatasets::dataValues::kDischargeY:
		this->setStateValue(
			ulCellID,
			model::domainValueIndices::kValueDischargeY,
			Util::round(dValue, ucRounding)
		);
		break;
	case model::rasterDatasets::dataValues::kVelocityX:
		this->setStateValue(
			ulCellID,
			model::domainValueIndices::kValueDischargeX,
			Util::round(dValue * (this->getStateValue(ulCellID, model::domainValueIndices::kValueFreeSurfaceLevel) - this->getBedElevation(ulCellID)), ucRounding)
		);
		break;
	case model::rasterDatasets::dataValues::kVelocityY:
		this->setStateValue(
			ulCellID,
			model::domainValueIndices::kValueDischargeY,
			Util::round(dValue * (this->getStateValue(ulCellID, model::domainValueIndices::kValueFreeSurfaceLevel) - this->getBedElevation(ulCellID)), ucRounding)
		);
		break;
	case model::rasterDatasets::dataValues::kManningCoefficient:
		this->setManningCoefficient(
			ulCellID,
			Util::round(dValue, ucRounding)
		);
		break;
	}
}

/*
 *  Sets the scheme we're running on this domain
 */
void	CDomainCartesian::setScheme(CScheme* pScheme)
{
	this->pScheme = pScheme;
}

/*
 *  Gets the scheme we're running on this domain
 */
CScheme* CDomainCartesian::getScheme()
{
	return pScheme;
}

/*
 *  Sets the device to use
 */
void	CDomainCartesian::setDevice(COCLDevice* pDevice)
{
	this->pDevice = pDevice;
}

/*
 *  Gets the scheme we're running on this domain
 */
COCLDevice* CDomainCartesian::getDevice()
{
	return this->pDevice;
}

/*
 *  Gets the scheme we're running on this domain
 */
CDomainCartesian::mpiSignalDataProgress	CDomainCartesian::getDataProgress()
{
	CDomainCartesian::mpiSignalDataProgress pResponse;
	CScheme* pScheme = getScheme();

	pResponse.uiDomainID = this->uiID;
	pResponse.dBatchTimesteps = pScheme->getAverageTimestep();
	pResponse.dCurrentTime = pScheme->getCurrentTime();
	pResponse.dCurrentTimestep = pScheme->getCurrentTimestep();
	pResponse.uiBatchSize = pScheme->getBatchSize();
	pResponse.uiBatchSkipped = pScheme->getIterationsSkipped();
	pResponse.uiBatchSuccessful = pScheme->getIterationsSuccessful();

	return pResponse;
}

void CDomainCartesian::setLogger(CLog* log) {
	this->logger = log;
}


/*
 *  Is this domain ready to be used for a model run?
 */
bool	CDomainCartesian::isInitialised()
{
	return true;
}

/*
 *  Return the total number of cells in the domain
 */
unsigned long	CDomainCartesian::getCellCount()
	{
		return this->ulCellCount;
	}
