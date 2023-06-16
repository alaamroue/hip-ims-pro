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

#include "CBoundaryMap.h"
#include "CBoundaryUniform.h"
#include "CDomainCartesian.h"
#include "COCLBuffer.h"
#include "COCLKernel.h"
#include "common.h"

using std::vector;

/*
 *  Constructor
 */
CBoundaryUniform::CBoundaryUniform( CDomain* pDomain )
{
	this->ucValue = model::boundaries::uniformValues::kValueLossRate;

	this->pDomain = pDomain;
}

/*
*  Destructor
*/
CBoundaryUniform::~CBoundaryUniform()
{
	delete[] this->pTimeseries;

	delete this->pBufferConfiguration;
	delete this->pBufferTimeseries;
}

/*
*	Configure this boundary and load in any related files
*/
bool CBoundaryUniform::setupFromConfig(XMLElement* pElement, std::string sBoundarySourceDir)
{
	char *cBoundaryType, *cBoundaryName, *cBoundarySource, *cBoundaryValue;

	Util::toLowercase(&cBoundaryType, pElement->Attribute("type"));
	Util::toNewString(&cBoundaryName, pElement->Attribute("name"));
	Util::toLowercase(&cBoundarySource, pElement->Attribute("source"));
	Util::toLowercase(&cBoundaryValue, pElement->Attribute("value"));

	// Must have unique name for each boundary (will get autoname by default)
	this->sName = std::string(cBoundaryName);

	// Volume increase column represents...
	if (cBoundaryValue == NULL || strcmp(cBoundaryValue, "rain-intensity") == 0) {
		this->setValue(model::boundaries::uniformValues::kValueRainIntensity);
	} else if ( strcmp(cBoundaryValue, "loss-rate") == 0) {
		this->setValue(model::boundaries::uniformValues::kValueLossRate);
	} else {
		model::doError(
			"Unrecognised value for uniform timeseries file.",
			model::errorCodes::kLevelWarning
		);
	}

	// Timeseries file...
	CCSVDataset* pCSVFile = new CCSVDataset(
		sBoundarySourceDir + std::string(cBoundarySource)
		);
	if (pCSVFile->readFile())
	{
		if (pCSVFile->isReady())
			this->importTimeseries(pCSVFile);
	}
	else {
		model::doError(
			"Could not read a uniform boundary timeseries file.",
			model::errorCodes::kLevelWarning
			);
		delete pCSVFile;
		return false;
	}
	delete pCSVFile;

	return true;
}

/*
*	Import timeseries data from a CSV file
*/
void CBoundaryUniform::importTimeseries(CCSVDataset *pCSV)
{

}

void CBoundaryUniform::prepareBoundary(
		COCLDevice* pDevice,
		COCLProgram* pProgram,
		COCLBuffer* pBufferBed,
		COCLBuffer* pBufferManning,
		COCLBuffer* pBufferTime,
		COCLBuffer* pBufferTimeHydrological,
		COCLBuffer* pBufferTimestep
	)
{
	// Configuration for the boundary and timeseries data
	if (pProgram->getFloatForm() == model::floatPrecision::kSingle)
	{
		sConfigurationSP pConfiguration;

		pConfiguration.TimeseriesEntries = this->uiTimeseriesLength;
		pConfiguration.TimeseriesInterval = this->dTimeseriesInterval;
		pConfiguration.TimeseriesLength = this->dTimeseriesLength;
		pConfiguration.Definition = (cl_uint)this->ucValue;

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
			sizeof(cl_float2)* this->uiTimeseriesLength,
			true
		);
		cl_float2 *pTimeseries = this->pBufferTimeseries->getHostBlock<cl_float2*>();
		for (unsigned int i = 0; i < this->uiTimeseriesLength; ++i)
		{
			pTimeseries[i].s[0] = this->pTimeseries[i].dTime;
			pTimeseries[i].s[1] = this->pTimeseries[i].dComponent;
		}
	} else {
		sConfigurationDP pConfiguration;

		pConfiguration.TimeseriesEntries = this->uiTimeseriesLength;
		pConfiguration.TimeseriesInterval = this->dTimeseriesInterval;
		pConfiguration.TimeseriesLength = this->dTimeseriesLength;
		pConfiguration.Definition = (cl_uint)this->ucValue;

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
			sizeof(cl_double2)* this->uiTimeseriesLength,
			true
		);
		cl_double2 *pTimeseries = this->pBufferTimeseries->getHostBlock<cl_double2*>();
		for (unsigned int i = 0; i < this->uiTimeseriesLength; ++i)
		{
			pTimeseries[i].s[0] = this->pTimeseries[i].dTime;
			pTimeseries[i].s[1] = this->pTimeseries[i].dComponent;
		}
	}

	this->pBufferConfiguration->createBuffer();
	this->pBufferConfiguration->queueWriteAll();
	this->pBufferTimeseries->createBuffer();
	this->pBufferTimeseries->queueWriteAll();

	this->oclKernel = pProgram->getKernel("bdy_Uniform");
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

	// TODO: Need a more sensible group size!
	CDomainCartesian* pDomain = static_cast<CDomainCartesian*>(this->pDomain);
	this->oclKernel->setGlobalSize(ceil(pDomain->getCols() / 8) * 8, ceil(pDomain->getRows() / 8) * 8);
	this->oclKernel->setGroupSize(8, 8);
}

void CBoundaryUniform::applyBoundary(COCLBuffer* pBufferCell)
{
	this->oclKernel->assignArgument(5, pBufferCell);
	this->oclKernel->scheduleExecution();
}

void CBoundaryUniform::streamBoundary(double dTime)
{
	// ...
}

void CBoundaryUniform::cleanBoundary()
{
	// ...
}
