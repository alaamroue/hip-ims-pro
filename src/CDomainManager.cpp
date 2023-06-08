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
 *  Domain Manager Class
 * ------------------------------------------
 *
 */
#include "common.h"
#include "CDomainManager.h"
#include "CDomainBase.h"
#include "CDomain.h"
#include "CDomainCartesian.h"
#include "CScheme.h"
#include "COCLDevice.h"

/*
 *  Constructor
 */
CDomainManager::CDomainManager(void)
{
	this->cExecutorControlOpenCL = NULL;
	this->logger = NULL;
	this->ucSyncMethod = model::syncMethod::kSyncForecast;
	this->uiSyncSpareIterations = 3;
}

/*
*  Destructor
*/
CDomainManager::~CDomainManager(void)
{
	this->logger->writeLine("The domain manager is being unloaded.");
	for (unsigned int uiID = 0; uiID < domains.size(); ++uiID)
		delete domains[uiID];

}



/*
*  Is a domain local to this node?
*/
bool	CDomainManager::isDomainLocal(unsigned int uiID)
{
	return !( domains[uiID]->isRemote() );
}

/*
*  Fetch a specific domain by ID
*/
CDomainBase*	CDomainManager::getDomainBase(unsigned int uiID)
{
	return domains[uiID];
}

/*
 *  Fetch a specific domain by ID
 */
CDomain*	CDomainManager::getDomain(unsigned int uiID)
{
	return static_cast<CDomain*>(domains[ uiID ]);
}

/*
 *  Fetch a specific domain by a point therein
 */
CDomain*	CDomainManager::getDomain(double dX, double dY)
{
	return NULL;
}

/*
 *  How many domains in the set?
 */
unsigned int	CDomainManager::getDomainCount()
{
	return (unsigned int) domains.size();
}

/*
 *  Fetch the total extent of all the domains
 */
CDomainManager::Bounds		CDomainManager::getTotalExtent()
{
	CDomainManager::Bounds	b;
	// TODO: Calculate the total extent
	b.N = NULL;
	b.E = NULL;
	b.S = NULL;
	b.W = NULL;
	return b;
}


/*
*	Fetch the current sync method being employed
*/
unsigned char CDomainManager::getSyncMethod()
{
	return this->ucSyncMethod;
}

/*
*	Set the sync method to being employed
*/
void CDomainManager::setSyncMethod(unsigned char ucMethod)
{
	this->ucSyncMethod = ucMethod;
}

/*
*	Fetch the number of spare batch iterations to aim for when forecast syncing
*/
unsigned int CDomainManager::getSyncBatchSpares()
{
	return this->uiSyncSpareIterations;
}

/*
*	Set the number of spare batch iterations to aim for when forecast syncing
*/
void CDomainManager::setSyncBatchSpares(unsigned int uiSpare)
{
	this->uiSyncSpareIterations = uiSpare;
}

/*
 *  Are all the domains contiguous?
 */
bool	CDomainManager::isSetContiguous()
{
	// TODO: Calculate this
	return true;
}

/*
 *  Are all the domains ready?
 */
bool	CDomainManager::isSetReady()
{
	// Is the domain ready?

	// Is the domain's scheme ready?

	// Is the domain's device ready?

	// TODO: Check this
	return true;
}

/*
 *  Write some details to the console about our domain set
 */
void	CDomainManager::logDetails()
{
	logger->writeDivide();
	unsigned short	wColour = model::cli::colourInfoBlock;

	logger->writeLine("MODEL DOMAIN SET", true, wColour);
	logger->writeLine("  Domain count:      " + std::to_string(this->getDomainCount()), true, wColour);
	if (this->getDomainCount() <= 1)
	{
		logger->writeLine("  Synchronisation:   Not required", true, wColour);
	}
	else {
		if (this->getSyncMethod() == model::syncMethod::kSyncForecast)
		{
			logger->writeLine("  Synchronisation:   Domain-independent forecast", true, wColour);
			logger->writeLine("    Forecast method: Aiming for " + std::to_string(this->uiSyncSpareIterations) + " spare row(s)", true, wColour);
		}
		if (this->getSyncMethod() == model::syncMethod::kSyncTimestep)
			logger->writeLine("  Synchronisation:   Explicit timestep exchange", true, wColour);
	}

	logger->writeLine("", false, wColour);

	logger->writeLine("+--------+------+--------+--------+--------+-------+-------+-------+", false, wColour);
	logger->writeLine("| Domain | Node | Device |  Rows  |  Cols  | Maths | Links | Resol |", false, wColour);
	logger->writeLine("+--------+------+--------+--------+--------+-------+-------+-------+", false, wColour);

	for (unsigned int i = 0; i < this->getDomainCount(); i++)
	{
		char cDomainLine[70] = "                                                                    X";
		CDomainBase::DomainSummary pSummary = this->getDomainBase(i)->getSummary();
		std::string resolutionShort = std::to_string(pSummary.dResolution);
		resolutionShort.resize(5);

		sprintf_s(
			cDomainLine,
			70,
			"| %6s | %4s | %6s | %6s | %6s | %5s | %5s | %5s |",
			std::to_string(pSummary.uiDomainID + 1).c_str(),
			"N/A",
			std::to_string(pSummary.uiLocalDeviceID).c_str(),
			std::to_string(pSummary.ulRowCount).c_str(),
			std::to_string(pSummary.ulColCount).c_str(),
			(pSummary.ucFloatPrecision == model::floatPrecision::kSingle ? std::string("32bit") : std::string("64bit")).c_str(),
			std::to_string(0).c_str(),
			resolutionShort.c_str()
		);

		logger->writeLine(std::string(cDomainLine), false, wColour);	// 13
	}

	logger->writeLine("+--------+------+--------+--------+--------+-------+-------+-------+", false, wColour);

	logger->writeDivide();
}

void CDomainManager::logDomainMultiOrSingle()
{
	if (domains.size() <= 1) {
		logger->writeLine("This is a SINGLE-DOMAIN model, limited to 1 device.");
	}
	else {
		logger->writeLine("This is a MULTI-DOMAIN model, and possibly multi-device.");
	}
}

