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

#include "model.h"

// Base includes






#include "common.h"
















#include "Platforms/windows_platform.h"

// Basic functions and variables used throughout
namespace model
{
int						loadConfiguration();
int						commenceSimulation();
int						closeConfiguration();
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


extern  bool			quietMode;
extern  bool			forceAbort;
extern  bool			gdalInitiated;
extern	bool			disableScreen;
extern	bool			disableConsole;
extern  char*			codeDir;
extern  char*			logFile;
extern	CModel*			pManager;
}

// Variables in use throughput
using	model::pManager;
