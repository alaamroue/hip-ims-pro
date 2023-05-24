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
	sName = "Boundary_" + std::to_string( ++CBoundary::uiInstances );
	this->pDomain = pDomain;
}

/*
 *  Destructor
 */
CBoundary::~CBoundary()
{
	// ...
}

