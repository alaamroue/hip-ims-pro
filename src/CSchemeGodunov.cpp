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
 *  Scheme class
 * ------------------------------------------
 *
 */
#include <algorithm>

#include "common.h"
#include "CBoundaryMap.h"
#include "CBoundary.h"
#include "CDomainManager.h"
#include "CDomain.h"
#include "CDomainLink.h"
#include "CDomainCartesian.h"
#include "CSchemeGodunov.h"
#include "CSchemeMUSCLHancock.h"
#include "CSchemeInertial.h"

using std::min;
using std::max;
/*
 *  Default constructor
 */
CSchemeGodunov::CSchemeGodunov(void)
{

}

CSchemeGodunov::CSchemeGodunov(CModel* cmodel)
{
	this->logger = cmodel->getLogger();
	logger->writeLine("Godunov-type scheme loaded for execution on OpenCL platform.");

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
	this->bFrictionInFluxKernel = true;
	this->bIncludeBoundaries = false;
	this->uiTimestepReductionWavefronts = 200;

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
	oclKernelFriction = NULL;
	oclKernelTimestepReduction = NULL;
	oclKernelTimeAdvance = NULL;
	oclKernelResetCounters = NULL;
	oclKernelTimestepUpdate = NULL;
	oclBufferCellStates = NULL;
	oclBufferCellStatesAlt = NULL;
	oclBufferCellManning = NULL;
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
	this->bFrictionEffects = cmodel->getFrictionStatus();
	this->ulCachedWorkgroupSizeX = cmodel->ulCachedWorkgroupSizeX;
	this->ulCachedWorkgroupSizeY = cmodel->ulCachedWorkgroupSizeX;
	this->ulNonCachedWorkgroupSizeX = cmodel->ulNonCachedWorkgroupSizeX;
	this->ulNonCachedWorkgroupSizeY = cmodel->ulNonCachedWorkgroupSizeY;
	this->floatPrecision = cmodel->getFloatPrecision();
	this->simulationLength = cmodel->getSimulationLength();
	this->outputFrequency = cmodel->getOutputFrequency();;
	this->syncMethod = cmodel->getDomainSet()->getSyncMethod();
	this->domainCount = cmodel->getDomainSet()->getDomainCount();
	this->syncBatchSpares = cmodel->getDomainSet()->getSyncBatchSpares();


	//logger->writeLine("Populated scheme with default settings.");
}

/*
 *  Destructor
 */
CSchemeGodunov::~CSchemeGodunov(void)
{
	this->releaseResources();
	logger->writeLine("The Godunov scheme class was unloaded from memory.");
}

/*
 *  Read in settings from the XML configuration file for this scheme
 */
void	CSchemeGodunov::setupFromConfig()
{
	// Call the base class function which handles a couple of things
	CScheme::setupFromConfig();


	this->setCourantNumber(0.5);

	//this->setDryThreshold( boost::lexical_cast<double>( 0.5 ) );

	//unsigned char ucTimestepMode = 255;
	//ucTimestepMode = model::timestepMode::kCFL;
	//ucTimestepMode = model::timestepMode::kFixed;
	//this->setTimestepMode( ucTimestepMode );


	//this->setTimestep( boost::lexical_cast<double>( ?? ) );
	//this->setReductionWavefronts( boost::lexical_cast<unsigned int>( ?? ) );



	unsigned char ucFriction = 255;
	//ucFriction = 1;
	ucFriction = 0;
	this->setFrictionStatus(ucFriction == 1);

	//unsigned char ucSolver = 255;
	//ucSolver = model::solverTypes::kHLLC;
	//this->setRiemannSolver( ucSolver );

	//this->setCachedWorkgroupSize( boost::lexical_cast<unsigned int>( 32 ) );
	//this->setNonCachedWorkgroupSize( boost::lexical_cast<unsigned int>( 32 ) );
	this->setCachedWorkgroupSize(1, 1);
	this->setNonCachedWorkgroupSize(1, 1);
	//this->setCachedWorkgroupSize( boost::lexical_cast<unsigned int>( sSizes[0] ) );
	//this->setCachedWorkgroupSize( boost::lexical_cast<unsigned int>( sSizes[0] ), boost::lexical_cast<unsigned int>( sSizes[1] ) );
	//this->setNonCachedWorkgroupSize( boost::lexical_cast<unsigned int>( sSizes[0] ) );
	//this->setNonCachedWorkgroupSize( boost::lexical_cast<unsigned int>( sSizes[0] ), boost::lexical_cast<unsigned int>( sSizes[1] ) );

	//unsigned char usCache = 255;
	//usCache = model::schemeConfigurations::godunovType::kCacheEnabled;
	//usCache = model::schemeConfigurations::godunovType::kCacheNone;
	//this->setCacheMode( usCache );

	//unsigned char ucCacheConstraints = 255;
	//ucCacheConstraints = model::cacheConstraints::godunovType::kCacheActualSize;
	//ucCacheConstraints = model::cacheConstraints::godunovType::kCacheAllowOversize;
	//ucCacheConstraints = model::cacheConstraints::godunovType::kCacheAllowUndersize;
	//this->setCacheConstraints( ucCacheConstraints );

}

/*
 *  Log the details and properties of this scheme instance.
 */
void CSchemeGodunov::logDetails()
{
	logger->writeDivide();
	unsigned short wColour = model::cli::colourInfoBlock;

	std::string sSolver = "Undefined";
	switch (this->ucSolverType)
	{
	case model::solverTypes::kHLLC:
		sSolver = "HLLC (Approximate)";
		break;
	}

	std::string sConfiguration = "Undefined";
	switch (this->ucConfiguration)
	{
	case model::schemeConfigurations::godunovType::kCacheNone:
		sConfiguration = "No local caching";
		break;
	case model::schemeConfigurations::godunovType::kCacheEnabled:
		sConfiguration = "Original state caching";
		break;
	}

	logger->writeLine("GODUNOV-TYPE 1ST-ORDER-ACCURATE SCHEME", true, wColour);
	logger->writeLine("  Timestep mode:      " + (std::string)(this->bDynamicTimestep ? "Dynamic" : "Fixed"), true, wColour);
	logger->writeLine("  Courant number:     " + (std::string)(this->bDynamicTimestep ? std::to_string(this->dCourantNumber) : "N/A"), true, wColour);
	logger->writeLine("  Initial timestep:   " + Util::secondsToTime(this->dTimestep), true, wColour);
	logger->writeLine("  Data reduction:     " + std::to_string(this->uiTimestepReductionWavefronts) + " divisions", true, wColour);
	logger->writeLine("  Boundaries:         " + std::to_string(this->pDomain->getBoundaries()->getBoundaryCount()), true, wColour);
	logger->writeLine("  Riemann solver:     " + sSolver, true, wColour);
	logger->writeLine("  Configuration:      " + sConfiguration, true, wColour);
	logger->writeLine("  Friction effects:   " + (std::string)(this->bFrictionEffects ? "Enabled" : "Disabled"), true, wColour);
	logger->writeLine("  Kernel queue mode:  " + (std::string)(this->bAutomaticQueue ? "Automatic" : "Fixed size"), true, wColour);
	logger->writeLine((std::string)(this->bAutomaticQueue ? "  Initial queue:      " : "  Fixed queue:        ") + std::to_string(this->uiQueueAdditionSize) + " iteration(s)", true, wColour);
	logger->writeLine("  Debug output:       " + (std::string)(this->bDebugOutput ? "Enabled" : "Disabled"), true, wColour);

	logger->writeDivide();
}

/*
 *  Run all preparation steps
 */
void CSchemeGodunov::prepareAll()
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
bool CSchemeGodunov::prepareCode()
{
	bool bReturnState = true;

	oclModel->appendCodeFromResource("CLDomainCartesian_H");
	oclModel->appendCodeFromResource("CLFriction_H");
	oclModel->appendCodeFromResource("CLSolverHLLC_H");
	oclModel->appendCodeFromResource("CLDynamicTimestep_H");
	oclModel->appendCodeFromResource("CLSchemeGodunov_H");
	oclModel->appendCodeFromResource("CLBoundaries_H");

	oclModel->appendCodeFromResource("CLDomainCartesian_C");
	oclModel->appendCodeFromResource("CLFriction_C");
	oclModel->appendCodeFromResource("CLSolverHLLC_C");
	oclModel->appendCodeFromResource("CLDynamicTimestep_C");
	oclModel->appendCodeFromResource("CLSchemeGodunov_C");
	oclModel->appendCodeFromResource("CLBoundaries_C");

	bReturnState = oclModel->compileProgram();

	return bReturnState;
}

/*
 *  Create boundary data arrays etc.
 */
bool CSchemeGodunov::prepareBoundaries()
{
	CBoundaryMap* pBoundaries = this->pDomain->getBoundaries();
	pBoundaries->prepareBoundaries(oclModel, oclBufferCellBed, oclBufferCellManning, oclBufferTime, oclBufferTimeHydrological, oclBufferTimestep);

	return true;
}

/*
 *  Set the dry cell threshold depth
 */
void	CSchemeGodunov::setDryThreshold(double dThresholdDepth)
{
	this->dThresholdVerySmall = dThresholdDepth;
	this->dThresholdQuiteSmall = dThresholdDepth * 10;
}

/*
 *  Get the dry cell threshold depth
 */
double	CSchemeGodunov::getDryThreshold()
{
	return this->dThresholdVerySmall;
}

/*
 *  Set number of wavefronts used in reductions
 */
void	CSchemeGodunov::setReductionWavefronts(unsigned int uiWavefronts)
{
	this->uiTimestepReductionWavefronts = uiWavefronts;
}

/*
 *  Get number of wavefronts used in reductions
 */
unsigned int	CSchemeGodunov::getReductionWavefronts()
{
	return this->uiTimestepReductionWavefronts;
}

/*
 *  Set the Riemann solver to use
 */
void	CSchemeGodunov::setRiemannSolver(unsigned char ucRiemannSolver)
{
	this->ucSolverType = ucRiemannSolver;
}

/*
 *  Get the Riemann solver in use
 */
unsigned char	CSchemeGodunov::getRiemannSolver()
{
	return this->ucSolverType;
}

/*
 *  Set the cache configuration to use
 */
void	CSchemeGodunov::setCacheMode(unsigned char ucCacheMode)
{
	this->ucConfiguration = ucCacheMode;
}

/*
 *  Get the cache configuration in use
 */
unsigned char	CSchemeGodunov::getCacheMode()
{
	return this->ucConfiguration;
}

/*
 *  Set the cache size
 */
void	CSchemeGodunov::setCachedWorkgroupSize(unsigned char ucSize)
{
	this->ulCachedWorkgroupSizeX = ucSize; this->ulCachedWorkgroupSizeY = ucSize;
}
void	CSchemeGodunov::setCachedWorkgroupSize(unsigned char ucSizeX, unsigned char ucSizeY)
{
	this->ulCachedWorkgroupSizeX = ucSizeX; this->ulCachedWorkgroupSizeY = ucSizeY;
}
void	CSchemeGodunov::setNonCachedWorkgroupSize(unsigned char ucSize)
{
	this->ulNonCachedWorkgroupSizeX = ucSize; this->ulNonCachedWorkgroupSizeY = ucSize;
}
void	CSchemeGodunov::setNonCachedWorkgroupSize(unsigned char ucSizeX, unsigned char ucSizeY)
{
	this->ulNonCachedWorkgroupSizeX = ucSizeX; this->ulNonCachedWorkgroupSizeY = ucSizeY;
}

/*
 *  Set the cache constraints
 */
void	CSchemeGodunov::setCacheConstraints(unsigned char ucCacheConstraints)
{
	this->ucCacheConstraints = ucCacheConstraints;
}

/*
 *  Get the cache constraints
 */
unsigned char	CSchemeGodunov::getCacheConstraints()
{
	return this->ucCacheConstraints;
}

/*
 *  Calculate the dimensions for executing the problems (e.g. reduction glob/local sizes)
 */
bool CSchemeGodunov::prepare1OExecDimensions()
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
		(this->ucCacheConstraints == model::cacheConstraints::musclHancock::kCacheAllowUndersize ? -1 : 0);
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
bool CSchemeGodunov::prepare1OConstants()
{
	CDomainCartesian* pDomain = static_cast<CDomainCartesian*>(this->pDomain);

	// --
	// Dry cell threshold depths
	// --
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

	if (this->ucConfiguration == model::schemeConfigurations::godunovType::kCacheNone)
	{
		oclModel->registerConstant(
			"REQD_WG_SIZE_FULL_TS",
			"__attribute__((reqd_work_group_size(" + std::to_string(this->ulNonCachedWorkgroupSizeX) + ", " + std::to_string(this->ulNonCachedWorkgroupSizeY) + ", 1)))"
		);
	}
	if (this->ucConfiguration == model::schemeConfigurations::godunovType::kCacheEnabled)
	{
		oclModel->registerConstant(
			"REQD_WG_SIZE_FULL_TS",
			"__attribute__((reqd_work_group_size(" + std::to_string(this->ulNonCachedWorkgroupSizeX) + ", " + std::to_string(this->ulNonCachedWorkgroupSizeY) + ", 1)))"
		);
	}

	oclModel->registerConstant(
		"REQD_WG_SIZE_LINE",
		"__attribute__((reqd_work_group_size(" + std::to_string(this->ulReductionWorkgroupSize) + ", 1, 1)))"
	);

	// --
	// Size of local cache arrays
	// --

	switch (this->ucCacheConstraints)
	{
	case model::cacheConstraints::godunovType::kCacheActualSize:
		oclModel->registerConstant("GTS_DIM1", std::to_string(this->ulCachedWorkgroupSizeX));
		oclModel->registerConstant("GTS_DIM2", std::to_string(this->ulCachedWorkgroupSizeY));
		break;
	case model::cacheConstraints::godunovType::kCacheAllowUndersize:
		oclModel->registerConstant("GTS_DIM1", std::to_string(this->ulCachedWorkgroupSizeX));
		oclModel->registerConstant("GTS_DIM2", std::to_string(this->ulCachedWorkgroupSizeY));
		break;
	case model::cacheConstraints::godunovType::kCacheAllowOversize:
		oclModel->registerConstant("GTS_DIM1", std::to_string(this->ulCachedWorkgroupSizeX));
		oclModel->registerConstant("GTS_DIM2", std::to_string(this->ulCachedWorkgroupSizeY == 16 ? 17 : ulCachedWorkgroupSizeY));
		break;
	}

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

	if (this->bFrictionEffects)
	{
		oclModel->registerConstant("FRICTION_ENABLED", "1");
	}
	else {
		oclModel->removeConstant("FRICTION_ENABLED");
	}

	if (this->bFrictionInFluxKernel)
	{
		oclModel->registerConstant("FRICTION_IN_FLUX_KERNEL", "1");
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
bool CSchemeGodunov::prepare1OMemory()
{
	bool						bReturnState = true;
	CExecutorControlOpenCL* pExecutor = cExecutorControlOpenCL;
	CDomain* pDomain = this->pDomain;
	CBoundaryMap* pBoundaries = pDomain->getBoundaries();
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

	void* pCellStates = NULL, * pBedElevations = NULL, * pManningValues = NULL;
	pDomain->createStoreBuffers(
		&pCellStates,
		&pBedElevations,
		&pManningValues,
		ucFloatSize
	);

	oclBufferCellStates = new COCLBuffer("Cell states", oclModel, false, true);
	oclBufferCellStatesAlt = new COCLBuffer("Cell states (alternate)", oclModel, false, true);
	oclBufferCellManning = new COCLBuffer("Manning coefficients", oclModel, true, true);
	oclBufferCellBed = new COCLBuffer("Bed elevations", oclModel, true, true);

	oclBufferCellStates->logger = logger;
	oclBufferCellStatesAlt->logger = logger; 
	oclBufferCellManning->logger = logger;
	oclBufferCellBed->logger = logger;


	oclBufferCellStates->setPointer(pCellStates, ucFloatSize * 4 * pDomain->getCellCount());
	oclBufferCellStatesAlt->setPointer(pCellStates, ucFloatSize * 4 * pDomain->getCellCount());
	oclBufferCellManning->setPointer(pManningValues, ucFloatSize * pDomain->getCellCount());
	oclBufferCellBed->setPointer(pBedElevations, ucFloatSize * pDomain->getCellCount());


	oclBufferCellStates->createBuffer();
	oclBufferCellStatesAlt->createBuffer();
	oclBufferCellManning->createBuffer();
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
bool CSchemeGodunov::prepareGeneralKernels()
{
	bool						bReturnState = true;
	CExecutorControlOpenCL* pExecutor = cExecutorControlOpenCL;
	CDomain* pDomain = this->pDomain;
	CBoundaryMap* pBoundaries = pDomain->getBoundaries();
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
bool CSchemeGodunov::prepare1OKernels()
{
	bool						bReturnState = true;
	CExecutorControlOpenCL* pExecutor = cExecutorControlOpenCL;
	CDomain* pDomain = this->pDomain;
	COCLDevice* pDevice = pExecutor->getDevice();

	// --
	// Godunov-type scheme kernels
	// --

	if (this->ucConfiguration == model::schemeConfigurations::godunovType::kCacheNone)
	{
		oclKernelFullTimestep = oclModel->getKernel("gts_cacheDisabled");
		oclKernelFullTimestep->setGroupSize(this->ulNonCachedWorkgroupSizeX, this->ulNonCachedWorkgroupSizeY);
		oclKernelFullTimestep->setGlobalSize(this->ulNonCachedGlobalSizeX, this->ulNonCachedGlobalSizeY);
		COCLBuffer* aryArgsFullTimestep[] = { oclBufferTimestep, oclBufferCellBed, oclBufferCellStates, oclBufferCellStatesAlt, oclBufferCellManning };
		oclKernelFullTimestep->assignArguments(aryArgsFullTimestep);
	}
	if (this->ucConfiguration == model::schemeConfigurations::godunovType::kCacheEnabled)
	{
		oclKernelFullTimestep = oclModel->getKernel("gts_cacheEnabled");
		oclKernelFullTimestep->setGroupSize(this->ulCachedWorkgroupSizeX, this->ulCachedWorkgroupSizeY);
		oclKernelFullTimestep->setGlobalSize(this->ulCachedGlobalSizeX, this->ulCachedGlobalSizeY);
		COCLBuffer* aryArgsFullTimestep[] = { oclBufferTimestep, oclBufferCellBed, oclBufferCellStates, oclBufferCellStatesAlt, oclBufferCellManning };
		oclKernelFullTimestep->assignArguments(aryArgsFullTimestep);
	}

	return bReturnState;
}

/*
 *  Release all OpenCL resources consumed using the OpenCL methods
 */
void CSchemeGodunov::releaseResources()
{
	this->bReady = false;

	logger->writeLine("Releasing scheme resources held for OpenCL.");

	this->release1OResources();
}

/*
 *  Release all OpenCL resources consumed using the OpenCL methods
 */
void CSchemeGodunov::release1OResources()
{
	this->bReady = false;

	logger->writeLine("Releasing 1st-order scheme resources held for OpenCL.");

	if (this->oclModel != NULL)							delete oclModel;
	if (this->oclKernelFullTimestep != NULL)				delete oclKernelFullTimestep;
	if (this->oclKernelFriction != NULL)					delete oclKernelFriction;
	if (this->oclKernelTimestepReduction != NULL)			delete oclKernelTimestepReduction;
	if (this->oclKernelTimeAdvance != NULL)				delete oclKernelTimeAdvance;
	if (this->oclKernelTimestepUpdate != NULL)			delete oclKernelTimestepUpdate;
	if (this->oclKernelResetCounters != NULL)				delete oclKernelResetCounters;
	if (this->oclBufferCellStates != NULL)				delete oclBufferCellStates;
	if (this->oclBufferCellStatesAlt != NULL)				delete oclBufferCellStatesAlt;
	if (this->oclBufferCellManning != NULL)				delete oclBufferCellManning;
	if (this->oclBufferCellBed != NULL)					delete oclBufferCellBed;
	if (this->oclBufferTimestep != NULL)					delete oclBufferTimestep;
	if (this->oclBufferTimestepReduction != NULL)			delete oclBufferTimestepReduction;
	if (this->oclBufferTime != NULL)						delete oclBufferTime;
	if (this->oclBufferTimeTarget != NULL)				delete oclBufferTimeTarget;
	if (this->oclBufferTimeHydrological != NULL)			delete oclBufferTimeHydrological;

	oclModel = NULL;
	oclKernelFullTimestep = NULL;
	oclKernelFriction = NULL;
	oclKernelTimestepReduction = NULL;
	oclKernelTimeAdvance = NULL;
	oclKernelResetCounters = NULL;
	oclKernelTimestepUpdate = NULL;
	oclBufferCellStates = NULL;
	oclBufferCellStatesAlt = NULL;
	oclBufferCellManning = NULL;
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
void	CSchemeGodunov::prepareSimulation()
{
	// Adjust cell bed elevations if necessary for boundary conditions
	logger->writeLine("Adjusting domain data for boundaries...");
	this->pDomain->getBoundaries()->applyDomainModifications();

	// Initial volume in the domain
	logger->writeLine("Initial domain volume: " + std::to_string(abs((int)(this->pDomain->getVolume()))) + "m3");

	// Copy the initial conditions
	logger->writeLine("Copying domain data to device...");
	oclBufferCellStates->queueWriteAll();
	oclBufferCellStatesAlt->queueWriteAll();
	oclBufferCellBed->queueWriteAll();
	oclBufferCellManning->queueWriteAll();					//It has a pointer to the Scheme's Values
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

DWORD CSchemeGodunov::Threaded_runBatchLaunch(LPVOID param)
{
	CSchemeGodunov* pScheme = static_cast<CSchemeGodunov*>(param);
	pScheme->Threaded_runBatch();
	return 0;
}

/*
 *	Create a new thread to run this batch using
 */
void CSchemeGodunov::runBatchThread()
{
	if (this->bThreadRunning)
		return;

	this->bThreadRunning = true;
	this->bThreadTerminated = false;

	HANDLE hThread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)CSchemeGodunov::Threaded_runBatchLaunch,
		this,
		0,
		NULL
	);
	CloseHandle(hThread);
}

/*
 *	Schedule a batch-load of work to run on the device and block
 *	until complete. Runs in its own thread.
 */
void CSchemeGodunov::Threaded_runBatch()
{
	unsigned long ulCellID;
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
		if ( //uiIterationsSinceSync < this->pDomain->getRollbackLimit() &&
			this->dCurrentTime < dTargetTime &&
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
		if (this->bImportLinks)
		{
			std::cout << "Ohh hhi mark" << std::endl;

			// Create Fake Data
			double value = 0;
			cl_double4* vStateData = new cl_double4[10000];
			for (unsigned long iRow = 0; iRow < 100; ++iRow) {
				for (unsigned long iCol = 0; iCol < 100; ++iCol) {
					ulCellID = pDomain->getCellID(iCol, iRow);
					value = np->getBedElevation(ulCellID);
					vStateData[ulCellID].s[0] = np->getBedElevation(ulCellID)*2;
					vStateData[ulCellID].s[1] = np->getBedElevation(ulCellID);
					vStateData[ulCellID].s[2] = 0.0;
					vStateData[ulCellID].s[3] = 0.0;
				}
			}



			oclBufferCellStates->queueWritePartial(0, 320000, vStateData);
			oclBufferCellStatesAlt->queueWritePartial(0, 320000, vStateData);

			// Import data from links which are 'dependent' on this domain
			for (unsigned int i = 0; i < pDomain->getLinkCount(); i++)
			{
				pDomain->getLink(i)->pushToBuffer(this->getNextCellSourceBuffer());
			}

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
		if (uiIterationsSinceSync < this->pDomain->getRollbackLimit() &&
			this->dCurrentTime < dTargetTime)
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

			#ifdef DEBUG_MPI
			logger->writeLine("[DEBUG] Downloading link data at " + Util::secondsToTime(this->dCurrentTime));
			#endif
			for (unsigned int i = 0; i < this->pDomain->getDependentLinkCount(); i++)
			{
				this->pDomain->getDependentLink(i)->pullFromBuffer(this->dCurrentTime, this->getNextCellSourceBuffer());
			}
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
void	CSchemeGodunov::runSimulation(double dTargetTime, double dRealTime)
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


		this->uiQueueAdditionSize = static_cast<unsigned int>(max(static_cast<unsigned int>(1), min(this->uiBatchRate * 3,static_cast<unsigned int>(ceil(1.0 / (dBatchDuration / static_cast<double>(this->uiQueueAdditionSize)))))));
		
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
void	CSchemeGodunov::cleanupSimulation()
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
void	CSchemeGodunov::rollbackSimulation(double dCurrentTime, double dTargetTime)
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
bool	CSchemeGodunov::isSimulationFailure(double dExpectedTargetTime)
{
	if (bRunning)
		return false;

	// Can't exceed number of buffer cells in forecast mode
	if (this->syncMethod == model::syncMethod::kSyncForecast &&
		uiBatchSuccessful >= pDomain->getRollbackLimit() &&
		dExpectedTargetTime - dCurrentTime > 1E-5)
		return true;

	// This shouldn't happen
	if (this->syncMethod == model::syncMethod::kSyncTimestep &&
		uiBatchSuccessful > pDomain->getRollbackLimit())
		return true;

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
void	CSchemeGodunov::forceTimeAdvance()
{
	bUseForcedTimeAdvance = true;
}

/*
 *  Is the simulation ready to be synchronised?
 */
bool	CSchemeGodunov::isSimulationSyncReady(double dExpectedTargetTime)
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

	// Have we downloaded the data we need for each domain link?
	if (!bCellStatesSynced && this->domainCount > 1)
		return false;

	// Are we synchronising the timesteps?
	if (this->syncMethod == model::syncMethod::kSyncTimestep &&
		uiIterationsSinceSync < this->pDomain->getRollbackLimit() - 1 &&
		dExpectedTargetTime - dCurrentTime > 1E-5 &&
		dCurrentTime > 0.0)
		return false;

	//if ( uiIterationsSinceSync < this->pDomain->getRollbackLimit() )
	//	return false;

	// Assume success
	#ifdef DEBUG_MPI
		//logger->writeLine( "Domain is considered sync ready" );
	#endif

	return true;
}

/*
 *  Runs the actual simulation until completion or error
 */
void	CSchemeGodunov::scheduleIteration(
	bool			bUseAlternateKernel,
	COCLDevice* pDevice,
	CDomain* pDomain
)
{
	// Re-set the kernel arguments to use the correct cell state buffer
	if (bUseAlternateKernel)
	{
		oclKernelFullTimestep->assignArgument(2, oclBufferCellStatesAlt);
		oclKernelFullTimestep->assignArgument(3, oclBufferCellStates);
		oclKernelFriction->assignArgument(1, oclBufferCellStates);
		oclKernelTimestepReduction->assignArgument(3, oclBufferCellStates);
	}
	else {
		oclKernelFullTimestep->assignArgument(2, oclBufferCellStates);
		oclKernelFullTimestep->assignArgument(3, oclBufferCellStatesAlt);
		oclKernelFriction->assignArgument(1, oclBufferCellStatesAlt);
		oclKernelTimestepReduction->assignArgument(3, oclBufferCellStatesAlt);
	}

	// Run the boundary kernels (each bndy has its own kernel now)
	pDomain->getBoundaries()->applyBoundaries(bUseAlternateKernel ? oclBufferCellStatesAlt : oclBufferCellStates);
	pDevice->queueBarrier();

	// Main scheme kernel
	oclKernelFullTimestep->scheduleExecution();
	pDevice->queueBarrier();

	// Friction
	if (this->bFrictionEffects && !this->bFrictionInFluxKernel)
	{
		oclKernelFriction->scheduleExecution();
		pDevice->queueBarrier();
	}

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
void CSchemeGodunov::readDomainAll()
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
void CSchemeGodunov::importLinkZoneData()
{
	this->bImportLinks = true;
}

/*
*	Fetch the pointer to the last cell source buffer
*/
COCLBuffer* CSchemeGodunov::getLastCellSourceBuffer()
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
COCLBuffer* CSchemeGodunov::getNextCellSourceBuffer()
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
void CSchemeGodunov::saveCurrentState()
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
void CSchemeGodunov::setTargetTime(double dTime)
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
double CSchemeGodunov::proposeSyncPoint( double dCurrentTime )
{
	// TODO: Improve this so we're using more than just the current timestep...
	double dProposal = dCurrentTime + fabs(this->dTimestep);

	// Can only use this method once we have some simulation completed, not valid
	// at the start.
	if ( dCurrentTime > 1E-5 && uiBatchSuccessful > 0 )
	{
		// Try to accommodate approximately three spare iterations
		dProposal = dCurrentTime + 
			max(fabs(this->dTimestep), pDomain->getRollbackLimit() * (dBatchTimesteps / uiBatchSuccessful) * (((double)pDomain->getRollbackLimit() - this->syncBatchSpares) / pDomain->getRollbackLimit()));
		// Don't allow massive jumps
		//if ((dProposal - dCurrentTime) > dBatchTimesteps * 3.0)
		//	dProposal = dCurrentTime + dBatchTimesteps * 3.0;
		// If we've hit our rollback limit, use the time we reached to determine a conservative estimate for 
		// a new sync point
		if ( uiBatchSuccessful >= pDomain->getRollbackLimit() )
			dProposal = dCurrentTime + dBatchTimesteps * 0.95;
	} else {
		// Can't return a suggestion of not going anywhere..
		if ( dProposal - dCurrentTime < 1E-5 )
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
double CSchemeGodunov::getAverageTimestep()
{
	if (uiBatchSuccessful < 1) return 0.0;
	return dBatchTimesteps / uiBatchSuccessful;
}

/*
 *	Force a specific timestep (when synchronising them)
 */
void CSchemeGodunov::forceTimestep(double dTimestep)
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
void	CSchemeGodunov::readKeyStatistics()
{
	cl_uint uiLastBatchSuccessful = uiBatchSuccessful;

	// Pull key data back from our buffers to the scheme class
	if ( this->floatPrecision == model::floatPrecision::kSingle )
	{
		dCurrentTimestep = static_cast<cl_double>( *( oclBufferTimestep->getHostBlock<float*>() ) );
		dCurrentTime = static_cast<cl_double>(*(oclBufferTime->getHostBlock<float*>()));
		dBatchTimesteps = static_cast<cl_double>( *( oclBufferBatchTimesteps->getHostBlock<float*>() ) );
	} else {
		dCurrentTimestep = *( oclBufferTimestep->getHostBlock<double*>() );
		dCurrentTime = *(oclBufferTime->getHostBlock<double*>());
		dBatchTimesteps = *( oclBufferBatchTimesteps->getHostBlock<double*>() );
	}
	uiBatchSuccessful = *( oclBufferBatchSuccessful->getHostBlock<cl_uint*>() );
	uiBatchSkipped	  = *( oclBufferBatchSkipped->getHostBlock<cl_uint*>() );
	uiBatchRate = uiBatchSuccessful > uiLastBatchSuccessful ? (uiBatchSuccessful - uiLastBatchSuccessful) : 1;
}
