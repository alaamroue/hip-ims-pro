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
#include "OpenCL/Executors/CExecutorControlOpenCL.h"
#include "Domain/CDomainManager.h"
#include "Domain/CDomain.h"
#include "Schemes/CScheme.h"
#include "Datasets/CRasterDataset.h"
#include "Schemes/CSchemeGodunov.h"

using std::min;
using std::max;

CModel* cModelClass;

/*
 *  Constructor
 */
CModel::CModel(void)
{
	this->log = new CLog();
	cModelClass = this;

	this->execController	= NULL;
	this->domains			= new CDomainManager();
	this->domains->logger	= this->log;
	this->mpiManager		= NULL;

	this->dCurrentTime		= 0.0;
	this->dSimulationTime	= 0;
	this->dOutputFrequency	= 0;
	this->bDoublePrecision	= true;

	this->pProgressCoords.sX = -1;
	this->pProgressCoords.sY = -1;

	this->ulRealTimeStart = 0;

	this->forcedAbort = false;
}


/*
 *  Destructor
 */
CModel::~CModel(void)
{
	if (this->domains != NULL)
		delete this->domains;
	if ( this->execController != NULL )
		delete this->execController;
	this->log->writeLine("The model engine is completely unloaded.");
	this->log->writeDivide();
	this->log->writeLine("                                                                  ", false);
	this->log->writeLine("                                                                  ", false);
	this->log->writeLine("                                                                  ", false);
	this->log->writeLine("                                                                  ", false);
	this->log->writeLine("                                                                  ", false);
	this->log->writeLine("                                                                  ", false);
	this->log->writeLine("                                                                  ", false);
	this->log->writeLine("                                                                  ", false);
	this->log->writeLine("                                                                  ", false);
	this->log->writeLine("                                                                  ", false);
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
	pExecutor->logPlatforms();
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
 *  Returns a pointer to the domain class
 */
CDomainManager* CModel::getDomainSet( void )
{
	return this->domains;
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
	if (!this->domains || !this->domains->isSetReady()) { model::doError("The domain is not ready.", model::errorCodes::kLevelModelStop); return false; }
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
 *  Write periodical output files to disk
 */
void	CModel::writeOutputs()
{
	this->getDomainSet()->writeOutputs();
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
	for( unsigned int i = 0; i < domainCount; ++i )
	{
		if (domains->isDomainLocal(i))
		{
			// Get the number of cells calculated (for the rate mainly)
			// TODO: Deal with this for MPI...
			ulCurrentCellsCalculated += domains->getDomain(i)->getScheme()->getCellsCalculated();
		}

		CDomainBase::mpiSignalDataProgress pProgress = domains->getDomain(i)->getDataProgress();

		if (uiBatchSizeMax < pProgress.uiBatchSize)
			uiBatchSizeMax = pProgress.uiBatchSize;
		if (uiBatchSizeMin > pProgress.uiBatchSize)
			uiBatchSizeMin = pProgress.uiBatchSize;
		if (dSmallestTimestep > pProgress.dBatchTimesteps)
			dSmallestTimestep = pProgress.dBatchTimesteps;
	}

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
	
	for( unsigned int i = 0; i < domainCount; i++ )
	{
		char cDomainLine[70] = "                                                                    X";
		CDomainBase::mpiSignalDataProgress pProgress = domains->getDomain(i)->getDataProgress();
		
		// TODO: Give this it's proper name...
		std::string sDeviceName = "REMOTE";

		if (domains->isDomainLocal(i))
		{
			sDeviceName = domains->getDomain(i)->getDevice()->getDeviceShortName();
		}

		sprintf_s(
			cDomainLine,
			70,
			"| Domain #%-2s | %8s | %14s | %10s | %8s |",
			std::to_string(i + 1).c_str(),
			sDeviceName.c_str(),
			Util::secondsToTime(pProgress.dBatchTimesteps).c_str(),
			std::to_string(pProgress.uiBatchSuccessful).c_str(),
			std::to_string(pProgress.uiBatchSkipped).c_str()
		);

		this->log->writeLine( std::string( cDomainLine ), false, wColour );	// ++
	}

	this->log->writeLine( "+------------+----------+----------------+------------+----------+" , false, wColour);	// 14
	this->log->writeDivide();																						// 15

	this->pProgressCoords = Util::getCursorPosition();
	if (this->dCurrentTime < this->dSimulationTime) 
	{
		this->pProgressCoords.sY = max(0, this->pProgressCoords.sY - (16 + (cl_int)domainCount));
		Util::setCursorPosition(this->pProgressCoords);
	}
}

/*
 *  Update the visualisation by sending domain data over to the relevant component
 */
void CModel::visualiserUpdate()
{
	CDomain*	pDomain = this->domains->getDomain(0);
	COCLDevice*	pDevice	= this->domains->getDomain(0)->getDevice();

	#ifdef _WINDLL
	this->domains->getDomain(0)->sendAllToRenderer();
	#endif

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
	domainCount = this->getDomainSet()->getDomainCount();

	domains->getDomain(0)->getScheme()->prepareSimulation();

	bSynchronised		= true;
	bAllIdle			= true;
	dTargetTime			= 0.0;
	dLastSyncTime		= -1.0;
	dLastOutputTime		= 0.0;


}

/*
*  Prepare domains for a new simulation.
*/
void	CModel::runModelPrepareDomains()
{
	for (unsigned int i = 0; i < domainCount; ++i)
	{
		if (!domains->isDomainLocal(i))
			continue;

		domains->getDomain(i)->getScheme()->prepareSimulation();
		domains->getDomain(i)->setRollbackLimit();		// Auto calculate from links...

		if (domainCount > 1)
		{
			this->log->writeLine("Domain #" + std::to_string(i + 1) + " has rollback limit of " +
				std::to_string(domains->getDomain(i)->getRollbackLimit()) + " iterations.");
		} else {
			this->log->writeLine("Domain #" + std::to_string(i + 1) + " is not constrained by " +
				"overlapping.");
		}
	}
}

/*
*  Assess the current state of each domain.
*/
void	CModel::runModelDomainAssess(bool *			bSyncReady,bool *			bIdle)
{
	double dMinTimestep = 0.0;

	if ( ( dMinTimestep == 0.0 || dMinTimestep > domains->getDomain(0)->getScheme()->getCurrentTimestep() ) &&
			domains->getDomain(0)->getScheme()->getCurrentTimestep() > 0.0 )
		dMinTimestep = domains->getDomain(0)->getScheme()->getCurrentTimestep();

	dGlobalTimestep = dMinTimestep;

}

/*
 *  Exchange data across domains where necessary.
 */
void	CModel::runModelDomainExchange()
{
	this->log->writeLine( "[DEBUG] Exchanging domain data NOW... (" + Util::secondsToTime( this->dEarliestTime ) + ")" );
	// Swap sync zones over
	for (unsigned int i = 0; i < domainCount; ++i)		// Source domain
	{
		if (domains->isDomainLocal(i))
		{
			domains->getDomain(i)->getScheme()->importLinkZoneData();
			// TODO: Above command does not actually cause import -- next line can be removed?
			domains->getDomain(i)->getDevice()->flushAndSetMarker();		
		}
	}
	
	this->runModelBlockNode();
}

/*
*  Synchronise the whole model across all domains.
*/
void	CModel::runModelUpdateTarget( double dTimeBase )
{
	// Identify the smallest batch size associated timestep
	double dEarliestSyncProposal = this->dSimulationTime;

	
	// Only bother with all this stuff if we actually need to synchronise,
	// otherwise run free, for as long as possible (i.e. until outputs needed)
	if (domainCount > 1 &&
		this->getDomainSet()->getSyncMethod() == model::syncMethod::kSyncForecast)
	{
		for (unsigned int i = 0; i < domainCount; ++i)
		{
			// TODO: How to calculate this for remote domains etc?
			if (domains->isDomainLocal(i))
				dEarliestSyncProposal = min(dEarliestSyncProposal, domains->getDomain(i)->getScheme()->proposeSyncPoint(dCurrentTime));
		}
	}

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
*  Synchronise the whole model across all domains.
*/
void	CModel::runModelSync()
{
	if (this->dTargetTime > this->dCurrentTime)
		return;
		
	// Write outputs if possible
	this->runModelOutputs();
		
	this->setModelUpdateTarget(this->dTargetTime * 10);

	if ( 
			(domainCount > 1 && this->getDomainSet()->getSyncMethod() == model::syncMethod::kSyncForecast ) ||
			( fabs(this->dCurrentTime - dLastOutputTime - this->getOutputFrequency()) < 1E-5 && this->dCurrentTime > dLastOutputTime ) 
		)
	{
		domains->getDomain(0)->getScheme()->saveCurrentState();
	}

	// Let devices finish
	this->runModelBlockNode();
	
	// Exchange domain data
	this->log->writeLine( "[DEBUG] Exchanging domain data NOW... (" + Util::secondsToTime( this->dEarliestTime ) + ")" );
	domains->getDomain(0)->getScheme()->importLinkZoneData();

	// Wait for all nodes and devices
	// TODO: This should become global across all nodes 
	//this->runModelBlockGlobal();
}

/*
*  Synchronise the whole model across all domains at a point in time
*/
void	CModel::runModelSyncUntil(double nextTime)
{
	if (bRollbackRequired ||
		!bSynchronised ||
		!bAllIdle)
		return;

	// No rollback required, thus we know the simulation time can now be increased
	// to match the target we'd defined
	this->dCurrentTime = dEarliestTime;
	dLastSyncTime = this->dCurrentTime;

	// TODO: Review if this is needed? Shouldn't earliest time get updated anyway?
	if (domainCount <= 1)
		this->dCurrentTime = dEarliestTime;

	this->runModelOutputs();

	this->setModelUpdateTarget(nextTime);

	for (unsigned int i = 0; i < domainCount; ++i)
	{
		if (domains->isDomainLocal(i))
		{
			// Update with the new target time
			// Can no longer do this here - may have to wait for MPI to return with the value
			//domains->getDomain(i)->getScheme()->setTargetTime(dTargetTime);

			// Save the current state back to host memory, but only if necessary
			// for either domain sync/rollbacks or to write outputs
			if (
				(domainCount > 1 && this->getDomainSet()->getSyncMethod() == model::syncMethod::kSyncForecast) ||
				(fabs(this->dCurrentTime - dLastOutputTime - this->getOutputFrequency()) < 1E-5 && this->dCurrentTime > dLastOutputTime)
				)
			{
				domains->getDomain(i)->getScheme()->saveCurrentState();
			}
		}
	}

	// Let devices finish
	this->runModelBlockNode();

	// Exchange domain data
	this->runModelDomainExchange();

	// Wait for all nodes and devices
	// TODO: This should become global across all nodes 
	this->runModelBlockNode();
	//this->runModelBlockGlobal();
}

/*
*  Block execution across all domains which reside on this node only
*/
void	CModel::runModelBlockNode()
{
	for (unsigned int i = 0; i < domainCount; i++)
	{
		if (domains->isDomainLocal(i))
			domains->getDomain(i)->getDevice()->blockUntilFinished();
	}
}

/*
 *  Block execution across all domains until every single one is ready
 */
void	CModel::runModelBlockGlobal()
{
	this->runModelBlockNode();
#ifdef MPI_ON
	this->getMPIManager()->asyncBlockOnComm();
#endif
}

/*
 *  Write output files if required.
 */
void	CModel::runModelOutputs()
{
	if ( bRollbackRequired ||
		 !bSynchronised ||
		 !bAllIdle ||
		 !( fabs(this->dCurrentTime - dLastOutputTime - this->getOutputFrequency()) < 1E-5 && this->dCurrentTime > dLastOutputTime) )
		return;

	this->writeOutputs();
	dLastOutputTime = this->dCurrentTime;
	
	for (unsigned int i = 0; i < domainCount; ++i)
	{
		if (domains->isDomainLocal(i))
			domains->getDomain(i)->getScheme()->forceTimeAdvance();
	}
	
#ifdef DEBUG_MPI
	this->log->writeLine( "[DEBUG] Global block until all output files have been written..." );
#endif
	this->runModelBlockGlobal();
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
	for (unsigned int i = 0; i < domainCount; i++)
	{
		if (domains->isDomainLocal(i))
			domains->getDomain(i)->getScheme()->rollbackSimulation(dLastSyncTime, dTargetTime);
	}

	// Global block across all nodes is required for rollbacks
	runModelBlockGlobal();
}


/*
 *  Clean things up after the model is complete or aborted
 */
void	CModel::runModelCleanup()
{
	// Note these will not return until their threads have terminated
	for (unsigned int i = 0; i < domainCount; ++i)
	{
		if (domains->isDomainLocal(i))
			domains->getDomain(i)->getScheme()->cleanupSimulation();
	}
}

/*
 *  Run the actual simulation, asking each domain and schemes therein in turn etc.
 */
void	CModel::runModelMain()
{
	bool*							bSyncReady				= new bool[domainCount];
	bool*							bIdle					= new bool[domainCount];
	double							dCellRate				= 0.0;


	dGlobalTimestep = this->domains->getDomain(0)->getScheme()->getTimestep();

	//dTargetTime = this->getSimulationLength();
	//dTargetTime = 360000.0;

	log->writeLine("Simulation Started...");

	this->runNext(36000.0);
	this->getDomainSet()->getDomain(0)->readDomain();
	this->runNext(36000.0 *2);


	
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
	for( unsigned int i = 0; i < domainCount; ++i )
	{
		if (!domains->isDomainLocal(i))
			continue;

		ulCurrentCellsCalculated += domains->getDomain(i)->getScheme()->getCellsCalculated();
		dVolume += abs( domains->getDomain(i)->getVolume() );
	}

	//this->log->writeLine( "Calculation rate:    " + std::to_string( floor(dCellRate) ) + " cells/sec" );
	//this->log->writeLine( "Final volume:        " + std::to_string( static_cast<int>( dVolume ) ) + "m3" );
	this->log->writeDivide();

	delete[] bSyncReady;
	delete[] bIdle;
}

void CModel::runNext(double	nextPoint) {
	dTargetTime = nextPoint;
	CBenchmark* pBenchmarkAll = new CBenchmark(true);
	CBenchmark::sPerformanceMetrics* sTotalMetrics = pBenchmarkAll->getMetrics();
	CSchemeGodunov* myscheme = (CSchemeGodunov*) this->domains->getDomain(0)->getScheme();
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