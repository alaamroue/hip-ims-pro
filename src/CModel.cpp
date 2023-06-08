/*
 * This file is a modified version of the code originally created by Luke S. Smith and Qiuhua Liang.
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
 *  Main model class
 * ------------------------------------------
 *
 */

// Includes
#include <cmath>
#include <math.h>
#include "common.h"
#include "CExecutorControlOpenCL.h"
#include "CDomainCartesian.h"
#include "CScheme.h"
#include "CSchemePromaides.h"
#include "Normalplain.h"

using std::min;
using std::max;

CModel* cModelClass;

/*
 *  Constructor
 */
CModel::CModel(void)
{
	this->log = new CLog();

	this->bAllIdle = false;
	this->bFrictionEffects = false;
	this->bIdle = false;
	this->bRollbackRequired = false;
	this->bSyncReady = false;
	this->bSynchronised = false;
	this->bWaitOnLinks = false;
	this->dCellRate = 0.0;
	this->dCourantNumber = 0.5;
	this->dEarliestTime = 0.0;
	this->dGlobalTimestep = 0.0;
	this->dLastOutputTime = 0.0;
	this->dLastProgressUpdate = 0.0;
	this->dLastSyncTime = 0.0;
	this->dProcessingTime = 0.0;
	this->dTargetTime = 0.0;
	this->dVisualisationTime = 0.0;
	this->selectedDevice = 1;
	this->ucFloatSize = model::floatPrecision::kDouble;
	this->ulCachedWorkgroupSizeX = 0;
	this->ulCachedWorkgroupSizeY = 0;
	this->ulNonCachedWorkgroupSizeX = 0;
	this->ulNonCachedWorkgroupSizeY = 0;


	cModelClass = this;

	this->execController	= NULL;
	this->domain			= NULL;
	this->mpiManager		= NULL;

	this->dCurrentTime		= 0.0;
	this->dSimulationTime	= 0;
	this->dOutputFrequency	= 0;
	this->bDoublePrecision	= true;

	this->pProgressCoords.sX = -1;
	this->pProgressCoords.sY = -1;

	this->ulRealTimeStart = 0;

	this->forcedAbort = false;
	this->ucRounding = 6;
}


/*
 *  Destructor
 */
CModel::~CModel(void)
{
	if ( this->execController != NULL )
		delete this->execController;
	this->log->writeLine("The model engine is completely unloaded.");
	this->log->writeDivide();
	delete this->log;
}

/*
 *  Set the type of executor to use for the model
 */
bool CModel::setExecutor(CExecutorControl* pExecutorControl)
{
	// TODO: Has the value actually changed?

	// TODO: Delete the old executor controller

	this->execController = static_cast<CExecutorControlOpenCL*>(pExecutorControl);

	if ( !this->execController->isReady() )
	{
		this->log->writeError(
			"The executor is not ready. Model cannot continue.",
			model::errorCodes::kLevelFatal
		);
		return false;
	}

	return true;
}

/*
 *  Set the type of executor to use for the model as the default GPU Excutor
 */
bool CModel::setExecutorToDefaultGPU()
{
	CExecutorControl* pExecutor = new CExecutorControlOpenCL();						//Create Executor
	pExecutor->logger = this->log;
	//pExecutor->logPlatforms();
	pExecutor->setDeviceFilter(model::filters::devices::devicesGPU);                //Set Type
	if (!pExecutor->createDevices()) return false;									//Creates Device
	this->setExecutor(pExecutor);

	return true;
}

/*
 *  Returns a pointer to the execution controller currently in use
 */
CExecutorControlOpenCL* CModel::getExecutor( void )
{
	return this->execController;
}

/*
*  Returns a pointer to the MPI manager class
*/
CMPIManager* CModel::getMPIManager(void)
{
	// Pass back the pointer - will be NULL if not using MPI version
	return this->mpiManager;
}

/*
 *  Log the details for the whole simulation
 */
void CModel::logDetails()
{
	unsigned short wColour = model::cli::colourInfoBlock;

	this->log->writeDivide();
	this->log->writeLine( "SIMULATION CONFIGURATION", true, wColour );
	this->log->writeLine( "  Name:               " + this->sModelName, true, wColour );
	//this->log->writeLine( "  Start time:         " + std::string( Util::fromTimestamp( this->ulRealTimeStart, "%d-%b-%Y %H:%M:%S" ) ), true, wColour );
	//this->log->writeLine( "  End time:           " + std::string( Util::fromTimestamp( this->ulRealTimeStart + static_cast<unsigned long>( std::ceil( this->dSimulationTime ) ), "%d-%b-%Y %H:%M:%S" ) ), true, wColour );
	this->log->writeLine( "  Simulation length:  " + Util::secondsToTime( this->dSimulationTime ), true, wColour );
	this->log->writeLine( "  Output frequency:   " + Util::secondsToTime( this->dOutputFrequency ), true, wColour );
	this->log->writeLine( "  Floating-point:     " + (std::string)( this->getFloatPrecision() == model::floatPrecision::kDouble ? "Double-precision" : "Single-precision" ), true, wColour );
	this->log->writeDivide();
}

/*
 *  Execute the model
 */
bool CModel::runModel( void )
{
	this->log->writeLine("Verifying the required data before model run...");
	if (!this->execController || !this->execController->isReady()) { model::doError("The executor is not ready.", model::errorCodes::kLevelModelStop); return false; }
	this->log->writeLine("Verification is complete.");
	this->log->writeDivide();
	this->log->writeLine("Starting a new simulation...");


	this->runModelPrepare();
	this->runModelMain();


	return true;
}

/*
 *  Sets a short name for the model
 */
void	CModel::setName( std::string sName )
{
	this->sModelName = sName;
}

/*
 *  Sets a short name for the model
 */
void	CModel::setDescription( std::string sDescription )
{
	this->sModelDescription = sDescription;
}

/*
 *  Sets the total length of a simulation
 */
void	CModel::setSimulationLength( double dLength )
{
	this->dSimulationTime		= dLength;
}

/*
 *  Gets the total length of a simulation
 */
double	CModel::getSimulationLength()
{
	return this->dSimulationTime;
}

/*
 *  Set the frequency of outputs
 */
void	CModel::setOutputFrequency( double dFrequency )
{
	this->dOutputFrequency = dFrequency;
}

/*
 *  Sets the real world start time

void	CModel::setRealStart( char* cTime, char* cFormat )
{
	this->ulRealTimeStart = Util::toTimestamp( cTime, cFormat );
}

/*
 *  Fetch the real world start time

unsigned long CModel::getRealStart()
{
	return this->ulRealTimeStart;
}
 */
/*
 *  Get the frequency of outputs
 */
double	CModel::getOutputFrequency()
{
	return this->dOutputFrequency;
}


/*
 *  Set floating point precision
 */
void	CModel::setFloatPrecision( unsigned char ucPrecision )
{
	if ( !this->getExecutor()->getDevice()->isDoubleCompatible() )
		ucPrecision = model::floatPrecision::kSingle;

	this->bDoublePrecision = ( ucPrecision == model::floatPrecision::kDouble );
}

/*
 *  Get floating point precision
 */
unsigned char	CModel::getFloatPrecision()
{
	return ( this->bDoublePrecision ? model::floatPrecision::kDouble : model::floatPrecision::kSingle );
}

/*
 *  Write details of where model execution is currently at
 */
void	CModel::logProgress( CBenchmark::sPerformanceMetrics* sTotalMetrics )
{
	char	cTimeLine[70]      = "                                                                    X";
	char	cCellsLine[70]     = "                                                                    X";
	char	cTimeLine2[70]     = "                                                                    X";
	char	cCells[70]         = "                                                                    X";
	char	cProgressLine[70]  = "                                                                    X";
	char	cBatchSizeLine[70] = "                                                                    X";
	char	cProgress[57]      = "                                                      ";
	char	cProgessNumber[7]  = "      ";

	unsigned short wColour	= model::cli::colourInfoBlock;

	double		  dCurrentTime = ( this->dCurrentTime > this->dSimulationTime ? this->dSimulationTime : this->dCurrentTime );
	double		  dProgress = dCurrentTime / this->dSimulationTime;

	// TODO: These next bits will need modifying for when we have multiple domains
	unsigned long long	ulCurrentCellsCalculated		   = 0;
	unsigned int		uiBatchSizeMax = 0, uiBatchSizeMin = 9999;
	double				dSmallestTimestep				   = 9999.0;

	// Get the total number of cells calculated
	ulCurrentCellsCalculated += domain->getScheme()->getCellsCalculated();

	CDomainCartesian::mpiSignalDataProgress pProgress = domain->getDataProgress();

	if (uiBatchSizeMax < pProgress.uiBatchSize)
		uiBatchSizeMax = pProgress.uiBatchSize;
	if (uiBatchSizeMin > pProgress.uiBatchSize)
		uiBatchSizeMin = pProgress.uiBatchSize;
	if (dSmallestTimestep > pProgress.dBatchTimesteps)
		dSmallestTimestep = pProgress.dBatchTimesteps;

	unsigned long ulRate = static_cast<unsigned long>(ulCurrentCellsCalculated / sTotalMetrics->dSeconds);

	// Make a progress bar
	for( unsigned char i = 0; i <= floor( 55.0f * dProgress ); i++ )
		cProgress[ i ] = ( i >= ( floor( 55.0f * dProgress ) - 1 ) ? '>' : '=' );

	// String padding stuff
	sprintf_s( cTimeLine,		70,	" Simulation time:  %-15sLowest timestep: %15s", Util::secondsToTime( dCurrentTime ).c_str(), Util::secondsToTime( dSmallestTimestep ).c_str() );
	sprintf_s( cCells,			70,	"%I64u", ulCurrentCellsCalculated );
	sprintf_s( cCellsLine,		70,	" Cells calculated: %-24s  Rate: %13s/s", cCells, std::to_string( ulRate ).c_str() );
	sprintf_s( cTimeLine2,		70,	" Processing time:  %-16sEst. remaining: %15s", Util::secondsToTime( sTotalMetrics->dSeconds ).c_str(), Util::secondsToTime( min( ( 1.0 - dProgress ) * ( sTotalMetrics->dSeconds / dProgress ), 31536000.0 ) ).c_str() );
	sprintf_s( cBatchSizeLine,	70,	" Batch size:       %-16s                                 ", std::to_string( uiBatchSizeMin ).c_str() );
	sprintf_s( cProgessNumber,	7,	"%.1f%%", dProgress * 100 );
	sprintf_s( cProgressLine,	70, " [%-55s] %7s", cProgress, cProgessNumber );

	this->log->writeDivide();																						// 1
	this->log->writeLine( "                                                                  ", false, wColour );	// 2
	this->log->writeLine( " SIMULATION PROGRESS                                              ", false, wColour );	// 3
	this->log->writeLine( "                                                                  ", false, wColour );	// 4
	this->log->writeLine( std::string( cTimeLine )											  , false, wColour );	// 5
	this->log->writeLine( std::string( cCellsLine )											  , false, wColour );	// 6
	this->log->writeLine( std::string( cTimeLine2 )											  , false, wColour );	// 7
	this->log->writeLine( std::string( cBatchSizeLine )										  , false, wColour );	// 8
	this->log->writeLine( "                                                                  ", false, wColour );	// 9
	this->log->writeLine( std::string( cProgressLine )										  , false, wColour );	// 10
	this->log->writeLine( "                                                                  ", false, wColour );	// 11
	this->log->writeLine( "             +----------+----------------+------------+----------+", false, wColour );	// 12
	this->log->writeLine( "             |  Device  |  Avg.timestep  | Iterations | Bypassed |", false, wColour );	// 12
	this->log->writeLine( "+------------+----------+----------------+------------+----------|", false, wColour );	// 13
	

	char cDomainLine[70] = "                                                                    X";
		
	// TODO: Give this it's proper name...
	std::string sDeviceName = "REMOTE";

	sDeviceName = domain->getDevice()->getDeviceShortName();

	sprintf_s(
		cDomainLine,
		70,
		"| Domain #%-2s | %8s | %14s | %10s | %8s |",
		std::to_string(1).c_str(),
		sDeviceName.c_str(),
		Util::secondsToTime(pProgress.dBatchTimesteps).c_str(),
		std::to_string(pProgress.uiBatchSuccessful).c_str(),
		std::to_string(pProgress.uiBatchSkipped).c_str()
	);

	this->log->writeLine( std::string( cDomainLine ), false, wColour );	// ++

	this->log->writeLine( "+------------+----------+----------------+------------+----------+" , false, wColour);	// 14
	this->log->writeDivide();																						// 15

	this->pProgressCoords = Util::getCursorPosition();
	if (this->dCurrentTime < this->dSimulationTime) 
	{
		this->pProgressCoords.sY = max(0, this->pProgressCoords.sY - (16 + (cl_int) 1));
		Util::setCursorPosition(this->pProgressCoords);
	}
}

/*
 *  Update the visualisation by sending domain data over to the relevant component
 */
void CModel::visualiserUpdate()
{

	if ( this->dCurrentTime >= this->dSimulationTime - 1E-5 || this->forcedAbort )
		return;

}

/*
 *  Memory read should have completed, so provided the simulation isn't over - read it back again
 */
void CL_CALLBACK CModel::visualiserCallback( cl_event clEvent, cl_int iStatus, void * vData )
{
	cModelClass->visualiserUpdate();
	clReleaseEvent( clEvent );
}

/*
*  Prepare for a new simulation, which may follow a failed simulation so states need to be reset.
*/
void	CModel::runModelPrepare()
{

	domain->getScheme()->prepareSimulation();

	bSynchronised		= true;
	bAllIdle			= true;
	dTargetTime			= 0.0;
	dLastSyncTime		= -1.0;
	dLastOutputTime		= 0.0;


}

/*
*  Assess the current state of each domain.
*/
void	CModel::runModelDomainAssess(bool *			bSyncReady,bool *			bIdle)
{
	double dMinTimestep = 0.0;

	if ( ( dMinTimestep == 0.0 || dMinTimestep > domain->getScheme()->getCurrentTimestep() ) &&
			domain->getScheme()->getCurrentTimestep() > 0.0 )
		dMinTimestep = domain->getScheme()->getCurrentTimestep();

	dGlobalTimestep = dMinTimestep;

}


/*
*  Synchronise the whole model across all domains.
*/
void	CModel::runModelUpdateTarget( double dTimeBase )
{
	// Identify the smallest batch size associated timestep
	double dEarliestSyncProposal = this->dSimulationTime;


	// Don't exceed an output interval if required
	if (floor(dEarliestSyncProposal / dOutputFrequency)  > floor(dLastSyncTime / dOutputFrequency))
	{
		dEarliestSyncProposal = (floor(dLastSyncTime / dOutputFrequency) + 1) * dOutputFrequency;
	}

	// Work scheduler within numerical schemes should identify whether this has changed
	// and update the buffer if required only...
	dTargetTime = dEarliestSyncProposal;

}

/*
*  Sets a new Target Time to go to before stopping
*/
void	CModel::setModelUpdateTarget(double newTarget)
{
	dTargetTime = newTarget;

}



/*
*  Process incoming and pending MPI messages etc.
*/
void	CModel::runModelMPI()
{
#ifdef MPI_ON
	this->getMPIManager()->processQueue();
#endif
}

/*
*  Schedule new work in the simulation.
*/
void	CModel::runModelSchedule( bool * bIdle)
{

		//domains->getDomain(0)->getScheme()->forceTimestep(dGlobalTimestep);

		//domains->getDomain(0)->getScheme()->runSimulation(dTargetTime);
}

/*
*  Update UI elements (progress bars etc.)
*/
void	CModel::runModelUI( CBenchmark::sPerformanceMetrics * sTotalMetrics )
{
	dProcessingTime = sTotalMetrics->dSeconds;
	if (sTotalMetrics->dSeconds - dLastProgressUpdate > 0.85)
	{
		this->logProgress(sTotalMetrics);
		dLastProgressUpdate = sTotalMetrics->dSeconds;
	}
}

/*
*  Rollback simulation states to a previous recorded state.
*/
void	CModel::runModelRollback()
{
	if ( !bRollbackRequired ||
		this->forcedAbort ||
		 !bAllIdle )
		return;

	model::doError(
		"Rollback invoked - code not yet ready",
		model::errorCodes::kLevelModelStop
	);
		
	// Now sync'd again and ready to continue
	bRollbackRequired = false;
	bSynchronised = false;

	// Use the data from the last run to work out how long we can run 
	// the batch for. Same function as normal but relative to the last sync time instead.
	this->runModelUpdateTarget(dLastSyncTime);
	this->log->writeLine("Simulation rollback at " + Util::secondsToTime(this->dCurrentTime) + "; revised sync point is " + Util::secondsToTime(dTargetTime) + ".");

	// ---
	// TODO: Do we need to do an MPI reduce here...?
	// ---

	// ---
	// TODO: Notify all nodes of the rollback requirements...?
	// ---

	// Let each domain know the goalposts have proverbially moved
	// Write the last full set of domain data back to the GPU
	dEarliestTime = dLastSyncTime;
	dCurrentTime = dLastSyncTime;

	domain->getScheme()->rollbackSimulation(dLastSyncTime, dTargetTime);

	// Global block across all nodes is required for rollbacks
	domain->getDevice()->blockUntilFinished();
}


/*
 *  Clean things up after the model is complete or aborted
 */
void	CModel::runModelCleanup()
{

	domain->getScheme()->cleanupSimulation();
}

/*
 *  Run the actual simulation, asking each domain and schemes therein in turn etc.
 */
void	CModel::runModelMain()
{
	dCellRate				= 0.0;


	dGlobalTimestep = this->domain->getScheme()->getTimestep();

	//dTargetTime = this->getSimulationLength();
	//dTargetTime = 360000.0;

	log->writeLine("Simulation Started...");
	unsigned char	ucRounding = 4;			// decimal places
	CDomainCartesian* cd = this->domain;
	CSchemePromaides* myProScheme = (CSchemePromaides*) this->domain->getScheme();
	Normalplain* np = new Normalplain(100, 100);
	np->SetBedElevationMountain();
	myProScheme->np = np;

	for (double i = 3600; i <= 3600 * 100; i += 3600)
	{
		//myProScheme->bDownloadLinks = true;
		this->runNext(i);

		//Read simulation results
		this->domain->readDomain();

		//if (i == 3600 * 5 && false) {
		//	std::cout << "Changed" << std::endl;
		//	myProScheme->importLinkZoneData();
//
//
		//}
	}


	
	while ((this->dCurrentTime < dSimulationTime - 1E-5) || !bAllIdle)
	{
		//Has not ended
	}


	log->writeLine("Simulation Ended...");
	// Simulation was aborted?
	if (this->forcedAbort)
	{
		model::doError(
			"Simulation has been aborted",
			model::errorCodes::kLevelModelStop
		);
	}

	// Get the total number of cells calculated
	unsigned long long	ulCurrentCellsCalculated = 0;
	double				dVolume = 0.0;
	ulCurrentCellsCalculated += domain->getScheme()->getCellsCalculated();
	dVolume += abs( domain->getVolume() );

	this->log->writeDivide();

}

void CModel::runNext(double	nextPoint) {
	this->dTargetTime = nextPoint;
	CBenchmark* pBenchmarkAll = new CBenchmark(true);
	CBenchmark::sPerformanceMetrics* sTotalMetrics = pBenchmarkAll->getMetrics();
	CSchemePromaides* myscheme = (CSchemePromaides*) this->domain->getScheme();
	while ((this->dCurrentTime - nextPoint < 0) || !bAllIdle)
	{
		this->dCurrentTime = myscheme->getCurrentTime();

		myscheme->runSimulation(nextPoint, sTotalMetrics->dSeconds);

		sTotalMetrics = pBenchmarkAll->getMetrics();
		this->runModelUI(sTotalMetrics);
	}

}


/*
 *  Set the Courant number
 */
void	CModel::setCourantNumber(double dCourantNumber)
{
	this->dCourantNumber = dCourantNumber;
}

/*
 *  Get the Courant number
 */
double	CModel::getCourantNumber()
{
	return this->dCourantNumber;
}

/*
 *  Enable/disable friction effects
 */
void	CModel::setFrictionStatus(bool bEnabled)
{
	this->bFrictionEffects = bEnabled;
}

/*
 *  Get enabled/disabled for friction
 */
bool	CModel::getFrictionStatus()
{
	return this->bFrictionEffects;
}

/*
 *  Set the cache size
 */
void	CModel::setCachedWorkgroupSize(unsigned char ucSize)
{
	this->ulCachedWorkgroupSizeX = ucSize; this->ulCachedWorkgroupSizeY = ucSize;
}
void	CModel::setCachedWorkgroupSize(unsigned char ucSizeX, unsigned char ucSizeY)
{
	this->ulCachedWorkgroupSizeX = ucSizeX; this->ulCachedWorkgroupSizeY = ucSizeY;
}
void	CModel::setNonCachedWorkgroupSize(unsigned char ucSize)
{
	this->ulNonCachedWorkgroupSizeX = ucSize; this->ulNonCachedWorkgroupSizeY = ucSize;
}
void	CModel::setNonCachedWorkgroupSize(unsigned char ucSizeX, unsigned char ucSizeY)
{
	this->ulNonCachedWorkgroupSizeX = ucSizeX; this->ulNonCachedWorkgroupSizeY = ucSizeY;
}

CLog* CModel::getLogger() {
	return this->log;
}


void CModel::setSelectedDevice(unsigned int id) {
	this->selectedDevice = id;
	this->getExecutor()->selectDevice(id);
}


unsigned int CModel::getSelectedDevice() {
	return this->selectedDevice;
}

void CModel::setDomain(CDomainCartesian* cDomainCartesian) {
	this->domain = cDomainCartesian;
}

CDomainCartesian* CModel::getDomain() {
	return this->domain;
}