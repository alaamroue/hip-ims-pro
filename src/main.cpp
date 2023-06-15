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
#include "main.h"
#include "CModel.h"
#include "COCLDevice.h"
#include "CDomainManager.h"
#include "CDomain.h"

// Globals
CModel*					model::pManager;



#ifdef PLATFORM_WIN
/*
 *  Application entry-point. 
 */
int _tmain(int argc, char* argv[])
{
	// Default configurations
	model::workingDir   = NULL;
	model::configFile	= new char[50];
	model::logFile		= new char[50];
	model::codeDir		= NULL;
	model::quietMode	= false;
	model::forceAbort	= false;
	model::gdalInitiated = true;
	model::disableScreen = true;
	model::disableConsole = false;

	std::strcpy( model::configFile, "configuration.xml" );
	std::strcpy( model::logFile,    "_model.log" );

	// Nasty function calls for Windows console stuff
	system("color 17");
	SetConsoleTitle("HiPIMS Simulation Engine");

	model::storeWorkingEnv();
	model::parseArguments( argc, argv );
	CRasterDataset::registerAll();

	int iReturnCode = model::loadConfiguration();
	if ( iReturnCode != model::appReturnCodes::kAppSuccess ) return iReturnCode;
	iReturnCode = model::commenceSimulation();
	if ( iReturnCode != model::appReturnCodes::kAppSuccess ) return iReturnCode;
	iReturnCode = model::closeConfiguration();
	if ( iReturnCode != model::appReturnCodes::kAppSuccess ) return iReturnCode;

	return iReturnCode;
}
#endif



/*
 *  Load the specified model config file and probe for devices etc.
 */
int model::loadConfiguration()
{
	pManager	= new CModel();

	// ---
	//  MPI
	// ---
#ifdef MPI_ON
	if (pManager->getMPIManager() != NULL)
	{
		pManager->getMPIManager()->logDetails();
		pManager->getMPIManager()->exchangeConfiguration( pConfigFile );
	}
#endif

	// ---
	//  CONFIG FILE
	// ---
	if (model::configFile != NULL && pManager->getMPIManager() == NULL)
	{
		boost::filesystem::path pConfigPath(model::configFile);
		boost::filesystem::path pConfigDir = pConfigPath.parent_path();
		pConfigFile = new CXMLDataset( std::string( model::configFile ) );
		chdir(boost::filesystem::canonical(pConfigDir).string().c_str());
	}
	else if (model::configFile == NULL) {
		pConfigFile = new CXMLDataset( "" );
	}

	if ( !pConfigFile->parseAsConfigFile() )
	{
		model::doError(
			"Cannot load model configuration.",
			model::errorCodes::kLevelModelStop
		);
		return model::doClose(
			model::appReturnCodes::kAppInitFailure
		);
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
#ifdef PLATFORM_UNIX

	// THIS IS TEMPORARY ONLY!
	char cPath[ FILENAME_MAX ];

	// Unix version of getcwd
	getcwd( cPath, FILENAME_MAX );

	// Verify that we've managed to obtain a path
	if ( strcmp( cPath, "" ) == 0 )
	{
		// Fallback is temp directory
		strcpy( cPath, "/tmp/" );
	}
	model::workingDir = new char[ FILENAME_MAX ];
	std::strcpy( model::workingDir, cPath );
#endif
#ifdef PLATFORM_WIN

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
#endif
}
