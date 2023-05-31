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
#include "CDomainLink.h"
#include "CScheme.h"
#include "CRasterDataset.h"
#include "CBoundaryMap.h"
#include "COCLDevice.h"

/*
 *  Constructor
 */
CDomainManager::CDomainManager(void)
{
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
 *  Set up the domain manager using the configuration file
 */
bool CDomainManager::setupFromConfig()
{
	/*
	//this->setSyncMethod(model::syncMethod::kSyncTimestep);
	//this->setSyncMethod(model::syncMethod::kSyncForecast);
	//this->setSyncBatchSpares(boost::lexical_cast<unsigned int>(??));

	CDomainBase* pDomainNew = CDomainBase::createDomain(model::domainStructureTypes::kStructureCartesian);
	logger->writeLine("ocal device IDs are relative to # 1." );
	logger->writeLine("Assigning domain to device #1 ."  );
	static_cast<CDomain*>(pDomainNew)->setDevice(pManager->getExecutor()->getDevice(1));

	//Domain Data
	if (!pDomainNew->configureDomain()){
		return false;
	}

	pDomainNew->setID( getDomainCount() );	// Should not be needed, but somehow is?
	domains.push_back( pDomainNew );

	this->logDomainMultiOrSingle();
	this->generateLinks();
	this->logDetails();
	if (!this->checkDomainLinks()) return false;
	*/
	return true;
}

/*
 *  Add a new domain to the set
 */
CDomainBase*	CDomainManager::createNewDomain( unsigned char ucType )
{
	CDomainBase*	pNewDomain = CDomainBase::createDomain(ucType);
	unsigned int	uiID		= getDomainCount() + 1;

	domains.push_back( pNewDomain );
	pNewDomain->setID( uiID );

	logger->writeLine( "A new domain has been created within the model." );

	return pNewDomain;
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
	return domains.size();
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
 *  Write all of the domain data to disk
 */
void	CDomainManager::writeOutputs()
{
	for( unsigned int i = 0; i < domains.size(); i++ )
	{
		if (!domains[i]->isRemote())
		{
			getDomain(i)->writeOutputs();
		}
	}
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
 *	Generate links between domains where possible
 */
void	CDomainManager::generateLinks()
{
	logger->writeLine("Generating link data for each domain");

	for (unsigned int i = 0; i < domains.size(); i++)
	{
		// Remove any pre-existing links
		domains[i]->clearLinks();
	}

	for (unsigned int i = 0; i < domains.size(); i++)
	{
		for (unsigned int j = 0; j < domains.size(); j++)
		{
			// Must overlap and meet our various constraints
			if (i != j && CDomainLink::canLink(domains[i], domains[j]))
			{
				// Make a new link...
				CDomainLink* pNewLink = new CDomainLink(domains[i], domains[j], logger);
				domains[i]->addLink(pNewLink);
				domains[j]->addDependentLink(pNewLink);
			}
		}
	}
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

		sprintf(
			cDomainLine,
			"| %6s | %4s | %6s | %6s | %6s | %5s | %5s | %5s |",
			std::to_string(pSummary.uiDomainID + 1).c_str(),
			#ifdef MPI_ON
			std::to_string(pSummary.uiNodeID).c_str(),
			#else
			"N/A",
			#endif
			std::to_string(pSummary.uiLocalDeviceID).c_str(),
			std::to_string(pSummary.ulRowCount).c_str(),
			std::to_string(pSummary.ulColCount).c_str(),
			(pSummary.ucFloatPrecision == model::floatPrecision::kSingle ? std::string("32bit") : std::string("64bit")).c_str(),
			std::to_string(this->getDomainBase(i)->getLinkCount()).c_str(),
			resolutionShort.c_str()
		);

		logger->writeLine(std::string(cDomainLine), false, wColour);	// 13
	}

	logger->writeLine("+--------+------+--------+--------+--------+-------+-------+-------+", false, wColour);

	logger->writeDivide();
}

bool CDomainManager::checkDomainLinks()
{
	// If we have more than one domain then we also need at least one link per domain
	// otherwise something is wrong...
	if (domains.size() > 1) {
		for (unsigned int i = 0; i < domains.size(); i++) {
			if (domains[i]->getLinkCount() < 1) {
				model::doError(
					"One or more domains are not linked.",
					model::errorCodes::kLevelModelStop
				);
				return false;
			}
		}
	}
	return true;
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
