/*
 * This file is a based on the code created by Luke S. Smith and Qiuhua Liang.
 * Created by: Alaa Mroue
 * Date of Modification: 04.2023
 *
 * Find the orignal code in OriginalSourceCode.zip
 * OriginalSourceCode.zip: Is a snapshot of the src folder from https://github.com/lukeshope/hipims-ocl based on 1e62acf6b9b480e08646b232361b68c1827d91ae
 */

#include <algorithm>

#include "common.h"
#include "CDomainCartesian.h"
#include "CSchemePromaides.h"

using std::min;
using std::max;
/*
 *  Default constructor
 */

CSchemePromaides::CSchemePromaides(CModel* cmodel)
{
	this->logger = cmodel->log;


	this->bCellStatesSynced = false;
	this->bDownloadLinks = false;
	this->bImportLinks = false;
	this->bOverrideTimestep = false;
	this->bUpdateTargetTime = false;
	this->bUseAlternateKernel = false;
	this->bUseForcedTimeAdvance = false;
	this->dLastSyncTime = 0.0;
	this->np = nullptr;
	this->oclBufferBatchSkipped = NULL;
	this->oclBufferBatchSuccessful = NULL;
	this->oclBufferBatchTimesteps = NULL;
	this->ucSyncMethod = model::syncMethod::kSyncForecast;
	this->ulBoundaryCellGlobalSize = 0;
	this->ulBoundaryCellWorkgroupSize = 0;
	this->ulCachedGlobalSizeX = 0;
	this->ulCachedGlobalSizeY = 0;
	this->ulNonCachedGlobalSizeX = 0;
	this->ulNonCachedGlobalSizeY = 0;
	this->ulReductionGlobalSize = 0;
	this->ulReductionWorkgroupSize = 0;


	// Default setup values
	this->bRunning = false;
	this->bThreadRunning = false;
	this->bThreadTerminated = false;
	this->bDebugOutput = false;
	this->uiDebugCellX = 9999;
	this->uiDebugCellY = 9999;

	this->dCurrentTime = 0.0;
	this->dThresholdVerySmall = 1E-10;
	this->dThresholdQuiteSmall = this->dThresholdVerySmall * 10;
	this->bFrictionInFluxKernel = false;
	this->bIncludeBoundaries = false;
	this->uiTimestepReductionWavefronts = 1000;

	this->ucSolverType = model::solverTypes::kHLLC;
	this->ucConfiguration = model::schemeConfigurations::godunovType::kCacheNone;
	this->ucCacheConstraints = model::cacheConstraints::godunovType::kCacheActualSize;

	this->dBoundaryTimeSeries = NULL;
	this->fBoundaryTimeSeries = NULL;
	this->uiBoundaryParameters = NULL;
	this->ulBoundaryRelationCells = NULL;
	this->uiBoundaryRelationSeries = NULL;

	// Default null values for OpenCL objects
	oclModel = NULL;
	oclKernelFullTimestep = NULL;
	oclKernelBoundary = NULL;
	oclKernelFriction = NULL;
	oclKernelTimestepReduction = NULL;
	oclKernelTimeAdvance = NULL;
	oclKernelResetCounters = NULL;
	oclKernelTimestepUpdate = NULL;
	oclBufferCellStates = NULL;
	oclBufferCellStatesAlt = NULL;
	oclBufferCellManning = NULL;
	oclBufferCellFlowStates = NULL;
	oclBufferBoundCoup = NULL;
	oclBufferdsdt = NULL;
	oclBufferReadN = NULL;
	oclBufferReadE = NULL;
	oclBufferWriteN = NULL;
	oclBufferWriteE = NULL;

	oclBufferCellBed = NULL;
	oclBufferTimestep = NULL;
	oclBufferTimestepReduction = NULL;
	oclBufferTime = NULL;
	oclBufferTimeTarget = NULL;
	oclBufferTimeHydrological = NULL;

	if (this->bDebugOutput)
		model::doError("Debug mode is enabled!", model::errorCodes::kLevelWarning);

	//Set Variables Based on CModel
	this->cExecutorControlOpenCL = cmodel->getExecutor();
	this->dCourantNumber = cmodel->getCourantNumber();
	this->ulCachedWorkgroupSizeX = cmodel->ulCachedWorkgroupSizeX;
	this->ulCachedWorkgroupSizeY = cmodel->ulCachedWorkgroupSizeX;
	this->ulNonCachedWorkgroupSizeX = cmodel->ulNonCachedWorkgroupSizeX;
	this->ulNonCachedWorkgroupSizeY = cmodel->ulNonCachedWorkgroupSizeY;
	this->floatPrecision = cmodel->getFloatPrecision();
	this->simulationLength = cmodel->getSimulationLength();
	this->outputFrequency = cmodel->getOutputFrequency();
	this->syncMethod = model::syncMethod::kSyncForecast;
	this->syncBatchSpares = 0;


	//logger->writeLine("Populated scheme with default settings.");
}

/*
 *  Destructor
 */
CSchemePromaides::~CSchemePromaides(void)
{
	this->releaseResources();
	logger->writeLine("The Godunov scheme class was unloaded from memory.");
}

/*
 *  Read in settings from the XML configuration file for this scheme
 */

/*
 *  Log the details and properties of this scheme instance.
 */
void CSchemePromaides::logDetails()
{
	logger->writeDivide();
	unsigned short wColour = model::cli::colourInfoBlock;

	logger->writeLine("Promaides SCHEME", true, wColour);
	logger->writeLine("  Timestep mode:      " + (std::string)(this->bDynamicTimestep ? "Dynamic" : "Fixed"), true, wColour);
	logger->writeLine("  Courant number:     " + (std::string)(this->bDynamicTimestep ? std::to_string(this->dCourantNumber) : "N/A"), true, wColour);
	logger->writeLine("  Initial timestep:   " + Util::secondsToTime(this->dTimestep), true, wColour);
	logger->writeLine("  Data reduction:     " + std::to_string(this->uiTimestepReductionWavefronts) + " divisions", true, wColour);
	logger->writeLine("  Kernel queue mode:  " + (std::string)(this->bAutomaticQueue ? "Automatic" : "Fixed size"), true, wColour);
	logger->writeLine((std::string)(this->bAutomaticQueue ? "  Initial queue:      " : "  Fixed queue:        ") + std::to_string(this->uiQueueAdditionSize) + " iteration(s)", true, wColour);
	logger->writeLine("  Debug output:       " + (std::string)(this->bDebugOutput ? "Enabled" : "Disabled"), true, wColour);

	logger->writeDivide();
}

/*
 *  Run all preparation steps
 */
void CSchemePromaides::prepareAll()
{
	logger->writeLine("Starting to prepare program for Godunov-type scheme.");

	this->releaseResources();

	oclModel = new COCLProgram(
		cExecutorControlOpenCL,
		this->pDomain->getDevice()
	);
	oclModel->logger = logger;

	// Run-time tracking values
	this->ulCurrentCellsCalculated = 0;
	this->dCurrentTimestep = this->dTimestep;
	this->dCurrentTime = 0;

	// Forcing single precision?
	this->oclModel->setForcedSinglePrecision(this->floatPrecision == model::floatPrecision::kSingle);

	// OpenCL elements
	if (!this->prepare1OExecDimensions())
	{
		model::doError(
			"Failed to dimension task. Cannot continue.",
			model::errorCodes::kLevelModelStop
		);
		this->releaseResources();
		return;
	}

	if (!this->prepare1OConstants())
	{
		model::doError(
			"Failed to allocate constants. Cannot continue.",
			model::errorCodes::kLevelModelStop
		);
		this->releaseResources();
		return;
	}

	if (!this->prepareCode())
	{
		model::doError(
			"Failed to prepare model codebase. Cannot continue.",
			model::errorCodes::kLevelModelStop
		);
		this->releaseResources();
		return;
	}

	if (!this->prepare1OMemory())
	{
		model::doError(
			"Failed to create memory buffers. Cannot continue.",
			model::errorCodes::kLevelModelStop
		);
		this->releaseResources();
		return;
	}

	if (!this->prepareGeneralKernels())
	{
		model::doError(
			"Failed to prepare general kernels. Cannot continue.",
			model::errorCodes::kLevelModelStop
		);
		this->releaseResources();
		return;
	}

	if (!this->prepare1OKernels())
	{
		model::doError(
			"Failed to prepare kernels. Cannot continue.",
			model::errorCodes::kLevelModelStop
		);
		this->releaseResources();
		return;
	}

	if (!this->prepareBoundaries())
	{
		model::doError(
			"Failed to prepare boundaries. Cannot continue.",
			model::errorCodes::kLevelModelStop
		);
		this->releaseResources();
		return;
	}

	this->logDetails();
	this->bReady = true;
}

/*
 *  Concatenate together the code for the different elements required
 */
bool CSchemePromaides::prepareCode()
{
	bool bReturnState = true;

	oclModel->appendCodeFromResource("CLDomainCartesian_H");
	oclModel->appendCodeFromResource("CLFriction_H");
	oclModel->appendCodeFromResource("CLSolverHLLC_H");
	oclModel->appendCodeFromResource("CLDynamicTimestep_H");
	oclModel->appendCodeFromResource("CLSchemePromaides_H");
	oclModel->appendCodeFromResource("CLBoundaries_H");

	oclModel->appendCodeFromResource("CLDomainCartesian_C");
	oclModel->appendCodeFromResource("CLFriction_C");
	oclModel->appendCodeFromResource("CLSolverHLLC_C");
	oclModel->appendCodeFromResource("CLDynamicTimestep_C");
	oclModel->appendCodeFromResource("CLSchemePromaides_C");
	oclModel->appendCodeFromResource("CLBoundaries_C");

	bReturnState = oclModel->compileProgram();

	return bReturnState;
}

/*
 *  Create boundary data arrays etc.
 */
bool CSchemePromaides::prepareBoundaries()
{
	bool						bReturnState = true;
	CExecutorControlOpenCL* pExecutor = cExecutorControlOpenCL;
	CDomainCartesian* pDomain = static_cast<CDomainCartesian*>(this->pDomain);
	COCLDevice* pDevice = pExecutor->getDevice();


	oclKernelBoundary = oclModel->getKernel("bdy_Promaides");
	oclKernelBoundary->setGlobalSize((cl_ulong) ceil(pDomain->getCols() / 8.0) * 8, (cl_ulong) ceil(pDomain->getRows() / 8.0) * 8);
	oclKernelBoundary->setGroupSize(8, 8);

	COCLBuffer* aryArgsBdy[] = { oclBufferBoundCoup,oclBufferTimestep,oclBufferCellStates, oclBufferCellBed, oclBufferReadN,oclBufferReadE, oclBufferdsdt};
	
	oclKernelBoundary->assignArguments(aryArgsBdy);

	return bReturnState;
}

/*
 *  Set the dry cell threshold depth
 */
void	CSchemePromaides::setDryThreshold(double dThresholdDepth)
{
	this->dThresholdVerySmall = dThresholdDepth;
	this->dThresholdQuiteSmall = dThresholdDepth * 10;
}

/*
 *  Get the dry cell threshold depth
 */
double	CSchemePromaides::getDryThreshold()
{
	return this->dThresholdVerySmall;
}

/*
 *  Set number of wavefronts used in reductions
 */
void	CSchemePromaides::setReductionWavefronts(unsigned int uiWavefronts)
{
	this->uiTimestepReductionWavefronts = uiWavefronts;
}

/*
 *  Get number of wavefronts used in reductions
 */
unsigned int	CSchemePromaides::getReductionWavefronts()
{
	return this->uiTimestepReductionWavefronts;
}

/*
 *  Set the Riemann solver to use
 */
void	CSchemePromaides::setRiemannSolver(unsigned char ucRiemannSolver)
{
	this->ucSolverType = ucRiemannSolver;
}

/*
 *  Get the Riemann solver in use
 */
unsigned char	CSchemePromaides::getRiemannSolver()
{
	return this->ucSolverType;
}

/*
 *  Set the cache configuration to use
 */
void	CSchemePromaides::setCacheMode(unsigned char ucCacheMode)
{
	this->ucConfiguration = ucCacheMode;
}

/*
 *  Get the cache configuration in use
 */
unsigned char	CSchemePromaides::getCacheMode()
{
	return this->ucConfiguration;
}

/*
 *  Set the cache size
 */
void	CSchemePromaides::setCachedWorkgroupSize(unsigned char ucSize)
{
	this->ulCachedWorkgroupSizeX = ucSize; this->ulCachedWorkgroupSizeY = ucSize;
}
void	CSchemePromaides::setCachedWorkgroupSize(unsigned char ucSizeX, unsigned char ucSizeY)
{
	this->ulCachedWorkgroupSizeX = ucSizeX; this->ulCachedWorkgroupSizeY = ucSizeY;
}
void	CSchemePromaides::setNonCachedWorkgroupSize(unsigned char ucSize)
{
	this->ulNonCachedWorkgroupSizeX = ucSize; this->ulNonCachedWorkgroupSizeY = ucSize;
}
void	CSchemePromaides::setNonCachedWorkgroupSize(unsigned char ucSizeX, unsigned char ucSizeY)
{
	this->ulNonCachedWorkgroupSizeX = ucSizeX; this->ulNonCachedWorkgroupSizeY = ucSizeY;
}

/*
 *  Set the cache constraints
 */
void	CSchemePromaides::setCacheConstraints(unsigned char ucCacheConstraints)
{
	this->ucCacheConstraints = ucCacheConstraints;
}

/*
 *  Get the cache constraints
 */
unsigned char	CSchemePromaides::getCacheConstraints()
{
	return this->ucCacheConstraints;
}

/*
 *  Calculate the dimensions for executing the problems (e.g. reduction glob/local sizes)
 */
bool CSchemePromaides::prepare1OExecDimensions()
{
	bool						bReturnState = true;
	CExecutorControlOpenCL* pExecutor = cExecutorControlOpenCL;
	COCLDevice* pDevice = pExecutor->getDevice();
	CDomainCartesian* pDomain = static_cast<CDomainCartesian*>(this->pDomain);

	// --
	// Maximum permissible work-group dimensions for this device
	// --

	cl_ulong	ulConstraintWGTotal = (cl_ulong)floor(sqrt(static_cast<double> (pDevice->clDeviceMaxWorkGroupSize)));
	cl_ulong	ulConstraintWGDim = min(pDevice->clDeviceMaxWorkItemSizes[0], pDevice->clDeviceMaxWorkItemSizes[1]);
	cl_ulong	ulConstraintWG = min(ulConstraintWGDim, ulConstraintWGTotal);

	// --
	// Main scheme kernels with/without caching (2D)
	// --

	if (this->ulNonCachedWorkgroupSizeX == 0)
		ulNonCachedWorkgroupSizeX = ulConstraintWG;
	if (this->ulNonCachedWorkgroupSizeY == 0)
		ulNonCachedWorkgroupSizeY = ulConstraintWG;

	ulNonCachedGlobalSizeX = pDomain->getCols();
	ulNonCachedGlobalSizeY = pDomain->getRows();

	if (this->ulCachedWorkgroupSizeX == 0)
		ulCachedWorkgroupSizeX = ulConstraintWG +
		(this->ucCacheConstraints == 12 ? -1 : 0);
	if (this->ulCachedWorkgroupSizeY == 0)
		ulCachedWorkgroupSizeY = ulConstraintWG;

	ulCachedGlobalSizeX = static_cast<unsigned long>(ceil(pDomain->getCols() *
		(this->ucConfiguration == model::schemeConfigurations::godunovType::kCacheEnabled ? static_cast<double>(ulCachedWorkgroupSizeX) / static_cast<double>(ulCachedWorkgroupSizeX - 2) : 1.0)));
	ulCachedGlobalSizeY = static_cast<unsigned long>(ceil(pDomain->getRows() *
		(this->ucConfiguration == model::schemeConfigurations::godunovType::kCacheEnabled ? static_cast<double>(ulCachedWorkgroupSizeY) / static_cast<double>(ulCachedWorkgroupSizeY - 2) : 1.0)));

	// --
	// Timestep reduction (2D)
	// --

	// TODO: May need to make this configurable?!
	ulReductionWorkgroupSize = min(static_cast<size_t>(512), pDevice->clDeviceMaxWorkGroupSize);
	//ulReductionWorkgroupSize = pDevice->clDeviceMaxWorkGroupSize / 2;
	ulReductionGlobalSize = static_cast<unsigned long>(ceil((static_cast<double>(pDomain->getCellCount()) / this->uiTimestepReductionWavefronts) / ulReductionWorkgroupSize) * ulReductionWorkgroupSize);

	return bReturnState;
}

/*
 *  Allocate constants using the settings herein
 */
bool CSchemePromaides::prepare1OConstants()
{
	CDomainCartesian* pDomain = static_cast<CDomainCartesian*>(this->pDomain);

	// --
	// Dry cell threshold depths
	// --
	//TODO: ALaa fix this asap
	oclModel->registerConstant("VERY_SMALL", [&]() { std::ostringstream oss; oss.precision(std::numeric_limits<double>::max_digits10); oss << this->dThresholdVerySmall; return oss.str(); }());
	oclModel->registerConstant("QUITE_SMALL", [&]() { std::ostringstream oss; oss.precision(std::numeric_limits<double>::max_digits10); oss << this->dThresholdQuiteSmall; return oss.str(); }());

	// --
	// Debug mode 
	// --

	if (this->bDebugOutput)
	{
		oclModel->registerConstant("DEBUG_OUTPUT", "1");
		oclModel->registerConstant("DEBUG_CELLX", std::to_string(this->uiDebugCellX));
		oclModel->registerConstant("DEBUG_CELLY", std::to_string(this->uiDebugCellY));
	}
	else {
		oclModel->removeConstant("DEBUG_OUTPUT");
		oclModel->removeConstant("DEBUG_CELLX");
		oclModel->removeConstant("DEBUG_CELLY");
	}

	// --
	// Work-group size requirements
	// --

	oclModel->registerConstant(
			"REQD_WG_SIZE_FULL_TS",
			"__attribute__((reqd_work_group_size(" + std::to_string(this->ulNonCachedWorkgroupSizeX) + ", " + std::to_string(this->ulNonCachedWorkgroupSizeY) + ", 1)))"
		);


	oclModel->registerConstant(
		"REQD_WG_SIZE_LINE",
		"__attribute__((reqd_work_group_size(" + std::to_string(this->ulReductionWorkgroupSize) + ", 1, 1)))"
	);

	// Promaides Constants
	oclModel->registerConstant("Cgg", "9.8066");
	oclModel->registerConstant("Cfacweir", "2.95245");


	// --
	// CFL/fixed timestep
	// --

	if (this->bDynamicTimestep)
	{
		oclModel->registerConstant("TIMESTEP_DYNAMIC", "1");
		oclModel->removeConstant("TIMESTEP_FIXED");
	}
	else {
		oclModel->registerConstant("TIMESTEP_FIXED", std::to_string(this->dTimestep));
		oclModel->removeConstant("TIMESTEP_DYNAMIC");
	}

	// --
	// Timestep reduction and simulation parameters
	// --
	oclModel->registerConstant("TIMESTEP_WORKERS", std::to_string(this->ulReductionGlobalSize));
	oclModel->registerConstant("TIMESTEP_GROUPSIZE", std::to_string(this->ulReductionWorkgroupSize));
	oclModel->registerConstant("SCHEME_ENDTIME", std::to_string(this->simulationLength));
	oclModel->registerConstant("SCHEME_OUTPUTTIME", std::to_string(this->outputFrequency));
	oclModel->registerConstant("COURANT_NUMBER", std::to_string(this->dCourantNumber));

	// --
	// Domain details (size, resolution, etc.)
	// --

	double	dResolution;
	pDomain->getCellResolution(&dResolution);

	oclModel->registerConstant("DOMAIN_CELLCOUNT", std::to_string(pDomain->getCellCount()));
	oclModel->registerConstant("DOMAIN_COLS", std::to_string(pDomain->getCols()));
	oclModel->registerConstant("DOMAIN_ROWS", std::to_string(pDomain->getRows()));
	oclModel->registerConstant("DOMAIN_DELTAX", std::to_string(dResolution));
	oclModel->registerConstant("DOMAIN_DELTAY", std::to_string(dResolution));

	return true;
}

/*
 *  Allocate memory for everything that isn't direct domain information (i.e. temporary/scheme data)
 */
bool CSchemePromaides::prepare1OMemory()
{
	bool						bReturnState = true;
	CExecutorControlOpenCL* pExecutor = cExecutorControlOpenCL;
	CDomainCartesian* pDomain = this->pDomain;
	COCLDevice* pDevice = pExecutor->getDevice();

	unsigned char ucFloatSize = (this->floatPrecision == model::floatPrecision::kSingle ? sizeof(cl_float) : sizeof(cl_double));

	// --
	// Batch tracking data
	// --

	oclBufferBatchTimesteps = new COCLBuffer("Batch timesteps cumulative", oclModel, false, true, ucFloatSize, true, logger);
	oclBufferBatchSuccessful = new COCLBuffer("Batch successful iterations", oclModel, false, true, sizeof(cl_uint), true, logger);
	oclBufferBatchSkipped = new COCLBuffer("Batch skipped iterations", oclModel, false, true, sizeof(cl_uint), true, logger);

	if (this->floatPrecision == model::floatPrecision::kSingle)
	{
		*(oclBufferBatchTimesteps->getHostBlock<float*>()) = 0.0f;
	}
	else {
		*(oclBufferBatchTimesteps->getHostBlock<double*>()) = 0.0;
	}
	*(oclBufferBatchSuccessful->getHostBlock<cl_uint*>()) = 0;
	*(oclBufferBatchSkipped->getHostBlock<cl_uint*>()) = 0;

	oclBufferBatchTimesteps->createBuffer();
	oclBufferBatchSuccessful->createBuffer();
	oclBufferBatchSkipped->createBuffer();

	// --
	// Domain and cell state data
	// --

	void* pCellStates = NULL, * pBedElevations = NULL, * pManningValues = NULL, * pFlowStateValues = NULL, * pBoundCoup = NULL, * pDsDt = NULL;
	pDomain->createStoreBuffers(
		&pCellStates,
		&pBedElevations,
		&pManningValues,
		&pFlowStateValues,
		&pBoundCoup,
		&pDsDt,
		ucFloatSize
	);

	oclBufferCellStates = new COCLBuffer("Cell states", oclModel, false, true);
	oclBufferCellStatesAlt = new COCLBuffer("Cell states (alternate)", oclModel, false, true);
	oclBufferWriteN = new COCLBuffer("Cell states (alternate)", oclModel, false, true);
	oclBufferWriteE = new COCLBuffer("Cell states (alternate)", oclModel, false, true);
	oclBufferReadN = new COCLBuffer("Cell states (alternate)", oclModel, false, true);
	oclBufferReadE = new COCLBuffer("Cell states (alternate)", oclModel, false, true);
	oclBufferCellManning = new COCLBuffer("Manning coefficients", oclModel, true, true);
	oclBufferCellFlowStates = new COCLBuffer("Flow Conditions", oclModel, true, true);
	oclBufferBoundCoup = new COCLBuffer("Boundary and Coupling Conditions", oclModel, false, true);
	oclBufferdsdt = new COCLBuffer("dsdt variable from Promaides", oclModel, false, true);
	oclBufferCellBed = new COCLBuffer("Bed elevations", oclModel, true, true);

	oclBufferCellStates->logger = logger;
	oclBufferCellStatesAlt->logger = logger;
	oclBufferCellManning->logger = logger;
	oclBufferCellFlowStates->logger = logger;
	oclBufferBoundCoup->logger = logger;
	oclBufferdsdt->logger = logger;
	oclBufferWriteN->logger = logger;
	oclBufferWriteE->logger = logger;
	oclBufferReadN->logger = logger;
	oclBufferReadE->logger = logger;
	oclBufferCellBed->logger = logger;


	oclBufferCellStates->setPointer(pCellStates, ucFloatSize * 4 * pDomain->getCellCount());
	oclBufferCellStatesAlt->setPointer(pCellStates, ucFloatSize * 4 * pDomain->getCellCount());
	oclBufferCellManning->setPointer(pManningValues, ucFloatSize * pDomain->getCellCount());
	oclBufferCellFlowStates->setPointer(pFlowStateValues, sizeof(model::FlowStates) * pDomain->getCellCount());
	oclBufferBoundCoup->setPointer(pBoundCoup, ucFloatSize * 2 * pDomain->getCellCount());
	oclBufferdsdt->setPointer(pDsDt, ucFloatSize * pDomain->getCellCount());

	oclBufferWriteN->setPointer(pCellStates, ucFloatSize * 4 * pDomain->getCellCount());
	oclBufferWriteE->setPointer(pCellStates, ucFloatSize * 4 * pDomain->getCellCount());
	oclBufferReadN->setPointer(pCellStates, ucFloatSize * 4 * pDomain->getCellCount());
	oclBufferReadE->setPointer(pCellStates, ucFloatSize * 4 * pDomain->getCellCount());

	oclBufferCellBed->setPointer(pBedElevations, ucFloatSize * pDomain->getCellCount());


	oclBufferCellStates->createBuffer();
	oclBufferCellStatesAlt->createBuffer();
	oclBufferCellManning->createBuffer();
	oclBufferCellFlowStates->createBuffer();
	oclBufferBoundCoup->createBuffer();
	oclBufferdsdt->createBuffer();
	oclBufferWriteN->createBuffer();
	oclBufferWriteE->createBuffer();
	oclBufferReadN->createBuffer();
	oclBufferReadE->createBuffer();
	oclBufferCellBed->createBuffer();

	// --
	// Timesteps and current simulation time
	// --

	oclBufferTimestep = new COCLBuffer("Timestep", oclModel, false, true, ucFloatSize, true, logger);
	oclBufferTime = new COCLBuffer("Time", oclModel, false, true, ucFloatSize, true, logger);
	oclBufferTimeTarget = new COCLBuffer("Target time (sync)", oclModel, false, true, ucFloatSize, true, logger);
	oclBufferTimeHydrological = new COCLBuffer("Time (hydrological)", oclModel, false, true, ucFloatSize, true, logger);

	// We duplicate the time and timestep variables if we're using single-precision so we have copies in both formats
	if (this->floatPrecision == model::floatPrecision::kSingle)
	{
		*(oclBufferTime->getHostBlock<float*>()) = static_cast<cl_float>(this->dCurrentTime);
		*(oclBufferTimestep->getHostBlock<float*>()) = static_cast<cl_float>(this->dCurrentTimestep);
		*(oclBufferTimeHydrological->getHostBlock<float*>()) = 0.0f;
		*(oclBufferTimeTarget->getHostBlock<float*>()) = 0.0f;
	}
	else {
		*(oclBufferTime->getHostBlock<double*>()) = this->dCurrentTime;
		*(oclBufferTimestep->getHostBlock<double*>()) = this->dCurrentTimestep;
		*(oclBufferTimeHydrological->getHostBlock<double*>()) = 0.0;
		*(oclBufferTimeTarget->getHostBlock<double*>()) = 0.0;
	}

	oclBufferTimestep->createBuffer();
	oclBufferTime->createBuffer();
	oclBufferTimeHydrological->createBuffer();
	oclBufferTimeTarget->createBuffer();

	// --
	// Timestep reduction global array
	// --

	oclBufferTimestepReduction = new COCLBuffer("Timestep reduction scratch", oclModel, false, true, this->ulReductionGlobalSize * ucFloatSize, true, logger);
	oclBufferTimestepReduction->createBuffer();

	// TODO: Check buffers were created successfully before returning a positive response

	// VISUALISER STUFF
	// TODO: Make this a bit better, put it somewhere else, etc.
	oclBufferCellStates->setCallbackRead(CModel::visualiserCallback);

	return bReturnState;
}

/*
 *  Create general kernels used by numerous schemes with the compiled program
 */
bool CSchemePromaides::prepareGeneralKernels()
{
	bool						bReturnState = true;
	CExecutorControlOpenCL* pExecutor = cExecutorControlOpenCL;
	CDomainCartesian* pDomain = this->pDomain;
	COCLDevice* pDevice = pExecutor->getDevice();

	// --
	// Timestep and simulation advancing
	// --

	oclKernelTimeAdvance = oclModel->getKernel("tst_Advance_Normal");
	oclKernelResetCounters = oclModel->getKernel("tst_ResetCounters");
	oclKernelTimestepReduction = oclModel->getKernel("tst_Reduce");
	oclKernelTimestepUpdate = oclModel->getKernel("tst_UpdateTimestep");

	oclKernelTimeAdvance->setGroupSize(1, 1, 1);
	oclKernelTimeAdvance->setGlobalSize(1, 1, 1);
	oclKernelTimestepUpdate->setGroupSize(1, 1, 1);
	oclKernelTimestepUpdate->setGlobalSize(1, 1, 1);
	oclKernelResetCounters->setGroupSize(1, 1, 1);
	oclKernelResetCounters->setGlobalSize(1, 1, 1);
	oclKernelTimestepReduction->setGroupSize(this->ulReductionWorkgroupSize);
	oclKernelTimestepReduction->setGlobalSize(this->ulReductionGlobalSize);

	COCLBuffer* aryArgsTimeAdvance[] = { oclBufferTime, oclBufferTimestep, oclBufferTimeHydrological, oclBufferTimestepReduction, oclBufferCellStates, oclBufferCellBed, oclBufferTimeTarget, oclBufferBatchTimesteps, oclBufferBatchSuccessful, oclBufferBatchSkipped };
	COCLBuffer* aryArgsTimestepUpdate[] = { oclBufferTime, oclBufferTimestep, oclBufferTimestepReduction, oclBufferTimeTarget, oclBufferBatchTimesteps };
	COCLBuffer* aryArgsTimeReduction[] = { oclBufferCellStates, oclBufferCellBed, oclBufferTimestepReduction };
	COCLBuffer* aryArgsResetCounters[] = { oclBufferBatchTimesteps, oclBufferBatchSuccessful, oclBufferBatchSkipped };

	oclKernelTimeAdvance->assignArguments(aryArgsTimeAdvance);
	oclKernelResetCounters->assignArguments(aryArgsResetCounters);
	oclKernelTimestepReduction->assignArguments(aryArgsTimeReduction);
	oclKernelTimestepUpdate->assignArguments(aryArgsTimestepUpdate);

	// --
	// Boundaries and friction etc.
	// --

	oclKernelFriction = oclModel->getKernel("per_Friction");
	oclKernelFriction->setGroupSize(this->ulNonCachedWorkgroupSizeX, this->ulNonCachedWorkgroupSizeY);
	oclKernelFriction->setGlobalSize(this->ulNonCachedGlobalSizeX, this->ulNonCachedGlobalSizeY);

	COCLBuffer* aryArgsFriction[] = { oclBufferTimestep, oclBufferCellStates, oclBufferCellBed, oclBufferCellManning, oclBufferTime };
	oclKernelFriction->assignArguments(aryArgsFriction);

	return bReturnState;
}

/*
 *  Create kernels using the compiled program
 */
bool CSchemePromaides::prepare1OKernels()
{
	bool						bReturnState = true;
	CExecutorControlOpenCL* pExecutor = cExecutorControlOpenCL;
	CDomainCartesian* pDomain = this->pDomain;
	COCLDevice* pDevice = pExecutor->getDevice();

	oclKernelFullTimestep = oclModel->getKernel("gts_cacheDisabled");
	oclKernelFullTimestep->setGroupSize(this->ulNonCachedWorkgroupSizeX, this->ulNonCachedWorkgroupSizeY);
	oclKernelFullTimestep->setGlobalSize(this->ulNonCachedGlobalSizeX, this->ulNonCachedGlobalSizeY);
	COCLBuffer* aryArgsFullTimestep[] = { oclBufferTimestep, oclBufferCellBed, oclBufferCellStates, oclBufferCellStatesAlt, oclBufferCellManning, oclBufferCellFlowStates, oclBufferBoundCoup, oclBufferdsdt,
	oclBufferReadN,oclBufferReadE, oclBufferWriteN,oclBufferWriteE };
	oclKernelFullTimestep->assignArguments(aryArgsFullTimestep);
	return bReturnState;
}

/*
 *  Release all OpenCL resources consumed using the OpenCL methods
 */
void CSchemePromaides::releaseResources()
{
	this->bReady = false;

	logger->writeLine("Releasing scheme resources held for OpenCL.");

	this->release1OResources();
}

/*
 *  Release all OpenCL resources consumed using the OpenCL methods
 */
void CSchemePromaides::release1OResources()
{
	this->bReady = false;

	logger->writeLine("Releasing 1st-order scheme resources held for OpenCL.");

	if (this->oclModel != NULL)								delete oclModel;
	if (this->oclKernelFullTimestep != NULL)				delete oclKernelFullTimestep;
	if (this->oclKernelBoundary != NULL)				delete oclKernelBoundary;
	if (this->oclKernelFriction != NULL)					delete oclKernelFriction;
	if (this->oclKernelTimestepReduction != NULL)			delete oclKernelTimestepReduction;
	if (this->oclKernelTimeAdvance != NULL)					delete oclKernelTimeAdvance;
	if (this->oclKernelTimestepUpdate != NULL)				delete oclKernelTimestepUpdate;
	if (this->oclKernelResetCounters != NULL)				delete oclKernelResetCounters;
	if (this->oclBufferCellStates != NULL)					delete oclBufferCellStates;
	if (this->oclBufferCellStatesAlt != NULL)				delete oclBufferCellStatesAlt;
	if (this->oclBufferCellManning != NULL)				delete oclBufferCellManning;
	if (this->oclBufferCellFlowStates != NULL)				delete oclBufferCellFlowStates;
	if (this->oclBufferBoundCoup != NULL)				delete oclBufferBoundCoup;
	if (this->oclBufferdsdt != NULL)				delete oclBufferdsdt;
	if (this->oclBufferReadN  != NULL)				delete oclBufferReadN;
	if (this->oclBufferReadE  != NULL)				delete oclBufferReadE;
	if (this->oclBufferWriteN != NULL)				delete oclBufferWriteN;
	if (this->oclBufferWriteE != NULL)				delete oclBufferWriteE;

	if (this->oclBufferCellBed != NULL)					delete oclBufferCellBed;
	if (this->oclBufferTimestep != NULL)					delete oclBufferTimestep;
	if (this->oclBufferTimestepReduction != NULL)			delete oclBufferTimestepReduction;
	if (this->oclBufferTime != NULL)						delete oclBufferTime;
	if (this->oclBufferTimeTarget != NULL)					delete oclBufferTimeTarget;
	if (this->oclBufferTimeHydrological != NULL)			delete oclBufferTimeHydrological;

	oclModel = NULL;
	oclKernelFullTimestep = NULL;
	oclKernelBoundary = NULL;
	oclKernelFriction = NULL;
	oclKernelTimestepReduction = NULL;
	oclKernelTimeAdvance = NULL;
	oclKernelResetCounters = NULL;
	oclKernelTimestepUpdate = NULL;
	oclBufferCellStates = NULL;
	oclBufferCellStatesAlt = NULL;
	oclBufferCellManning = NULL;
	oclBufferCellFlowStates = NULL;
	oclBufferBoundCoup = NULL;
	oclBufferdsdt = NULL;
	oclBufferReadN = NULL;
	oclBufferReadE = NULL;
	oclBufferWriteN = NULL;
	oclBufferWriteE = NULL;
	oclBufferCellBed = NULL;
	oclBufferTimestep = NULL;
	oclBufferTimestepReduction = NULL;
	oclBufferTime = NULL;
	oclBufferTimeTarget = NULL;
	oclBufferTimeHydrological = NULL;

	if (this->bIncludeBoundaries)
	{
		delete[] this->dBoundaryTimeSeries;
		delete[] this->fBoundaryTimeSeries;
		delete[] this->uiBoundaryParameters;
		if (this->ulBoundaryRelationCells != NULL)
			delete[] this->ulBoundaryRelationCells;
		if (this->uiBoundaryRelationSeries != NULL)
			delete[] this->uiBoundaryRelationSeries;
	}
	this->dBoundaryTimeSeries = NULL;
	this->fBoundaryTimeSeries = NULL;
	this->ulBoundaryRelationCells = NULL;
	this->uiBoundaryRelationSeries = NULL;
	this->uiBoundaryParameters = NULL;
}

/*
 *  Prepares the simulation
 */
void	CSchemePromaides::prepareSimulation()
{
	// Initial volume in the domain
	logger->writeLine("Initial domain volume: " + std::to_string(abs((int)(this->pDomain->getVolume()))) + "m3");

	// Copy the initial conditions
	logger->writeLine("Copying domain data to device...");
	oclBufferCellStates->queueWriteAll();
	oclBufferCellStatesAlt->queueWriteAll();
	oclBufferCellBed->queueWriteAll();
	oclBufferCellManning->queueWriteAll();					//It has a pointer to the Scheme's Values
	oclBufferCellFlowStates->queueWriteAll();
	oclBufferBoundCoup->queueWriteAll();
	oclBufferdsdt->queueWriteAll();
	oclBufferReadN->queueWriteAll();
	oclBufferReadE->queueWriteAll();
	oclBufferWriteN->queueWriteAll();
	oclBufferWriteE->queueWriteAll();
	oclBufferTime->queueWriteAll();
	oclBufferTimestep->queueWriteAll();
	oclBufferTimeHydrological->queueWriteAll();
	this->pDomain->getDevice()->blockUntilFinished();

	// Sort out memory alternation
	bUseAlternateKernel = false;
	bOverrideTimestep = false;
	bDownloadLinks = false;
	bImportLinks = false;
	bUseForcedTimeAdvance = true;
	bCellStatesSynced = true;

	// Need a timer...
	dBatchStartedTime = 0.0;

	// Zero counters
	ulCurrentCellsCalculated = 0;
	uiIterationsSinceSync = 0;
	uiIterationsSinceProgressCheck = 0;
	dLastSyncTime = 0.0;

	// States
	bRunning = false;
	bThreadRunning = false;
	bThreadTerminated = false;
}

DWORD CSchemePromaides::Threaded_runBatchLaunch(LPVOID param)
{
	CSchemePromaides* pScheme = static_cast<CSchemePromaides*>(param);
	pScheme->Threaded_runBatch();
	return 0;
}

/*
 *	Create a new thread to run this batch using
 */
void CSchemePromaides::runBatchThread()
{
	if (this->bThreadRunning)
		return;

	this->bThreadRunning = true;
	this->bThreadTerminated = false;

	HANDLE hThread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)CSchemePromaides::Threaded_runBatchLaunch,
		this,
		0,
		NULL
	);
	if (hThread ) {
		CloseHandle(hThread);
	}
}

/*
 *	Schedule a batch-load of work to run on the device and block
 *	until complete. Runs in its own thread.
 */
void CSchemePromaides::Threaded_runBatch()
{
	// Keep the thread in existence because of the overhead
	// associated with creating a thread.
	while (this->bThreadRunning)
	{
		// Are we expected to run?
		if (!this->bRunning || this->pDomain->getDevice()->isBusy())
		{
			if (this->pDomain->getDevice()->isBusy())
			{
				this->pDomain->getDevice()->blockUntilFinished();
			}
			continue;
		}

		// Have we been asked to update the target time?
		if (this->bUpdateTargetTime)
		{
			this->bUpdateTargetTime = false;
			#ifdef DEBUG_MPI
			logger->writeLine("[DEBUG] Setting new target time of " + Util::secondsToTime(this->dTargetTime) + "...");
			#endif

			if (this->floatPrecision == model::floatPrecision::kSingle)
			{
				*(oclBufferTimeTarget->getHostBlock<float*>()) = static_cast<cl_float>(this->dTargetTime);
			}
			else {
				*(oclBufferTimeTarget->getHostBlock<double*>()) = this->dTargetTime;
			}
			oclBufferTimeTarget->queueWriteAll();
			pDomain->getDevice()->queueBarrier();

			this->bCellStatesSynced = false;
			this->uiIterationsSinceSync = 0;

			bUseForcedTimeAdvance = true;

			/*  Don't do this when syncing the timesteps, as it's important we have a zero timestep immediately after
			 *	output files are written otherwise the timestep wont be reduced across MPI and the domains will go out
			 *  of sync!
			 */
			if (dCurrentTimestep <= 0.0 && this->syncMethod == model::syncMethod::kSyncForecast)
			{
				pDomain->getDevice()->queueBarrier();
				oclKernelTimestepReduction->scheduleExecution();
				pDomain->getDevice()->queueBarrier();
				oclKernelTimestepUpdate->scheduleExecution();
			}

			if (dCurrentTime + dCurrentTimestep > dTargetTime + 1E-5)
			{
				this->dCurrentTimestep = dTargetTime - dCurrentTime;
				this->bOverrideTimestep = true;
			}

			pDomain->getDevice()->queueBarrier();
			//pDomain->getDevice()->blockUntilFinished();		// Shouldn't be needed

			#ifdef DEBUG_MPI
			logger->writeLine("[DEBUG] Done updating new target time to " + Util::secondsToTime(this->dTargetTime) + "...");
			#endif
		}

		// Have we been asked to override the timestep at the start of this batch?
		if ( this->dCurrentTime < dTargetTime &&
			this->bOverrideTimestep)
		{
			if (this->floatPrecision == model::floatPrecision::kSingle)
			{
				*(oclBufferTimestep->getHostBlock<float*>()) = static_cast<cl_float>(this->dCurrentTimestep);
			}
			else {
				*(oclBufferTimestep->getHostBlock<double*>()) = this->dCurrentTimestep;
			}

			oclBufferTimestep->queueWriteAll();

			// TODO: Remove me?
			pDomain->getDevice()->queueBarrier();
			//pDomain->getDevice()->blockUntilFinished();

			this->bOverrideTimestep = false;
		}

		// Have we been asked to import data for our domain links?
		if (this->bImportLinks )
		{

			//this->getNextCellSourceBuffer()->queueWriteAll();
			//this->oclBufferdsdt->queueWriteAll();
			this->oclBufferBoundCoup->queueWriteAll();

			// Last sync time
			this->dLastSyncTime = this->dCurrentTime;
			this->uiIterationsSinceSync = 0;

			// Update the data
			oclKernelResetCounters->scheduleExecution();
			pDomain->getDevice()->queueBarrier();

			// Force timestep recalculation if necessary
			if (this->syncMethod == model::syncMethod::kSyncForecast)
			{
				oclKernelTimestepReduction->scheduleExecution();
				pDomain->getDevice()->queueBarrier();
				oclKernelTimestepUpdate->scheduleExecution();
				pDomain->getDevice()->queueBarrier();
			}

			this->bImportLinks = false;
		}

		// Don't schedule any work if we're already at the sync point
		// TODO: Review this...
		//if (this->dCurrentTime > dTargetTime /* + 1E-5 */)
		//{
		//	bRunning = false;
		//	continue;
		//}

		// Can only schedule one iteration before we need to sync timesteps
		// if timestep sync method is active.
		unsigned int uiQueueAmount = this->uiQueueAdditionSize;
		if (this->syncMethod == model::syncMethod::kSyncTimestep)
			uiQueueAmount = 1;

		#ifdef DEBUG_MPI
		if (uiQueueAmount > 0)
			logger->writeLine("[DEBUG] Starting batch of " + std::to_string(uiQueueAmount) + " with timestep " + Util::secondsToTime(this->dCurrentTimestep) + " at " + Util::secondsToTime(this->dCurrentTime));
		#endif

		// Schedule a batch-load of work for the device
		// Do we need to run any work?
		if (this->dCurrentTime < dTargetTime)
		{
			for (unsigned int i = 0; i < uiQueueAmount; i++)
			{
				#ifdef DEBUG_MPI
				logger->writeLine("Scheduling a new iteration...");
				#endif
				this->scheduleIteration(
					bUseAlternateKernel,
					pDomain->getDevice(),
					pDomain
				);
				uiIterationsSinceSync++;
				uiIterationsSinceProgressCheck++;
				ulCurrentCellsCalculated += this->pDomain->getCellCount();
				bUseAlternateKernel = !bUseAlternateKernel;
			}

			// A further download will be required...
			this->bCellStatesSynced = false;
		}

		// Schedule reading data back. We always need the timestep
		// but we might not need the other details always...
		oclBufferTimestep->queueReadAll();
		oclBufferTime->queueReadAll();
		oclBufferBatchSkipped->queueReadAll();
		oclBufferBatchSuccessful->queueReadAll();
		oclBufferBatchTimesteps->queueReadAll();
		uiIterationsSinceProgressCheck = 0;

		#ifdef _WINDLL
		oclBufferCellStates->queueReadAll();
		#endif

		// Download data for each of the dependent domains
		if (bDownloadLinks)
		{
			// We need to know the time...
			this->pDomain->getDevice()->blockUntilFinished();
			this->readKeyStatistics();

			//pullFromBuffer was here
		}

		// Flush the command queue so we can wait for it to finish
		this->pDomain->getDevice()->flushAndSetMarker();

		// Now that we're thread-based we can actually just block
		// this thread... probably don't need the marker	
		this->pDomain->getDevice()->blockUntilFinished();

		// Are cell states now synced?
		if (bDownloadLinks)
		{
			bDownloadLinks = false;
			bCellStatesSynced = true;
		}

		// Read from buffers back to scheme memory space
		this->readKeyStatistics();

		#ifdef DEBUG_MPI
		if (uiQueueAmount > 0)
		{
			logger->writeLine("[DEBUG] Finished batch of " + std::to_string(uiQueueAmount) + " with timestep " + Util::secondsToTime(this->dCurrentTimestep) + " at " + Util::secondsToTime(this->dCurrentTime));
			if (this->dCurrentTimestep < 0.0)
			{
				logger->writeLine("[DEBUG] We have a negative timestep...");
			}
		}
		#endif

		// Wait until further work is scheduled
		this->bRunning = false;
	}

	this->bThreadTerminated = true;
}

/*
 *  Runs the actual simulation until completion or error
 */
void	CSchemePromaides::runSimulation(double dTargetTime, double dRealTime)
{
	// Wait for current work to finish
	if (this->bRunning || this->pDomain->getDevice()->isBusy())
		return;

	// Has the target time changed?
	if (this->dTargetTime != dTargetTime)
		setTargetTime(dTargetTime);

	// If we've hit our target time, download the data we need for any dependent
	// domain links (or in timestep sync, hit the iteration limit)
	//bDownloadLinks = true;
	if (dTargetTime - this->dCurrentTime <= 0)
		bDownloadLinks = true;

	if (dRealTime > 1E-5) {
		// We're aiming for a seconds worth of work to be carried out
		double dBatchDuration = dRealTime - dBatchStartedTime;
		unsigned int uiOldQueueAdditionSize = this->uiQueueAdditionSize;


		this->uiQueueAdditionSize = static_cast<unsigned int>(max(static_cast<unsigned int>(1), min(this->uiBatchRate * 3, static_cast<unsigned int>(ceil(1.0 / (dBatchDuration / static_cast<double>(this->uiQueueAdditionSize)))))));

		// Stop silly jumps in the queue addition size
		if (this->uiQueueAdditionSize > uiOldQueueAdditionSize * 2 &&
			this->uiQueueAdditionSize > 40)
			this->uiQueueAdditionSize = min(static_cast<unsigned int>(this->uiBatchRate * 3), uiOldQueueAdditionSize * 2);

		// Can't have zero queue addition size
		if (this->uiQueueAdditionSize < 1)
			this->uiQueueAdditionSize = 1;
	}
	dBatchStartedTime = dRealTime;
	this->bRunning = true;
	this->runBatchThread();

}

/*
 *  Clean-up temporary resources consumed during the simulation
 */
void	CSchemePromaides::cleanupSimulation()
{
	// TODO: Anything to clean-up? Callbacks? Timers?
	dBatchStartedTime = 0.0;

	// Kill the worker thread
	bRunning = false;
	bThreadRunning = false;

	// Wait for the thread to terminate before returning
	while (!bThreadTerminated && bThreadRunning) {}
}

/*
 *  Rollback the simulation to the last successful round
 */
void	CSchemePromaides::rollbackSimulation(double dCurrentTime, double dTargetTime)
{
	// Wait until any pending tasks have completed first...
	this->getDomain()->getDevice()->blockUntilFinished();

	uiIterationsSinceSync = 0;

	this->dCurrentTime = dCurrentTime;
	this->dTargetTime = dTargetTime;

	// Update the time
	if (this->floatPrecision == model::floatPrecision::kSingle)
	{
		*(oclBufferTime->getHostBlock<float*>()) = static_cast<cl_float>(dCurrentTime);
		*(oclBufferTimeTarget->getHostBlock<float*>()) = static_cast<cl_float> (dTargetTime);
	}
	else {
		*(oclBufferTime->getHostBlock<double*>()) = dCurrentTime;
		*(oclBufferTimeTarget->getHostBlock<double*>()) = dTargetTime;
	}

	// Write all memory buffers...
	oclBufferTime->queueWriteAll();
	oclBufferTimeTarget->queueWriteAll();
	oclBufferCellStatesAlt->queueWriteAll();
	oclBufferCellStates->queueWriteAll();

	// Schedule timestep calculation again
	// Timestep reduction
	if (this->bDynamicTimestep)
	{
		oclKernelTimestepReduction->scheduleExecution();
		pDomain->getDevice()->queueBarrier();
	}

	// Timestep update without simulation time update
	if (this->syncMethod != model::syncMethod::kSyncTimestep)
		oclKernelTimestepUpdate->scheduleExecution();
	bUseForcedTimeAdvance = true;

	// Clear the failure state
	oclKernelResetCounters->scheduleExecution();

	pDomain->getDevice()->queueBarrier();
	pDomain->getDevice()->flush();
}

/*
 *  Is the simulation a failure requiring a rollback?
 */
bool	CSchemePromaides::isSimulationFailure(double dExpectedTargetTime)
{
	if (bRunning)
		return false;

	// This also shouldn't happen... but might...
	if (this->dCurrentTime > dExpectedTargetTime + 1E-5)
	{
		model::doError(
			"Scheme has exceeded target sync time. Rolling back...",
			model::errorCodes::kLevelWarning
		);
		logger->writeLine(
			"Current time: " + std::to_string(dCurrentTime) +
			", target time: " + std::to_string(dExpectedTargetTime)
		);
		return true;
	}

	// Assume success
	return false;
}

/*
 *  Force the timestap to be advanced even if we're synced
 */
void	CSchemePromaides::forceTimeAdvance()
{
	bUseForcedTimeAdvance = true;
}

/*
 *  Is the simulation ready to be synchronised?
 */
bool	CSchemePromaides::isSimulationSyncReady(double dExpectedTargetTime)
{
	// Check whether we're still busy or failure occured
	//if ( isSimulationFailure() )
	//	return false;

	if (bRunning)
		return false;

	// Have we hit our target time?
	// TODO: Review whether this is appropriate (need fabs?) (1E-5?)
	if (this->syncMethod == model::syncMethod::kSyncTimestep)
	{
		// Any criteria required for timestep-based sync?
	}
	else {
		if (dExpectedTargetTime - dCurrentTime > 1E-5)
		{
			logger->writeLine("Expected target: " + std::to_string(dExpectedTargetTime) + " Current time: " + std::to_string(dCurrentTime));
			return false;
		}
	}

	// Are we synchronising the timesteps?
	if (this->syncMethod == model::syncMethod::kSyncTimestep &&
		dExpectedTargetTime - dCurrentTime > 1E-5 &&
		dCurrentTime > 0.0)
		return false;

	// Assume success
	return true;
}

/*
 *  Runs the actual simulation until completion or error
 */
void	CSchemePromaides::scheduleIteration(
	bool			bUseAlternateKernel,
	COCLDevice* pDevice,
	CDomainCartesian* pDomain
)
{
	// Re-set the kernel arguments to use the correct cell state buffer
	if (bUseAlternateKernel)
	{
		oclKernelFullTimestep->assignArgument(2, oclBufferCellStatesAlt);
		oclKernelFullTimestep->assignArgument(3, oclBufferCellStates);

		oclKernelFullTimestep->assignArgument(8, oclBufferWriteN);
		oclKernelFullTimestep->assignArgument(9, oclBufferWriteE);
		oclKernelFullTimestep->assignArgument(10, oclBufferReadN);
		oclKernelFullTimestep->assignArgument(11, oclBufferReadE);

		oclKernelBoundary->assignArgument(2, oclBufferCellStatesAlt);
		oclKernelBoundary->assignArgument(4, oclBufferWriteN);
		oclKernelBoundary->assignArgument(5, oclBufferWriteE);
		oclKernelFriction->assignArgument(1, oclBufferCellStates);
		oclKernelTimestepReduction->assignArgument(3, oclBufferCellStates);
	}
	else {
		oclKernelFullTimestep->assignArgument(2, oclBufferCellStates);
		oclKernelFullTimestep->assignArgument(3, oclBufferCellStatesAlt);

		oclKernelFullTimestep->assignArgument(10, oclBufferWriteN);
		oclKernelFullTimestep->assignArgument(11, oclBufferWriteE);
		oclKernelFullTimestep->assignArgument(8, oclBufferReadN);
		oclKernelFullTimestep->assignArgument(9, oclBufferReadE);

		oclKernelBoundary->assignArgument(2, oclBufferCellStates);
		oclKernelBoundary->assignArgument(4, oclBufferReadN);
		oclKernelBoundary->assignArgument(5, oclBufferReadE);
		oclKernelFriction->assignArgument(1, oclBufferCellStatesAlt);
		oclKernelTimestepReduction->assignArgument(3, oclBufferCellStatesAlt);
	}

	// Main scheme kernel
	oclKernelBoundary->scheduleExecution();
	pDevice->queueBarrier();

	// Main scheme kernel
	oclKernelFullTimestep->scheduleExecution();
	pDevice->queueBarrier();


	// Timestep reduction
	if (this->bDynamicTimestep)
	{
		oclKernelTimestepReduction->scheduleExecution();
		pDevice->queueBarrier();
	}

	// Time advancing
	oclKernelTimeAdvance->scheduleExecution();
	pDevice->queueBarrier();

	// Only block after every iteration when testing things that need it...
	// Big performance hit...
	//pDevice->blockUntilFinished();
}

/*
 *  Read back all of the domain data
 */
void CSchemePromaides::readDomainAll()
{
	if (bUseAlternateKernel)
	{
		oclBufferCellStatesAlt->queueReadAll();
	}
	else {
		oclBufferCellStates->queueReadAll();
	}
}

/*
 *  Read back domain data for the synchronisation zones only
 */
void CSchemePromaides::importLinkZoneData()
{
	this->bImportLinks = true;
}

/*
*	Fetch the pointer to the last cell source buffer
*/
COCLBuffer* CSchemePromaides::getLastCellSourceBuffer()
{
	if (bUseAlternateKernel)
	{
		return oclBufferCellStates;
	}
	else {
		return oclBufferCellStatesAlt;
	}
}

/*
 *	Fetch the pointer to the next cell source buffer
 */
COCLBuffer* CSchemePromaides::getNextCellSourceBuffer()
{
	if (bUseAlternateKernel)
	{
		return oclBufferCellStatesAlt;
	}
	else {
		return oclBufferCellStates;
	}
}

/*
 *  Save current cell states incase of need to rollback
 */
void CSchemePromaides::saveCurrentState()
{
	// Flag is flipped after an iteration, so if it's true that means
	// the last one saved to the normal cell state buffer...
	getNextCellSourceBuffer()->queueReadAll();

	// Reset iteration tracking
	// TODO: Should this be moved into the sync function?
	uiIterationsSinceSync = 0;

	// Block until complete
	// TODO: Investigate - does this need to be a blocking command?
	// Blocking should be carried out in CModel to allow multiple domains 
	// to download at once...
	//pDomain->getDevice()->queueBarrier();
	//pDomain->getDevice()->blockUntilFinished();
}

/*
 *  Set the target sync time
 */
void CSchemePromaides::setTargetTime(double dTime)
{
	if (dTime == this->dTargetTime)
		return;

	this->dTargetTime = dTime;
	//this->dLastSyncTime = this->dCurrentTime;
	this->bUpdateTargetTime = true;
	//this->bUseForcedTimeAdvance = true;
}

/*
 *  Propose a synchronisation point based on current performance of the scheme
 */
double CSchemePromaides::proposeSyncPoint(double dCurrentTime)
{
	// TODO: Improve this so we're using more than just the current timestep...
	double dProposal = dCurrentTime + fabs(this->dTimestep);

	// Can only use this method once we have some simulation completed, not valid
	// at the start.
	if (dCurrentTime > 1E-5 && uiBatchSuccessful > 0)
	{
		// Try to accommodate approximately three spare iterations
		dProposal = dCurrentTime +
			max(fabs(this->dTimestep), 999999999.0 * (dBatchTimesteps / uiBatchSuccessful) * ((999999999.0 - this->syncBatchSpares) / 999999999.0));
		// Don't allow massive jumps
		//if ((dProposal - dCurrentTime) > dBatchTimesteps * 3.0)
		//	dProposal = dCurrentTime + dBatchTimesteps * 3.0;
		// If we've hit our rollback limit, use the time we reached to determine a conservative estimate for 
		// a new sync point
	}
	else {
		// Can't return a suggestion of not going anywhere..
		if (dProposal - dCurrentTime < 1E-5)
			dProposal = dCurrentTime + fabs(this->dTimestep);
	}

	// If using real-time visualisation, force syncs more frequently (?)
	#ifdef _WINDLL
		// This line is broken... Maybe? Not sure...
		//dProposal = min( dProposal, dCurrentTime + ( dBatchTimesteps / uiBatchSuccessful ) * 2 * uiQueueAdditionSize );
	#endif

	return dProposal;
}

/*
 *  Get the batch average timestep
 */
double CSchemePromaides::getAverageTimestep()
{
	if (uiBatchSuccessful < 1) return 0.0;
	return dBatchTimesteps / uiBatchSuccessful;
}

/*
 *	Force a specific timestep (when synchronising them)
 */
void CSchemePromaides::forceTimestep(double dTimestep)
{
	// Might not have to write anything :-)
	if (dTimestep == this->dCurrentTimestep)
		return;

	this->dCurrentTimestep = dTimestep;
	this->bOverrideTimestep = true;
}

/*
 *  Fetch key details back to the right places in memory
 */
void	CSchemePromaides::readKeyStatistics()
{
	cl_uint uiLastBatchSuccessful = uiBatchSuccessful;

	// Pull key data back from our buffers to the scheme class
	if (this->floatPrecision == model::floatPrecision::kSingle)
	{
		dCurrentTimestep = static_cast<cl_double>(*(oclBufferTimestep->getHostBlock<float*>()));
		dCurrentTime = static_cast<cl_double>(*(oclBufferTime->getHostBlock<float*>()));
		dBatchTimesteps = static_cast<cl_double>(*(oclBufferBatchTimesteps->getHostBlock<float*>()));
	}
	else {
		dCurrentTimestep = *(oclBufferTimestep->getHostBlock<double*>());
		dCurrentTime = *(oclBufferTime->getHostBlock<double*>());
		dBatchTimesteps = *(oclBufferBatchTimesteps->getHostBlock<double*>());
	}
	uiBatchSuccessful = *(oclBufferBatchSuccessful->getHostBlock<cl_uint*>());
	uiBatchSkipped = *(oclBufferBatchSkipped->getHostBlock<cl_uint*>());
	uiBatchRate = uiBatchSuccessful > uiLastBatchSuccessful ? (uiBatchSuccessful - uiLastBatchSuccessful) : 1;
}
