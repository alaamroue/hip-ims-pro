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
 *  Main include only required for some files
 *  Contains app version settings, etc.
 * ------------------------------------------
 *
 */
#ifndef HIPIMS_MAIN_H_
#define HIPIMS_MAIN_H_

// Includes
//#include <vld.h>				// Memory leak detection
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>
#include <math.h>
#include <cmath>
#include <stdexcept>
#include <thread>


// Base includes
#include "common.h"
#include "platforms.h"

// Basic functions and variables used throughout
namespace model
{
int						loadConfiguration();
int						commenceSimulation();
int						closeConfiguration();
void					outputVersion();
void					doPause();
int						doClose( int );

// Data structures used in interop
struct DomainData
{
	double			dResolution;
	double			dWidth;
	double			dHeight;
	double			dCornerWest;
	double			dCornerSouth;
	unsigned long	ulCellCount;
	unsigned long	ulRows;
	unsigned long	ulCols;
	unsigned long	ulBoundaryCells;
	unsigned long	ulBoundaryOthers;
};

extern	CModel*			pManager;
}


#endif
