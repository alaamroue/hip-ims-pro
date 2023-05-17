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
 *  Application entry point. Instantiates the
 *  management class.
 * ------------------------------------------
 *
 */

// Includes
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include "main.h"
#include "CModel.h"
#include "MPI/CMPIManager.h"
#include "Datasets/CXMLDataset.h"
#include "Datasets/CRasterDataset.h"
#include "OpenCL/Executors/COCLDevice.h"
#include "Domain/CDomainManager.h"
#include "Domain/CDomain.h"
#include "Boundaries/CBoundaryMap.h"
#include "Schemes\CSchemeGodunov.h"

// Globals
CModel*					model::pManager;
CXMLDataset*			pConfigFile;
char*					model::workingDir;
char*					model::logFile;
char*					model::configFile;
char*					model::codeDir;
bool					model::quietMode;
bool					model::forceAbort;
bool					model::gdalInitiated;
bool					model::disableScreen;
bool					model::disableConsole;

/*
 *  Application entry-point. 
 */
int _tmain(int argc, char* argv[])
{
	// Default configurations
	model::configFile	= new char[50];
	model::logFile		= new char[50];
	model::forceAbort	= false;

	std::strcpy( model::logFile,    "_model.log" );

	model::storeWorkingEnv();

	int iReturnCode = model::loadConfiguration();
	if ( iReturnCode != model::appReturnCodes::kAppSuccess ) return iReturnCode;
	iReturnCode = model::commenceSimulation();
	if ( iReturnCode != model::appReturnCodes::kAppSuccess ) return iReturnCode;
	iReturnCode = model::closeConfiguration();
	if ( iReturnCode != model::appReturnCodes::kAppSuccess ) return iReturnCode;

	return iReturnCode;
}


/*
 *  Load the specified model config file and probe for devices etc.
 */
int model::loadConfiguration()
{
	pManager = new CModel();

	pManager->log->writeLine("Reading configuration: execution settings...");

	CExecutorControlOpenCL* pExecutor = new CExecutorControlOpenCL();
	pExecutor->logPlatforms();
	pExecutor->setDeviceFilter(model::filters::devices::devicesGPU);
	if (!pExecutor->createDevices()) return false;									//Creates Device
	pManager->setExecutor(pExecutor);


	pManager->log->writeLine("Reading configuration: model metadata...");

	pManager->setName(std::string("Name"));
	pManager->setDescription(std::string("Desc"));


	pManager->log->writeLine("Reading configuration: simulation settings...");

	pManager->setSimulationLength(360000.0);
	//pManager->setRealStart(cParameterValue, cParameterFormat);
	pManager->setOutputFrequency(360000.0/2);
	pManager->setFloatPrecision(model::floatPrecision::kDouble);


	pManager->log->writeLine("Reading configuration: domain data...");

	CDomainManager* pManagerDomainManager = pManager->getDomainSet();
	//pManagerDomainManager->setSyncMethod(model::syncMethod::kSyncTimestep);
	//pManagerDomainManager->setSyncMethod(model::syncMethod::kSyncForecast);
	//pManagerDomainManager->setSyncBatchSpares(10);


	CDomainBase* ourNewDomain;
	unsigned int uiDeviceAdjust = 1;
	// Domain resides on this node
	pManager->log->writeLine("Creating a new Cartesian-structured domain.");


	ourNewDomain = CDomainBase::createDomain(model::domainStructureTypes::kStructureCartesian);
	CDomainCartesian* ourCartesianDomain = (CDomainCartesian*)ourNewDomain;
	ourCartesianDomain->setDevice(pManager->getExecutor()->getDevice(1 - uiDeviceAdjust + 1));

	pManager->log->writeLine("Dimensioning domain from raster dataset.");

	ourCartesianDomain->setProjectionCode(0);					// Unknown
	ourCartesianDomain->setUnits("m");
	ourCartesianDomain->setCellResolution(10.0);
	ourCartesianDomain->setRealDimensions(10.0 * 100, 10.0 * 100);
	ourCartesianDomain->setRealOffset(0.0, 0.0);
	ourCartesianDomain->setRealExtent(0 + 10.0 * 100.0, 0.0 + 10 * 100, 0.0, 0.0);


	pManager->log->writeLine("Progressing to load boundary conditions.");

	CBoundaryMap* ourBoundryMap = ourCartesianDomain->getBoundaries();
	//CBoundary* pNewBoundary = NULL;
	//pNewBoundary = static_cast<CBoundary*>(new CBoundaryCell(ourBoundryMap->pDomain));
	//pNewBoundary = static_cast<CBoundary*>(new CBoundaryUniform(ourBoundryMap->pDomain));
	//pNewBoundary = static_cast<CBoundary*>(new CBoundaryGridded(ourBoundryMap->pDomain));

	ourCartesianDomain->pScheme = new CSchemeGodunov();
	ourCartesianDomain->pScheme->setDomain(ourCartesianDomain);
	ourCartesianDomain->pScheme->prepareAll();
	ourCartesianDomain->setScheme(ourCartesianDomain->pScheme);

	pManager->log->writeLine("Progressing to load initial conditions.");
	unsigned long ulCellID;
	unsigned char	ucRounding = 4;			// decimal places
	for (unsigned long iRow = 0; iRow < 100; iRow++) {
		for (unsigned long iCol = 0; iCol < 100; iCol++) {
			ulCellID = ourCartesianDomain->getCellID(iCol, 100 - iRow - 1);
			//Elevations
			ourCartesianDomain->handleInputData(ulCellID, sqrt(iCol * iCol + iRow * iRow) / 10, model::rasterDatasets::dataValues::kBedElevation, ucRounding);
			//Manning Coefficient
			ourCartesianDomain->handleInputData(ulCellID, 0.0286, model::rasterDatasets::dataValues::kManningCoefficient, ucRounding);
			//Depth
			ourCartesianDomain->handleInputData(ulCellID, 0.0, model::rasterDatasets::dataValues::kDepth, ucRounding);
			//VelocityX
			ourCartesianDomain->handleInputData(ulCellID, 0.0, model::rasterDatasets::dataValues::kVelocityX, ucRounding);
			//VelocityY
			ourCartesianDomain->handleInputData(ulCellID, 0.0, model::rasterDatasets::dataValues::kVelocityY, ucRounding);
		}
	}



	ourCartesianDomain->setID(pManagerDomainManager->getDomainCount());	// Should not be needed, but somehow is?
	pManagerDomainManager->domains.push_back(ourCartesianDomain);

	// Warn the user if it's a multi-domain model, just so they know...
	if (pManagerDomainManager->domains.size() <= 1)
	{
		pManager->log->writeLine("This is a SINGLE-DOMAIN model, limited to 1 device.");
	}
	else {
		pManager->log->writeLine("This is a MULTI-DOMAIN model, and possibly multi-device.");
	}

	// Generate links
	pManagerDomainManager->generateLinks();

	// Spit out some details
	pManagerDomainManager->logDetails();

	// If we have more than one domain then we also need at least one link per domain
	// otherwise something is wrong...
	if (pManagerDomainManager->domains.size() > 1)
	{
		for (unsigned int i = 0; i < pManagerDomainManager->domains.size(); i++)
		{
			if (pManagerDomainManager->domains[i]->getLinkCount() < 1)
			{
				model::doError(
					"One or more domains are not linked.",
					model::errorCodes::kLevelModelStop
				);
				return false;
			}
		}
	}


	pManager->log->writeLine("The computational engine is now ready.");

	return model::appReturnCodes::kAppSuccess;
}

/*
 *  Read in configuration file and launch a new simulation
 */
int model::commenceSimulation()
{
	if ( !pManager ) 
		return model::doClose(
			model::appReturnCodes::kAppInitFailure	
		);

	// ---
	//  MODEL RUN
	// ---
	if ( !pManager->runModel() )
	{
		model::doError(
			"Simulation start failed.",
			model::errorCodes::kLevelModelStop
		);
		return model::doClose( 
			model::appReturnCodes::kAppFatal 
		);
	}

	// Allow safe deletion 
	pManager->runModelCleanup();

	return model::appReturnCodes::kAppSuccess;
}

/*
 *  Close down the simulation
 */
int model::closeConfiguration()
{
	return model::doClose( 
		model::appReturnCodes::kAppSuccess 
	);
}

/*
 *  Parse command-line arguments
 */
void model::parseArguments( int iArgCount, char* cArgEntities[] )
{
	// Arguments to check for
	unsigned int	argOptionCount = 6;
	modelArgument	argOptions[]   = {
		{	
			"-c",
			"--config-file\0",
			"XML-based configuration file defining the model\0"
		},
		{	
			"-l",
			"--log-file\0",
			"File for model execution log\0"
		},
		{
			"-s",
			"--quiet-mode\0",
			"Disable all requirements for user feedback\0"
		},
		{
			"-n",
			"--disable-screen\0",
			"Disable using a screen for output with NCurses\0"
		},
		{
			"-m",
			"--mpi-mode\0",
			"Adapt output so only one process provides console output\0"
		},
		{
			"-x",
			"--code-dir\0",
			"Directory containing the OpenCL code structure\0"
		}
	};

	// First argument is the invokation command
	for( int i = 1; i < iArgCount; i++ )
	{
		for( unsigned int j = 0; j < argOptionCount; j++ )
		{
			// Check for short (unusually single char) argument option
			if ( std::strcmp( argOptions[ j ].cShort, const_cast<const char*>( cArgEntities[ i ] ) ) == 0 )
			{
				handleArgument( argOptions[ j ].cLong, static_cast<char*>( cArgEntities[ i + 1 ] ) );
				++i;
			}

			// Check for long (followed by equal sign) argument option
			if ( std::strstr( cArgEntities[ i ], const_cast<const char*>( argOptions[ j ].cLong ) ) != NULL )
			{
				std::string sArgument	= cArgEntities[ i ];
				std::string sValue		= "";
				if( strlen( cArgEntities[i] ) > strlen( argOptions[ j ].cLong ) + 1 )
							sValue      = sArgument.substr( strlen( argOptions[ j ].cLong ) + 1, 255 );
				char*		cValue		= new char[ sValue.length() + 1 ];
				cValue = const_cast<char*>( sValue.c_str() );
				handleArgument( argOptions[ j ].cLong, cValue );
			}
		}
	}
}

/*
 *  Handle an argument that's been passed
 */
void model::handleArgument( const char * cLongName, char * cValue )
{
	if ( strcmp( cLongName, "--config-file" ) == 0 )
	{
		configFile = new char[ strlen( cValue ) + 1 ];
		strcpy( configFile, cValue );
	}

	else if ( strcmp( cLongName, "--log-file" ) == 0 )
	{
		logFile = new char[ strlen( cValue ) + 1 ];
		strcpy( logFile, cValue );
	}
	
	else if ( strcmp( cLongName, "--code-dir" ) == 0 )
	{
		codeDir = new char[ strlen( cValue ) + 1 ];
		strcpy( codeDir, cValue );
	}

	else if ( strcmp( cLongName, "--quiet-mode" ) == 0 )
	{
		model::quietMode = true;
	}

	else if (strcmp(cLongName, "--disable-screen") == 0)
	{
		model::disableScreen = true;
	}

	else if (strcmp(cLongName, "--mpi-mode") == 0)
	{
#ifdef MPI_ON
		int iProcCount, iProcID;

		MPI_Comm_rank(MPI_COMM_WORLD, &iProcID);
		MPI_Comm_size(MPI_COMM_WORLD, &iProcCount);

		// Disable outputs for all except the master process
		model::quietMode = (iProcID != 0);
		model::disableScreen = (iProcID != 0);
		model::disableConsole = (iProcID != 0);
#else
		model::quietMode = true;
		model::disableScreen = true;
		model::disableConsole = true;
#endif
	}
}

/*
 *  Model is complete.
 */
int model::doClose( int iCode )
{
	CRasterDataset::cleanupAll();
	delete pConfigFile;
	delete pManager;
	delete [] model::workingDir;
	delete [] model::logFile;			// TODO: Fix me...
	delete [] model::configFile;
	delete [] model::codeDir;
	model::doPause();

	pManager			= NULL;
	pConfigFile			= NULL;
	model::workingDir	= NULL;
	model::logFile		= NULL;
	model::configFile	= NULL;

	return iCode;
}

/*
 *  Suspend the application temporarily pending the user
 *  pressing return to continue.
 */
void model::doPause()
{
#ifndef _WINDLL
	if ( model::quietMode ) return;
#ifndef PLATFORM_UNIX
	std::cout << std::endl << "Press any key to close." << std::endl;
#else
	if ( !model::disableScreen )
	{
		addstr( "Press any key to close.\n" );
		refresh();
	}
	else {
		std::cout << std::endl << "Press any key to close." << std::endl;
	}
#endif
	std::getchar();
#endif
}

/*
 *  Raise an error message and deal with it accordingly.
 */
void model::doError( std::string sError, unsigned char cError )
{
	pManager->log->writeError( sError, cError );
	if ( cError & model::errorCodes::kLevelModelStop )
		model::forceAbort = true;
	if ( cError & model::errorCodes::kLevelFatal )
	{
		model::doPause();
#ifdef _CONSOLE
#ifdef MPI_ON
		MPI_Finalize();
#endif
		exit( model::appReturnCodes::kAppFatal );
#else
		model::fCallbackComplete(
			model::appReturnCodes::kAppFatal
		);
		ExitThread(model::appReturnCodes::kAppFatal);
#endif
		
	}
}

/*
 *  Discovers the full path of the current working directory.
 */
void model::storeWorkingEnv()
{


	if ( model::workingDir != NULL )
		return;

	char cPath[ _MAX_PATH ];

	// ISO compliant version of getcwd
	_getcwd( cPath, _MAX_PATH );

	// Verify that we've managed to obtain a path
	if ( strcmp( cPath, "" ) == 0 )
	{
		// Fallback is temp directory
		strcpy_s( cPath, "%TEMP%" );
	}
	model::workingDir = new char[ _MAX_PATH ];
	std::strcpy( model::workingDir, cPath );
}
