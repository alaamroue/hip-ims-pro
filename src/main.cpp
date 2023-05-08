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
#include "Datasets/CRasterDataset.h"
#include "OpenCL/Executors/COCLDevice.h"
#include "Domain/CDomainManager.h"
#include "Domain/CDomain.h"

// Globals
CModel*					model::pManager;
char*					model::logFile;
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
	model::logFile = new char[50];
	model::codeDir		= NULL;
	model::quietMode	= false;
	model::forceAbort	= false;
	model::gdalInitiated = true;
	model::disableScreen = true;
	model::disableConsole = false;

	std::strcpy( model::logFile,    "./_modelzz.log" );

	// Nasty function calls for Windows console stuff
	system("color 17");
	SetConsoleTitle("HiPIMS Simulation Engine");

	int iReturnCode = model::loadConfiguration();
	iReturnCode = model::commenceSimulation();
	iReturnCode = model::closeConfiguration();

	return iReturnCode;
}




/*
 *  Load the specified model config file and probe for devices etc.
 */
int model::loadConfiguration()
{
	pManager	= new CModel();
	CDomainManager* pManagerDomains = pManager->getDomainSet();						//Read Domain and Boundries


	pManager->setExecutorToDefaultGPU();											//Set Executor to a default GPU Config
	pManager->setName("Name");														//Set Name
	pManager->setDescription("The Description");									//Set Description
	pManager->setSimulationLength(3600*20);											//Set Simulation Length
	pManager->setOutputFrequency(3600*20/20);												//Set Output Frequency
	pManager->setFloatPrecision(model::floatPrecision::kDouble);					//Set Precision 
	//pManager->setRealStart("2022-05-06 13:30", "%Y-%m-%d %H:%M");					//Sets Realtime

	//Create new Domain
	CDomainCartesian* ourCartesianDomain = new CDomainCartesian;				//Creeate a new Domain
	static_cast<CDomain*>(ourCartesianDomain)->setDevice(pManager->getExecutor()->getDevice(1));    //TODO: Alaa it not always 1 is it? Review the original code
	
	if (!ourCartesianDomain->configureDomain()) return 4141;
	
	ourCartesianDomain->setID(pManagerDomains->getDomainCount());	// Should not be needed, but somehow is?

	//Set newly created domain to the model and do logging and checking
	pManagerDomains->domains.push_back(ourCartesianDomain);
	pManagerDomains->logDomainMultiOrSingle();
	pManagerDomains->generateLinks();
	pManagerDomains->logDetails();
	pManagerDomains->checkDomainLinks();

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
 *  Model is complete.
 */
int model::doClose( int iCode )
{
	CRasterDataset::cleanupAll();
	delete pManager;
	delete [] model::logFile;			// TODO: Fix me...
	delete [] model::codeDir;
	model::doPause();

	pManager			= NULL;
	//model::workingDir	= NULL;
	model::logFile		= NULL;

	return iCode;
}

/*
 *  Suspend the application temporarily pending the user
 *  pressing return to continue.
 */
void model::doPause()
{
	if ( model::quietMode ) return;
	//std::getchar();
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
		
	}
}

