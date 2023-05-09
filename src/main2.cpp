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
#include "main2.h"
#include "CModel.h"
#include "Datasets/CRasterDataset.h"
#include "OpenCL/Executors/COCLDevice.h"
#include "Domain/CDomainManager.h"
#include "Domain/CDomain.h"
#include "Schemes\CSchemeGodunov.h"

// Globals
CModel* pManager;
int loadConfiguration();
int commenceSimulation();
int closeConfiguration();
int doClose(int iCode);

//CModel*					model::pManager;

/*
 *  Application entry-point. 
 */
int main()
{

	// Nasty function calls for Windows console stuff
	system("color 17");
	SetConsoleTitle("HiPIMS Simulation Engine");

	int iReturnCode = loadConfiguration();
	iReturnCode = commenceSimulation();
	iReturnCode = closeConfiguration();

	return iReturnCode;
}




/*
 *  Load the specified model config file and probe for devices etc.
 */
int loadConfiguration()
{
	pManager	= new CModel();

	pManager->setExecutorToDefaultGPU();											//Set Executor to a default GPU Config
	pManager->setName("Name");														//Set Name
	pManager->setDescription("The Description");									//Set Description
	pManager->setSimulationLength(3600);											//Set Simulation Length
	pManager->setOutputFrequency(3600);												//Set Output Frequency
	pManager->setFloatPrecision(model::floatPrecision::kDouble);					//Set Precision
	pManager->setCourantNumber(0.5);
	pManager->setFrictionStatus(false);
	pManager->setCachedWorkgroupSize(1, 1);
	pManager->setNonCachedWorkgroupSize(1, 1);
	//pManager->setRealStart("2022-05-06 13:30", "%Y-%m-%d %H:%M");					//Sets Realtime

	CDomainCartesian* ourCartesianDomain = new CDomainCartesian(pManager);				//Creeate a new Domain
	ourCartesianDomain->configureDomain();



	CSchemeGodunov* pScheme = new CSchemeGodunov(pManager);
	pScheme->setDomain(ourCartesianDomain);
	pScheme->prepareAll();

	ourCartesianDomain->setScheme(pScheme);
	pManager->log->writeLine("Numerical scheme reports is ready.");
	pManager->log->writeLine("Progressing to load initial conditions.");
	ourCartesianDomain->loadInitialConditions();

	CDomainManager* pManagerDomains = pManager->getDomainSet();

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
int commenceSimulation()
{
	if ( !pManager ) 
		return doClose(
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
		return doClose( 
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
int closeConfiguration()
{
	return doClose( 
		model::appReturnCodes::kAppSuccess 
	);
}

/*
 *  Model is complete.
 */
int doClose( int iCode )
{
	CRasterDataset::cleanupAll();
	delete pManager;
	std::getchar();

	pManager			= NULL;

	return iCode;
}

/*
 *  Raise an error message and deal with it accordingly.
 */
void model::doError( std::string sError, unsigned char cError )
{
	pManager->log->writeError( sError, cError );
	if ( cError & model::errorCodes::kLevelModelStop )
		pManager->forcedAbort = true;
	if ( cError & model::errorCodes::kLevelFatal )
	{
		std::getchar();
	}
}
