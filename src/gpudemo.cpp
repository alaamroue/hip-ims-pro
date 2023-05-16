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
#include "gpudemo.h"
#include "CModel.h"
#include "Domain/CDomainManager.h"
#include "Domain/CDomain.h"
#include "Domain/Cartesian/CDomainCartesian.h"
#include "OpenCL/Executors/COCLDevice.h"
#include "Schemes\CSchemeGodunov.h"


CModel* pManager;
int loadConfiguration();
int commenceSimulation();
int closeConfiguration();
int doClose(int iCode);

/*
 *  Application entry-point. 
 */
int main()
{
	//system("color 17");
	//SetConsoleTitle("HiPIMS Simulation Engine");

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
	double SyncTime = 3600.00*100;
	pManager->setExecutorToDefaultGPU();											// Set Executor to a default GPU Config

	pManager->setSelectedDevice(2);												// Set GPU device to Use. Important: Has to be called after setExecutor. Default is the faster one.
	pManager->setName("Name");														// Set Name of Project
	pManager->setDescription("The Description");									// Set Description of Project
	pManager->setSimulationLength(SyncTime);										// Set Simulation Length
	pManager->setOutputFrequency(SyncTime);										// Set Output Frequency
	pManager->setFloatPrecision(model::floatPrecision::kDouble);					// Set Precision
	pManager->setCourantNumber(0.5);												// Set the Courant Number to be used (Godunov)
	pManager->setFrictionStatus(false);												// Flag for activating friction
	pManager->setCachedWorkgroupSize(8, 8);											// Set the work group size of the GPU for cached mode
	pManager->setNonCachedWorkgroupSize(8, 8);										// Set the work group size of the GPU for non-cached mode
	//pManager->setRealStart("2022-05-06 13:30", "%Y-%m-%d %H:%M");					//Sets Realtime

	CDomainCartesian* ourCartesianDomain = new CDomainCartesian(pManager);				//Creeate a new Domain
	ourCartesianDomain->configureDomain(0.00);
	CSchemeGodunov* pScheme = new CSchemeGodunov(pManager);
	pScheme->setDomain(ourCartesianDomain);
	pScheme->prepareAll();
	ourCartesianDomain->setScheme(pScheme);
	ourCartesianDomain->loadInitialConditions();

	CDomainCartesian* ourCartesianDomain2 = new CDomainCartesian(pManager);				//Creeate a new Domain
	ourCartesianDomain2->configureDomain(10.00);
	CSchemeGodunov* pScheme2 = new CSchemeGodunov(pManager);
	pScheme2->setDomain(ourCartesianDomain2);
	pScheme2->prepareAll();
	ourCartesianDomain2->setScheme(pScheme2);
	ourCartesianDomain2->loadInitialConditions();

	CDomainManager* pManagerDomains = pManager->getDomainSet();
	ourCartesianDomain->setID(pManagerDomains->getDomainCount());	// Should not be needed, but somehow is?
	ourCartesianDomain2->setID(pManagerDomains->getDomainCount());	// Should not be needed, but somehow is?

	//Set newly created domain to the model and do logging and checking
	pManagerDomains->domains.push_back(ourCartesianDomain);
	pManagerDomains->domains.push_back(ourCartesianDomain2);

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
	if ( !pManager ) return doClose(model::appReturnCodes::kAppInitFailure	);




	if (!pManager->runModel())
	{model::doError("Simulation start failed.", model::errorCodes::kLevelModelStop); return doClose(model::appReturnCodes::kAppFatal);}




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
