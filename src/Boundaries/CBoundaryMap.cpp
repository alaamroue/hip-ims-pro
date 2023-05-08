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

#include "../common.h"
#include "CBoundaryMap.h"
#include "CBoundary.h"
#include "CBoundaryCell.h"
#include "CBoundaryUniform.h"
#include "CBoundaryGridded.h"
#include "../Domain/CDomainManager.h"
#include "../Domain/CDomain.h"
#include "../Domain/Cartesian/CDomainCartesian.h"

using std::vector;

/* 
 *  Constructor
 */
CBoundaryMap::CBoundaryMap( CDomain* pDomain )
{
	this->pDomain = pDomain;
}

/*
 *  Destructor
 */
CBoundaryMap::~CBoundaryMap()
{
	for (mapBoundaries_t::iterator it = mapBoundaries.begin(); it != mapBoundaries.end(); it++)
	{
		(it->second)->cleanBoundary();
		delete it->second;
		mapBoundaries.erase(it);
	}
}

/*
 *	Prepare the boundary resources required (buffers, kernels, etc.)
 */
void CBoundaryMap::prepareBoundaries(
		COCLProgram* pProgram,
		COCLBuffer* pBufferBed,
		COCLBuffer* pBufferManning,
		COCLBuffer* pBufferTime,
		COCLBuffer* pBufferTimeHydrological,
		COCLBuffer* pBufferTimestep
	)
{
	for (mapBoundaries_t::iterator it = mapBoundaries.begin(); it != mapBoundaries.end(); it++)
		(it->second)->prepareBoundary( pProgram->getDevice(), pProgram, pBufferBed, pBufferManning, pBufferTime, pBufferTimeHydrological, pBufferTimestep );
}

/*
 *	Apply the buffers (execute the relevant kernels etc.)
 */
void CBoundaryMap::applyBoundaries(COCLBuffer* pCellBuffer)
{
	for (mapBoundaries_t::iterator it = mapBoundaries.begin(); it != mapBoundaries.end(); it++)
		(it->second)->applyBoundary(pCellBuffer);
}

/*
*	Stream the buffer (i.e. prepare resources for the current time period)
*/
void CBoundaryMap::streamBoundaries(double dTime)
{
	for (mapBoundaries_t::iterator it = mapBoundaries.begin(); it != mapBoundaries.end(); it++)
	{
		(it->second)->streamBoundary(dTime);
	}
}

/*
 *	How many boundaries do we have?
 */
unsigned int CBoundaryMap::getBoundaryCount()
{
	return mapBoundaries.size();
}

/*
 *  Parse everything under the <boundaryConditions> element

bool	CBoundaryMap::setupFromConfig()
{

	//  Map file
	if (sMapFile.length() > 0)
	{
		pMapFile = new CCSVDataset(sMapFile);
		pMapFile->readFile();
	}

		pNewBoundary->importMap(pMapFile);

	delete pMapFile;



	//  Timeseries boundaries
	CBoundary *pNewBoundary = NULL;

	//pNewBoundary = static_cast<CBoundary*>( new CBoundaryCell( this->pDomain ) );
	pNewBoundary = static_cast<CBoundary*>(new CBoundaryUniform(this->pDomain));
	//pNewBoundary = static_cast<CBoundary*>(new CBoundaryGridded(this->pDomain));


	// Configure the new boundary
	if (!pNewBoundary->setupFromConfig())
	{
		model::doError(
			"Encountered an error loading a boundary definition.",
			model::errorCodes::kLevelWarning
		);
	}

	// Store the new boundary in the unordered map
	mapBoundaries[pNewBoundary->getName()] = pNewBoundary;
	pManager->log->writeLine("Loaded new boundary condition '" + pNewBoundary->getName() + "'.");


	return true;
}
 */
/*
*  Adjust cell bed elevations as necessary etc. around the boundaries
*/
void	CBoundaryMap::applyDomainModifications()
{
	CDomainCartesian* pDomain = static_cast<CDomainCartesian*>(this->pDomain);

	// Closed/open boundary conditions
	pDomain->imposeBoundaryModification(CDomainCartesian::kEdgeN, this->ucBoundaryTreatment[CDomainCartesian::kEdgeN]);
	pDomain->imposeBoundaryModification(CDomainCartesian::kEdgeE, this->ucBoundaryTreatment[CDomainCartesian::kEdgeE]);
	pDomain->imposeBoundaryModification(CDomainCartesian::kEdgeS, this->ucBoundaryTreatment[CDomainCartesian::kEdgeS]);
	pDomain->imposeBoundaryModification(CDomainCartesian::kEdgeW, this->ucBoundaryTreatment[CDomainCartesian::kEdgeW]);
}