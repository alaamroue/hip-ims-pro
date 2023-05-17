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
#include "Datasets/CRasterDataset.h"
#include "Floodplain/Normalplain.h"


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
	Normalplain* np = new Normalplain(100, 100);
	np->SetBedElevationMountainDef();



	pManager	= new CModel();
	double SyncTime = 3600.00*10;
	pManager->setExecutorToDefaultGPU();											// Set Executor to a default GPU Config

	pManager->setSelectedDevice(1);												// Set GPU device to Use. Important: Has to be called after setExecutor. Default is the faster one.
	pManager->setName("Name");														// Set Name of Project
	pManager->setDescription("The Description");									// Set Description of Project
	pManager->setSimulationLength(SyncTime);										// Set Simulation Length
	pManager->setOutputFrequency(SyncTime/10);										// Set Output Frequency
	pManager->setFloatPrecision(model::floatPrecision::kDouble);					// Set Precision
	pManager->setCourantNumber(0.5);												// Set the Courant Number to be used (Godunov)
	pManager->setFrictionStatus(false);												// Flag for activating friction
	pManager->setCachedWorkgroupSize(8, 8);											// Set the work group size of the GPU for cached mode
	pManager->setNonCachedWorkgroupSize(8, 8);										// Set the work group size of the GPU for non-cached mode
	//pManager->setRealStart("2022-05-06 13:30", "%Y-%m-%d %H:%M");					//Sets Realtime

	CDomainCartesian* ourCartesianDomain = new CDomainCartesian(pManager);				//Creeate a new Domain

	CRasterDataset	pDataset;

	pDataset.setLogger(pManager->log);
	pDataset.bAvailable = true;
	pDataset.ulRows = np->getSizeY();
	pDataset.ulColumns = np->getSizeX();
	pDataset.uiBandCount = 1;
	pDataset.dResolutionX = 10.0;
	pDataset.dResolutionY = 10.0;
	pDataset.dOffsetX = 0.00;
	pDataset.dOffsetY = 0.00;

	pDataset.logDetails();

	ourCartesianDomain->setProjectionCode(0);					// Unknown
	ourCartesianDomain->setUnits((char*)"m");
	ourCartesianDomain->setCellResolution(pDataset.dResolutionX);
	ourCartesianDomain->setRealDimensions(pDataset.dResolutionX * pDataset.ulColumns, pDataset.dResolutionY * pDataset.ulRows);
	ourCartesianDomain->setRealOffset(pDataset.dOffsetX, pDataset.dOffsetY);
	ourCartesianDomain->setRealExtent(
		pDataset.dOffsetY + pDataset.dResolutionY * pDataset.ulRows,
		pDataset.dOffsetX + pDataset.dResolutionX * pDataset.ulColumns,
		pDataset.dOffsetY,
		pDataset.dOffsetX
	);

	ourCartesianDomain->configureDomain(0.00);
	CSchemeGodunov* pScheme = new CSchemeGodunov(pManager);
	pScheme->setDomain(ourCartesianDomain);
	pScheme->prepareAll();

	ourCartesianDomain->setScheme(pScheme);

	unsigned long ulCellID;
	unsigned char	ucRounding = 4;			// decimal places
	for (unsigned long iRow = 0; iRow < np->getSizeX(); iRow++) {
		for (unsigned long iCol = 0; iCol < np->getSizeY(); iCol++) {
			ulCellID = ourCartesianDomain->getCellID(iCol, pDataset.ulRows - iRow - 1);
			//Elevations
			ourCartesianDomain->handleInputData(ulCellID, np->getBedElevation(ulCellID), model::rasterDatasets::dataValues::kBedElevation, ucRounding);
			//Manning Coefficient
			ourCartesianDomain->handleInputData(ulCellID, np->getManning(ulCellID), model::rasterDatasets::dataValues::kManningCoefficient, ucRounding);
			//Depth
			ourCartesianDomain->handleInputData(ulCellID, 0.0, model::rasterDatasets::dataValues::kDepth, ucRounding);
			//VelocityX
			ourCartesianDomain->handleInputData(ulCellID, 0.0, model::rasterDatasets::dataValues::kVelocityX, ucRounding);
			//VelocityY
			ourCartesianDomain->handleInputData(ulCellID, 0.0, model::rasterDatasets::dataValues::kVelocityY, ucRounding);
		}
	}
	





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
