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

namespace model 
{

// Application author details
const std::string appName				= " _    _   _   _____    _____   __  __    _____  \n"
										  " | |  | | (_) |  __ \\  |_   _| |  \\/  |  / ____| \n"
										  " | |__| |  _  | |__) |   | |   | \\  / | | (___   \n"
										  " |  __  | | | |  ___/    | |   | |\\/| |  \\___ \\  \n"
										  " | |  | | | | | |       _| |_  | |  | |  ____) | \n"
										  " |_|  |_| |_| |_|      |_____| |_|  |_| |_____/  \n"
										  "   High-performance Integrated Modelling System   ";
const std::string appAuthor				= "Luke S. Smith and Qiuhua Liang";
const std::string appContact			= "luke@smith.ac";
const std::string appUnit				= "School of Civil Engineering and Geosciences";
const std::string appOrganisation		= "Newcastle University";
const std::string appRevision			= "$Revision: 717 $";

// Application version details
const unsigned int appVersionMajor		= 0;	// Major 
const unsigned int appVersionMinor		= 2;	// Minor
const unsigned int appVersionRevision	= 0;	// Revision

// Application structure for argument names
struct modelArgument {
	const char		cShort[3];
	const char *  cLong;
	const char *  cDescription;
};

}

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
void					storeWorkingEnv();
void					parseArguments( int, char*[] );
void					handleArgument( const char *, char* );

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


extern  bool			forceAbort;
extern  char*			logFile;
extern	CModel*			pManager;
}

// Variables in use throughput
using	model::pManager;

#endif
