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
	for (unsigned int uiID = 0; uiID < domains.size(); ++uiID)
		delete domains[uiID];

	model::log->writeLine("The domain manager has been unloaded.");
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

	model::log->writeLine( "A new domain has been created within the model." );

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
std::vector<CDomainBase*>* CDomainManager::getDomainBaseVector()
{
	return &domains;
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
	model::log->writeLine("Generating link data for each domain");

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
				CDomainLink* pNewLink = new CDomainLink(domains[i], domains[j]);
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
	model::log->writeDivide();
	unsigned short	wColour = model::cli::colourInfoBlock;

	model::log->writeLine("MODEL DOMAIN SET", true, wColour);
	model::log->writeLine("  Domain count:      " + toStringExact(this->getDomainCount()), true, wColour);
	if (this->getDomainCount() <= 1)
	{
		model::log->writeLine("  Synchronisation:   Not required", true, wColour);
	}
	else {
		if (this->getSyncMethod() == model::syncMethod::kSyncForecast)
		{
			model::log->writeLine("  Synchronisation:   Domain-independent forecast", true, wColour);
			model::log->writeLine("    Forecast method: Aiming for " + toStringExact(this->uiSyncSpareIterations) + " spare row(s)", true, wColour);
		}
		if (this->getSyncMethod() == model::syncMethod::kSyncTimestep)
			model::log->writeLine("  Synchronisation:   Explicit timestep exchange", true, wColour);
	}

	model::log->writeLine("", false, wColour);

	model::log->writeLine("+--------+------+--------+--------+--------+-------+-------+-------+", false, wColour);	
	model::log->writeLine("| Domain | Node | Device |  Rows  |  Cols  | Maths | Links | Resol |", false, wColour);	
	model::log->writeLine("+--------+------+--------+--------+--------+-------+-------+-------+", false, wColour);

	for (unsigned int i = 0; i < this->getDomainCount(); i++)
	{
		char cDomainLine[70] = "                                                                    X";
		CDomainBase::DomainSummary pSummary = this->getDomainBase(i)->getSummary();
		std::string resolutionShort = toStringExact(pSummary.dResolutionX);
		resolutionShort.resize(5);

		sprintf(
			cDomainLine,
			"| %6s | %4s | %6s | %6s | %6s | %5s | %5s | %5s |",
			toStringExact(pSummary.uiDomainID + 1).c_str(),
			"N/A",
			toStringExact(pSummary.uiLocalDeviceID).c_str(),
			toStringExact(pSummary.ulRowCount).c_str(),
			toStringExact(pSummary.ulColCount).c_str(),
			(pSummary.ucFloatPrecision == model::floatPrecision::kSingle ? std::string("32bit") : std::string("64bit")).c_str(),
			toStringExact(this->getDomainBase(i)->getLinkCount()).c_str(),
			resolutionShort.c_str()
		);

		model::log->writeLine(std::string(cDomainLine), false, wColour);	// 13
	}

	model::log->writeLine("+--------+------+--------+--------+--------+-------+-------+-------+", false, wColour);

	model::log->writeDivide();
}


