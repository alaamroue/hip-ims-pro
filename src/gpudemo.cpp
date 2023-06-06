/*
 * This file is a modified version of the code originally created by Luke S. Smith and Qiuhua Liang. (Originally main.cpp)
 * Modifications: Project structure changes (Compare with original for exact changes)
 * Modified by: Alaa Mroue
 * Date of Modification: 04.2023
 *
 * Find the orignal code in OriginalSourceCode.zip
 * OriginalSourceCode.zip: Is a snapshot of the src folder from https://github.com/lukeshope/hipims-ocl based on 1e62acf6b9b480e08646b232361b68c1827d91ae
 */

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
#include <bitset>

#include "gpudemo.h"
#include "CModel.h"
#include "CDomainManager.h"
#include "CDomain.h"
#include "CDomainCartesian.h"
#include "CRasterDataset.h"
#include "COCLDevice.h"
#include "CSchemeGodunov.h"
#include "CSchemePromaides.h"
#include "Normalplain.h"


CModel* model::cModel;
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
	np->SetBedElevationMountain();

	pManager	= new CModel();
	model::cModel = pManager;
	double SyncTime = 3600.0*100;
	pManager->setExecutorToDefaultGPU();											// Set Executor to a default GPU Config

	pManager->setSelectedDevice(1);												// Set GPU device to Use. Important: Has to be called after setExecutor. Default is the faster one.
	pManager->setName("Name");														// Set Name of Project
	pManager->setDescription("The Description");									// Set Description of Project
	pManager->setSimulationLength(SyncTime);										// Set Simulation Length
	pManager->setOutputFrequency(SyncTime);										// Set Output Frequency
	pManager->setFloatPrecision(model::floatPrecision::kDouble);					// Set Precision
	pManager->setCourantNumber(0.9);												// Set the Courant Number to be used (Godunov)
	pManager->setFrictionStatus(false);												// Flag for activating friction
	pManager->setCachedWorkgroupSize(8, 8);											// Set the work group size of the GPU for cached mode
	pManager->setNonCachedWorkgroupSize(8, 8);										// Set the work group size of the GPU for non-cached mode
	//pManager->setRealStart("2022-05-06 13:30", "%Y-%m-%d %H:%M");					//Sets Realtime

	CDomainCartesian* ourCartesianDomain = new CDomainCartesian(pManager);				//Creeate a new Domain

	CRasterDataset	pDataset;

	pDataset.setLogger(pManager->log);
	pDataset.bAvailable = true;
	pDataset.ulRows = np->getSizeX();
	pDataset.ulColumns = np->getSizeY();
	pDataset.uiBandCount = 1;
	pDataset.dResolutionX = 10.0;
	pDataset.dResolutionY = 10.00;
	pDataset.dOffsetX = 0.00;
	pDataset.dOffsetY = 0.00;

	//pDataset.logDetails();

	ourCartesianDomain->setProjectionCode(0);					// Unknown
	ourCartesianDomain->setUnits("m");
	ourCartesianDomain->setCellResolution(pDataset.dResolutionX);
	ourCartesianDomain->setRealDimensions(pDataset.dResolutionX * pDataset.ulColumns, pDataset.dResolutionY * pDataset.ulRows);
	ourCartesianDomain->setRealOffset(pDataset.dOffsetX, pDataset.dOffsetY);
	ourCartesianDomain->setRealExtent(
		pDataset.dOffsetY + pDataset.dResolutionY * pDataset.ulRows,
		pDataset.dOffsetX + pDataset.dResolutionX * pDataset.ulColumns,
		pDataset.dOffsetY,
		pDataset.dOffsetX
	);

	//CSchemeGodunov* pScheme = new CSchemeGodunov(pManager);
	CSchemePromaides* pScheme = new CSchemePromaides(pManager);
	pScheme->setDryThreshold(1E-10);
	pScheme->setDomain(ourCartesianDomain);
	pScheme->prepareAll();
	ourCartesianDomain->setScheme(pScheme);


	unsigned long ulCellID;
	model::FlowStates flowStates; flowStates.isFlowElement = true; flowStates.noflow_x = false; flowStates.noflow_y = false; flowStates.opt_pol_x = false; flowStates.opt_pol_y = false;
	for (unsigned long iRow = 0; iRow < np->getSizeX(); iRow++) {
		for (unsigned long iCol = 0; iCol < np->getSizeY(); iCol++) {
			ulCellID = ourCartesianDomain->getCellID(iCol, pDataset.ulRows - iRow - 1);
			//Elevations
			ourCartesianDomain->handleInputData(ulCellID, np->getBedElevation(ulCellID), model::rasterDatasets::dataValues::kBedElevation, pManager->ucRounding);
			//Manning Coefficient
			ourCartesianDomain->handleInputData(ulCellID, np->getManning(ulCellID), model::rasterDatasets::dataValues::kManningCoefficient, pManager->ucRounding);
			//Depth
			ourCartesianDomain->handleInputData(ulCellID, 0.0, model::rasterDatasets::dataValues::kDepth, pManager->ucRounding);
			//VelocityX
			ourCartesianDomain->handleInputData(ulCellID, 0.0, model::rasterDatasets::dataValues::kVelocityX, pManager->ucRounding);
			//VelocityY
			ourCartesianDomain->handleInputData(ulCellID, 0.0, model::rasterDatasets::dataValues::kVelocityY, pManager->ucRounding);
			//Flow States
			ourCartesianDomain->setFlowStatesValue(ulCellID, flowStates);
			//Boundary Condition
			ourCartesianDomain->setBoundaryCondition(ulCellID, 3e-6);
			//Coupling Condition
			ourCartesianDomain->setCouplingCondition(ulCellID, 0.0);
			//dsdt Condition
			ourCartesianDomain->setdsdt(ulCellID, 0.0);

		}
	}

	CDomainManager* pManagerDomains = pManager->getDomainSet();
	pManagerDomains->setSyncMethod(model::syncMethod::kSyncTimestep);
	ourCartesianDomain->setID(pManagerDomains->getDomainCount());	// Should not be needed, but somehow is?

	//Set newly created domain to the model and do logging and checking
	pManagerDomains->domains.push_back(ourCartesianDomain);

	pManagerDomains->logDetails();

	pManager->log->writeLine("The computational engine is now ready.");
	return model::appReturnCodes::kAppSuccess;
}

/*
 *  Read in configuration file and launch a new simulation
 */
int commenceSimulation()
{


	pManager->runModelPrepare();
	pManager->runModelMain();



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
