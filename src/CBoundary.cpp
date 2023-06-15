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
#include "CBoundary.h"
#include "common.h"
#include "CDomain.h"
#include "CDomainCartesian.h"
#include "opencl.h"

using std::vector;
int CBoundary::uiInstances = 0;

/* 
 *  Constructor
 */
CBoundary::CBoundary( CDomain* pDomain )
{
	sName = "Boundary_" + toString( ++CBoundary::uiInstances );
	this->pDomain = pDomain;
}

/*
 *  Destructor
 */
CBoundary::~CBoundary()
{
	// ...
}

