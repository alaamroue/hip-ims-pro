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
 *  Domain Class
 * ------------------------------------------
 *
 */

#include "common.h"
#include "CDomainBase.h"
#include "CDomainCartesian.h"

/*
 *  Constructor
 */
CDomainBase::CDomainBase(void)
{
	this->uiRollbackLimit	= 999999999;

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
CDomainBase::~CDomainBase(void)
{

}


/*
 *  Is this domain ready to be used for a model run?
 */
bool	CDomainBase::isInitialised()
{
	return true;
}

/*
 *  Return the total number of cells in the domain
 */
unsigned long	CDomainBase::getCellCount()
{
	return this->ulCellCount;
}

/*
 *	Fetch summary information for this domain
 */
CDomainBase::DomainSummary CDomainBase::getSummary()
{
	CDomainBase::DomainSummary pSummary;

	pSummary.uiNodeID = 0;
	
	return pSummary;
}

/*
 *	Fetch a cell ID based on Cartesian assumption and data held in the summary
 */
unsigned long	CDomainBase::getCellID(unsigned long ulX, unsigned long ulY)
{
	DomainSummary pSummary = this->getSummary();
	return (ulY * pSummary.ulColCount) + ulX;
}
