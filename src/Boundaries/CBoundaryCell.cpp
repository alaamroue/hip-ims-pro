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
#include <boost/lexical_cast.hpp>

#include "CBoundaryMap.h"
#include "CBoundaryCell.h"
#include "../Domain/Cartesian/CDomainCartesian.h"
#include "../OpenCL/Executors/COCLBuffer.h"
#include "../OpenCL/Executors/COCLKernel.h"
#include "../common.h"

using std::vector;

/* 
 *  Constructor
 */
CBoundaryCell::CBoundaryCell( CDomain* pDomain )
{
	this->ucDepthValue = model::boundaries::depthValues::kValueDepth;
	this->ucDischargeValue = model::boundaries::dischargeValues::kValueTotal;

	this->pBufferConfiguration = NULL;
	this->pBufferRelations = NULL;
	this->pBufferTimeseries = NULL;

	this->pDomain = pDomain;
}

/*
 *  Destructor
 */
CBoundaryCell::~CBoundaryCell()
{
	delete[] this->pTimeseries;

	delete this->pBufferRelations;
	delete this->pBufferConfiguration;
	delete this->pBufferTimeseries;
}

/*
 *	Configure this boundary and load in any related files
 */
bool CBoundaryCell::setupFromConfig(XMLElement* pElement, std::string sBoundarySourceDir)
{
	char *cBoundaryType, *cBoundaryName, *cBoundarySource, *cBoundaryDepth, *cBoundaryDischarge, *cBoundaryMap;

	Util::toLowercase(&cBoundaryType,		pElement->Attribute("type"));
	Util::toNewString(&cBoundaryName,		pElement->Attribute("name"));
	Util::toLowercase(&cBoundarySource,		pElement->Attribute("source"));
	Util::toLowercase(&cBoundaryMap,		pElement->Attribute("mapFile"));
	Util::toLowercase(&cBoundaryDepth,		pElement->Attribute("depthValue"));
	Util::toLowercase(&cBoundaryDischarge,  pElement->Attribute("dischargeValue"));

	this->sName = std::string( cBoundaryName );

	// Discharge column represents...?
	if (cBoundaryDischarge == NULL || strcmp(cBoundaryDischarge, "total") == 0)
	{
		this->setDischargeValue(model::boundaries::dischargeValues::kValueTotal);
	} else if (strcmp(cBoundaryDischarge, "cell") == 0) {
		this->setDischargeValue(model::boundaries::dischargeValues::kValuePerCell);
	} else if (strcmp(cBoundaryDischarge, "velocity") == 0) {
		this->setDischargeValue(model::boundaries::dischargeValues::kValueVelocity);
	} else if (strcmp(cBoundaryDischarge, "ignore") == 0 || strcmp(cBoundaryDischarge, "disabled") == 0) {
		this->setDischargeValue(model::boundaries::dischargeValues::kValueIgnored);
	} else if (strcmp(cBoundaryDischarge, "volume") == 0 || strcmp(cBoundaryDischarge, "surging") == 0) {
		this->setDischargeValue(model::boundaries::dischargeValues::kValueSurging);
	} else {
		model::doError(
			"Unrecognised discharge parameter specified for timeseries file.",
			model::errorCodes::kLevelWarning
		);
	}

	// Depth column represents...?
	if (cBoundaryDepth == NULL || strcmp(cBoundaryDepth, "fsl") == 0) {
		this->setDepthValue(model::boundaries::depthValues::kValueFSL);
	} else if (strcmp(cBoundaryDepth, "depth") == 0) {
		this->setDepthValue(model::boundaries::depthValues::kValueDepth);
	} else if (strcmp(cBoundaryDepth, "ignore") == 0 || strcmp(cBoundaryDepth, "disabled") == 0) {
		this->setDepthValue(model::boundaries::depthValues::kValueIgnored);
	} else {
		model::doError(
			"Unrecognised depth parameter specified in timeseries file.",
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
	} else {
		model::doError(
			"Could not read a boundary timeseries file.",
			model::errorCodes::kLevelWarning
		);
		delete pCSVFile;
		return false;
	}
	delete pCSVFile;

	// Map file is optional -- could also have a single map file for all boundaries
	if ( cBoundaryMap == NULL )
		return true;

	// Map file...
	pCSVFile = new CCSVDataset(
		sBoundarySourceDir + std::string(cBoundaryMap)
	);
	if (pCSVFile->readFile())
	{
		if (pCSVFile->isReady())
			this->importMap(pCSVFile);
	}
	else {
		model::doError(
			"Could not read a boundary map file.",
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
void CBoundaryCell::importTimeseries(CCSVDataset *pCSV)
{

}

/*
 *	Import cell map data from a CSV file
 */
void CBoundaryCell::importMap(CCSVDataset *pCSV)
{

}

void CBoundaryCell::prepareBoundary(
			COCLDevice* pDevice, 
			COCLProgram* pProgram,
			COCLBuffer* pBufferBed, 
			COCLBuffer* pBufferManning,
			COCLBuffer* pBufferTime,
			COCLBuffer* pBufferTimeHydrological,
			COCLBuffer* pBufferTimestep
	 )
{
	// TODO: Apply scaling if discharge column is the total volume across the boundary

	// Configuration for the boundary and timeseries data
	if ( pProgram->getFloatForm() == model::floatPrecision::kSingle )
	{
		sConfigurationSP pConfiguration;
		
		pConfiguration.TimeseriesEntries  = this->uiTimeseriesLength;
		pConfiguration.TimeseriesInterval = this->dTimeseriesInterval;
		pConfiguration.TimeseriesLength   = this->dTimeseriesLength;
		pConfiguration.DefinitionDepth	  = (cl_uint)this->ucDepthValue;
		pConfiguration.DefinitionDischarge = (cl_uint)this->ucDischargeValue;
		pConfiguration.RelationCount      = this->uiRelationCount;

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
			sizeof(cl_float4)* this->uiTimeseriesLength,
			true
		);
		cl_float4 *pTimeseries = this->pBufferTimeseries->getHostBlock<cl_float4*>();
		for (unsigned int i = 0; i < this->uiTimeseriesLength; ++i)
		{
			pTimeseries[i].s[0] = this->pTimeseries[i].dTime;
			pTimeseries[i].s[1] = this->pTimeseries[i].dDepthComponent;
			pTimeseries[i].s[2] = this->pTimeseries[i].dDischargeComponentX;
			pTimeseries[i].s[3] = this->pTimeseries[i].dDischargeComponentY;

			if (this->ucDischargeValue == model::boundaries::dischargeValues::kValueTotal)
			{
				pTimeseries[i].s[2] /= this->uiRelationCount;
				pTimeseries[i].s[3] /= this->uiRelationCount;
			}
		}
	} else {
		sConfigurationDP pConfiguration;

		pConfiguration.TimeseriesEntries  = this->uiTimeseriesLength;
		pConfiguration.TimeseriesInterval = this->dTimeseriesInterval;
		pConfiguration.TimeseriesLength   = this->dTimeseriesLength;
		pConfiguration.DefinitionDepth = (cl_uint)this->ucDepthValue;
		pConfiguration.DefinitionDischarge = (cl_uint)this->ucDischargeValue;
		pConfiguration.RelationCount	  = this->uiRelationCount;

		this->pBufferConfiguration = new COCLBuffer(
			"Bdy_" + this->sName + "_Conf",
			pProgram,
			true,
			true,
			sizeof( sConfigurationDP ),
			true
		);
		std::memcpy(
			this->pBufferConfiguration->getHostBlock<void*>(),
			&pConfiguration,
			sizeof( sConfigurationDP )
		);

		this->pBufferTimeseries = new COCLBuffer(
			"Bdy_" + this->sName + "_Series",
			pProgram,
			true,
			true,
			sizeof(cl_double4)* this->uiTimeseriesLength,
			true
		);
		cl_double4 *pTimeseries = this->pBufferTimeseries->getHostBlock<cl_double4*>();
		for (unsigned int i = 0; i < this->uiTimeseriesLength; ++i)
		{
			pTimeseries[i].s[0] = this->pTimeseries[i].dTime;
			pTimeseries[i].s[1] = this->pTimeseries[i].dDepthComponent;
			pTimeseries[i].s[2] = this->pTimeseries[i].dDischargeComponentX;
			pTimeseries[i].s[3] = this->pTimeseries[i].dDischargeComponentY;

			if (this->ucDischargeValue == model::boundaries::dischargeValues::kValueTotal)
			{
				pTimeseries[i].s[2] /= this->uiRelationCount;
				pTimeseries[i].s[3] /= this->uiRelationCount;
			}
		}
	}

	this->pBufferConfiguration->createBuffer();
	this->pBufferConfiguration->queueWriteAll();
	this->pBufferTimeseries->createBuffer();
	this->pBufferTimeseries->queueWriteAll();

	// Cell relation data for the boundary
	this->pBufferRelations = new COCLBuffer(
		"Bdy_" + this->sName + "_Rels",
		pProgram,
		true,
		true,
		sizeof( cl_ulong ) * this->uiRelationCount,
		true
	);
	// This is a bit of a mess... but it works...
	CDomainCartesian* pDomainCart = static_cast<CDomainCartesian*>(this->pDomain);
	cl_ulong* pCells = this->pBufferRelations->getHostBlock<cl_ulong*>();
	for (unsigned int i = 0; i < this->uiRelationCount; ++i)
		pCells[i] = pDomainCart->getCellID( this->pRelations[i].uiCellX, this->pRelations[i].uiCellY );
	this->pBufferRelations->createBuffer();
	this->pBufferRelations->queueWriteAll();

	this->oclKernel = pProgram->getKernel("bdy_Cell");
	COCLBuffer* aryArgsBdy[] = { 
		pBufferConfiguration, 
		pBufferRelations, 
		pBufferTimeseries, 
		pBufferTime, 
		pBufferTimestep, 
		pBufferTimeHydrological, 
		NULL,	// Cell states (added later)
		pBufferBed, 
		pBufferManning
	};

	this->oclKernel->assignArguments(aryArgsBdy);
	this->oclKernel->setGroupSize(8);
	this->oclKernel->setGlobalSize( ( this->uiRelationCount / 8 + 1 ) * 8 );
}

// TODO: Only the cell buffer should be passed here...
void CBoundaryCell::applyBoundary(COCLBuffer* pBufferCell)
{
	this->oclKernel->assignArgument( 6, pBufferCell );
	this->oclKernel->scheduleExecution();
}

void CBoundaryCell::streamBoundary(double dTime)
{
	// ...
	// TODO: Should we handle all the memory buffer writing in here?...
}

void CBoundaryCell::cleanBoundary()
{
	// ...
	// TODO: Is this needed? Most stuff is cleaned in the destructor
}

