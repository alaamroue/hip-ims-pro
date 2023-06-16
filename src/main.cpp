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
#include "CRasterDataset.h"
#include "COCLDevice.h"
#include "CDomainManager.h"
#include "CDomain.h"
#include "CBoundaryMap.h"
#include "CScheme.h"

// Globals
CModel*					model::pManager;

/*
 *  Application entry-point. 
 */
int main()
{
	// Default configurations
	model::configFile	= new char[50];


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
	CModel* pManager = new CModel();

	CExecutorControl* pExecutor = CExecutorControl::createExecutor(model::executorTypes::executorTypeOpenCL);
	//pExecutor->setDeviceFilter(model::filters::devices::devicesCPU);
	pExecutor->setDeviceFilter(model::filters::devices::devicesGPU);
	//pExecutor->setDeviceFilter(model::filters::devices::devicesAPU);
	pExecutor->createDevices();
	pManager->setExecutor(pExecutor);


	pManager->setName("Name");
	pManager->setDescription("Desc");
	pManager->setSimulationLength(1000.0);
	pManager->setOutputFrequency(1000.0);
	//pManager->setFloatPrecision(model::floatPrecision::kSingle);
	pManager->setFloatPrecision(model::floatPrecision::kDouble);


	pManager->getDomainSet()->setSyncMethod(model::syncMethod::kSyncTimestep);
	pManager->getDomainSet()->setSyncMethod(model::syncMethod::kSyncForecast);
	//pManager->getDomainSet()->setSyncBatchSpares(10);


	CDomainBase* pDomainNew;
	pDomainNew = CDomainBase::createDomain(model::domainStructureTypes::kStructureCartesian);
	static_cast<CDomain*>(pDomainNew)->setDevice(pManager->getExecutor()->getDevice(1));
	CDomainCartesian* ourCartesianDomain = (CDomainCartesian*) pDomainNew;


	unsigned long ulCellID;
	for (unsigned long iRow = 0; iRow < 100; iRow++) {
		for (unsigned long iCol = 0; iCol < 100; iCol++) {
			ulCellID = ourCartesianDomain->getCellID(iCol, ourCartesianDomain->getRows() - iRow - 1);
			//Elevations
			ourCartesianDomain->handleInputData(ulCellID, pow(iRow* iRow + iCol * iCol,0.5), model::rasterDatasets::dataValues::kBedElevation, pManager->ucRounding);
			//Manning Coefficient
			ourCartesianDomain->handleInputData(ulCellID, 0.03, model::rasterDatasets::dataValues::kManningCoefficient, pManager->ucRounding);
			//Depth
			ourCartesianDomain->handleInputData(ulCellID, 0.0, model::rasterDatasets::dataValues::kDepth, pManager->ucRounding);
			//VelocityX
			ourCartesianDomain->handleInputData(ulCellID, 0.0, model::rasterDatasets::dataValues::kVelocityX, pManager->ucRounding);
			//VelocityY
			ourCartesianDomain->handleInputData(ulCellID, 0.0, model::rasterDatasets::dataValues::kVelocityY, pManager->ucRounding);
			//Boundary Condition
			ourCartesianDomain->setBoundaryCondition(ulCellID, 0.0);
			//Coupling Condition
			ourCartesianDomain->setCouplingCondition(ulCellID, 0.0);

		}
	}



	CScheme* pScheme;
	pScheme = CScheme::createScheme(model::schemeTypes::kGodunov);
	//pScheme = CScheme::createScheme(model::schemeTypes::kMUSCLHancock);
	//pScheme = CScheme::createScheme(model::schemeTypes::kInertialSimplification);

	//General Scheme Config
	pScheme->setQueueMode(model::queueMode::kAuto);
	pScheme->setQueueSize(1);

	SchemeSettings schemeSettings;
	schemeSettings.CourantNumber = 0.5;
	schemeSettings.DryThreshold = 1e-5;
	schemeSettings.Timestep = 0.01;
	schemeSettings.ReductionWavefronts = 200;
	schemeSettings.FrictionStatus = false;
	schemeSettings.CachedWorkgroupSize[0] = 8;
	schemeSettings.CachedWorkgroupSize[1] = 8;
	schemeSettings.NonCachedWorkgroupSize[0] = 8;
	schemeSettings.NonCachedWorkgroupSize[1] = 8;
	schemeSettings.CacheMode = model::schemeConfigurations::godunovType::kCacheNone;
	schemeSettings.CacheConstraints = model::cacheConstraints::godunovType::kCacheActualSize;
	schemeSettings.ExtrapolatedContiguity = false;

	pScheme->setupScheme(schemeSettings);
	pScheme->setDomain(ourCartesianDomain);			// Scheme allocates the memory and thus needs to know the dimensions
	pScheme->prepareAll();							//Needs Dimension data to alocate memory
	ourCartesianDomain->setScheme(pScheme);

	pDomainNew->setID(1);	// Should not be needed, but somehow is?
	pManager->getDomainSet()->getDomainBaseVector()->push_back(pDomainNew);


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
	delete pManager;
	model::doPause();

	pManager			= NULL;

	return iCode;
}

/*
 *  Suspend the application temporarily pending the user
 *  pressing return to continue.
 */
void model::doPause()
{
	std::cout << std::endl << "Press any key to close." << std::endl;
	std::cout << std::endl << "Press any key to close." << std::endl;
	std::getchar();
}

/*
 *  Raise an error message and deal with it accordingly.
 */
void model::doError( std::string sError, unsigned char cError )
{
	pManager->log->writeError( sError, cError );
	if ( cError & model::errorCodes::kLevelModelStop )
		std::cout << "model forceAbort was requested by a function." << std::endl;
	if ( cError & model::errorCodes::kLevelFatal )
	{
		model::doPause();
		exit( model::appReturnCodes::kAppFatal );

		
	}
}

