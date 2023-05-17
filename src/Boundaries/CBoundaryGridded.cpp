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
 *  Domain boundary handling class
 * ------------------------------------------
 *
 */
#include <vector>
#include <algorithm>
#include <boost/lexical_cast.hpp>

#include "CBoundaryMap.h"
#include "CBoundaryGridded.h"
#include "../Datasets/CXMLDataset.h"
#include "../Datasets/CRasterDataset.h"
#include "../Domain/Cartesian/CDomainCartesian.h"
#include "../OpenCL/Executors/COCLBuffer.h"
#include "../OpenCL/Executors/COCLKernel.h"
#include "../common.h"

using std::vector;
using std::min;
using std::max;

/*
*  Constructor
*/
CBoundaryGridded::CBoundaryGridded(CDomain* pDomain)
{
	this->ucValue = model::boundaries::griddedValues::kValueRainIntensity;

	this->pTimeseries = NULL;
	this->pTransform = NULL;
	this->pBufferConfiguration = NULL;
	this->pBufferTimeseries = NULL;
	this->uiTimeseriesLength = 0;

	this->pDomain = pDomain;
}

/*
*  Destructor
*/
CBoundaryGridded::~CBoundaryGridded()
{
	for (unsigned int i = 0; i < this->uiTimeseriesLength; ++i)
		delete this->pTimeseries[i];
	delete[] this->pTimeseries;
	delete this->pTransform;
	delete this->pBufferConfiguration;
	delete this->pBufferTimeseries;
}

/*
*	Configure this boundary and load in any related files
*/
bool CBoundaryGridded::setupFromConfig(XMLElement* pElement, std::string sBoundarySourceDir)
{
	return true;
}

void CBoundaryGridded::prepareBoundary(
	COCLDevice* pDevice,
	COCLProgram* pProgram,
	COCLBuffer* pBufferBed,
	COCLBuffer* pBufferManning,
	COCLBuffer* pBufferTime,
	COCLBuffer* pBufferTimeHydrological,
	COCLBuffer* pBufferTimestep
	)
{
	if ( this->pTransform == NULL )
		return;

	// Configuration for the boundary and timeseries data
	if (pProgram->getFloatForm() == model::floatPrecision::kSingle)
	{
		sConfigurationSP pConfiguration;

		pConfiguration.TimeseriesEntries = this->uiTimeseriesLength;
		pConfiguration.TimeseriesInterval = this->dTimeseriesInterval;
		pConfiguration.Definition = (cl_uint)this->ucValue;
		pConfiguration.GridRows = this->pTransform->uiRows;
		pConfiguration.GridCols = this->pTransform->uiColumns;
		pConfiguration.GridResolution = this->pTransform->dTargetResolution;
		pConfiguration.GridOffsetX = this->pTransform->dOffsetWest;
		pConfiguration.GridOffsetY = this->pTransform->dOffsetSouth;

		this->pBufferConfiguration = new COCLBuffer(
			"Bdy_" + this->sName + "_Conf",
			pProgram,
			true,
			true,
			sizeof(sConfigurationSP),
			true
		);
		std::memcpy(
			this->pBufferConfiguration->getHostBlock<void*>(),
			&pConfiguration,
			sizeof(sConfigurationSP)
		);

		this->pBufferTimeseries = new COCLBuffer(
			"Bdy_" + this->sName + "_Series",
			pProgram,
			true,
			true,
			sizeof(cl_float)* this->pTransform->uiColumns * this->pTransform->uiRows * this->uiTimeseriesLength,
			true
		);

		unsigned long ulSize;
		unsigned long ulOffset = 0;
		for (unsigned int i = 0; i < this->uiTimeseriesLength; ++i)
		{
			void* pGridData = this->pTimeseries[i]->getBufferData( model::floatPrecision::kSingle, this->pTransform );
			ulSize = sizeof( cl_float )* this->pTransform->uiColumns * this->pTransform->uiRows;
			ulOffset += ulSize;
			std::memcpy(
				&( ( this->pBufferTimeseries->getHostBlock<cl_uchar*>() )[ulOffset] ),
				pGridData,
				ulSize
			);
			delete[] pGridData;
		}
	}
	else {
		sConfigurationDP pConfiguration;

		pConfiguration.TimeseriesEntries = this->uiTimeseriesLength;
		pConfiguration.TimeseriesInterval = this->dTimeseriesInterval;
		pConfiguration.Definition = (cl_uint)this->ucValue;
		pConfiguration.GridRows = this->pTransform->uiRows;
		pConfiguration.GridCols = this->pTransform->uiColumns;
		pConfiguration.GridResolution = this->pTransform->dSourceResolution;
		pConfiguration.GridOffsetX = this->pTransform->dOffsetWest;
		pConfiguration.GridOffsetY = this->pTransform->dOffsetSouth;

		this->pBufferConfiguration = new COCLBuffer(
			"Bdy_" + this->sName + "_Conf",
			pProgram,
			true,
			true,
			sizeof(sConfigurationDP),
			true
		);
		std::memcpy(
			this->pBufferConfiguration->getHostBlock<void*>(),
			&pConfiguration,
			sizeof(sConfigurationDP)
		);

		this->pBufferTimeseries = new COCLBuffer(
			"Bdy_" + this->sName + "_Series",
			pProgram,
			true,
			true,
			sizeof( cl_double ) * this->pTransform->uiColumns * this->pTransform->uiRows * this->uiTimeseriesLength,
			true
		);

		unsigned long ulSize;
		unsigned long ulOffset = 0;
		for (unsigned int i = 0; i < this->uiTimeseriesLength; ++i)
		{
			void* pGridData = this->pTimeseries[i]->getBufferData( model::floatPrecision::kDouble, this->pTransform );
			ulSize = sizeof( cl_double ) * this->pTransform->uiColumns * this->pTransform->uiRows;
			std::memcpy(
				&((this->pBufferTimeseries->getHostBlock<cl_uchar*>())[ulOffset]),
				pGridData,
				ulSize
			);
			ulOffset += ulSize;
			delete[] pGridData;
		}
	}

	this->pBufferConfiguration->createBuffer();
	this->pBufferConfiguration->queueWriteAll();
	this->pBufferTimeseries->createBuffer();
	this->pBufferTimeseries->queueWriteAll();

	// Prepare kernel and arguments
	this->oclKernel = pProgram->getKernel("bdy_Gridded");
	COCLBuffer* aryArgsBdy[] = {
		pBufferConfiguration,
		pBufferTimeseries,
		pBufferTime,
		pBufferTimestep,
		pBufferTimeHydrological,
		NULL,	// Cell states
		pBufferBed,
		pBufferManning
	};
	this->oclKernel->assignArguments(aryArgsBdy);

	// Dimension the kernel
	// TODO: Need a more sensible group size!
	CDomainCartesian* pDomain = static_cast<CDomainCartesian*>( this->pDomain );
	this->oclKernel->setGlobalSize( ceil( pDomain->getCols() / 8 ) * 8, ceil( pDomain->getRows() / 8 ) * 8 );
	this->oclKernel->setGroupSize( 8, 8 );
}

// TODO: Only the cell buffer should be passed here...
void CBoundaryGridded::applyBoundary(COCLBuffer* pBufferCell)
{
	this->oclKernel->assignArgument(5, pBufferCell);
	this->oclKernel->scheduleExecution();
}

void CBoundaryGridded::streamBoundary(double dTime)
{
	// ...
	// TODO: Should we handle all the memory buffer writing in here?...
}

void CBoundaryGridded::cleanBoundary()
{
	// ...
	// TODO: Is this needed? Most stuff is cleaned in the destructor
}

/*
 *	Timeseries grid data has its own management class
 */
CBoundaryGridded::CBoundaryGriddedEntry::CBoundaryGriddedEntry( double dTime, double* dValues )
{
	this->dTime = dTime;
	this->dValues = dValues;
}

CBoundaryGridded::CBoundaryGriddedEntry::~CBoundaryGriddedEntry()
{
	delete[] this->dValues;
}

void* CBoundaryGridded::CBoundaryGriddedEntry::getBufferData(unsigned char ucFloatMode, CBoundaryGridded::SBoundaryGridTransform *pTransform)
{
	void* pReturn;

	if (ucFloatMode == model::floatPrecision::kSingle)
	{
		cl_float* pFloat = new cl_float[pTransform->uiColumns * pTransform->uiRows];
		for (unsigned long i = 0; i < pTransform->uiColumns * pTransform->uiRows; i++)
			pFloat[i] = this->dValues[i];
		pReturn = static_cast<void*>(pFloat);
	} else {
		pReturn = static_cast<void*>( this->dValues );
	}

	return pReturn;
}

