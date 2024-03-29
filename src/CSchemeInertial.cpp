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
 *  Inertial formulation (i.e. simplified)
 * ------------------------------------------
 *
 */
#include <algorithm>

#include "common.h"
#include "CDomain.h"
#include "CDomainCartesian.h"
#include "CSchemeInertial.h"

using std::min;
using std::max;

/*
 *  Constructor
 */
CSchemeInertial::CSchemeInertial(void)
{
	// Scheme is loaded
	model::log->writeLine( "Inertial scheme loaded for execution on OpenCL platform." );

	// Default setup values
	this->bDebugOutput					= false;
	this->uiDebugCellX					= 100;
	this->uiDebugCellY					= 100;

	this->ucSolverType					= model::solverTypes::kHLLC;
	this->ucConfiguration				= model::schemeConfigurations::inertialFormula::kCacheNone;
	this->ucCacheConstraints			= model::cacheConstraints::inertialFormula::kCacheActualSize;
}

/*
 *  Destructor
 */
CSchemeInertial::~CSchemeInertial(void)
{
	this->releaseResources();
	model::log->writeLine( "The inertial formula scheme was unloaded from memory." );
}

/*
 *  Run all preparation steps
 */
void CSchemeInertial::prepareAll()
{
	// Clean any pre-existing OpenCL objects
	this->releaseResources();

	oclModel = new COCLProgram(
		cModel->getExecutor(),
		cModel->getExecutor()->getDevice()
	);

	// Run-time tracking values
	this->ulCurrentCellsCalculated = 0;
	this->dCurrentTimestep = this->dTimestep;
	this->dCurrentTime = 0;

	// Forcing single precision?
	this->oclModel->setForcedSinglePrecision(cModel->getFloatPrecision() == model::floatPrecision::kSingle);
	unsigned char ucFloatSize = (cModel->getFloatPrecision() == model::floatPrecision::kSingle ? sizeof(cl_double) : sizeof(cl_float));

	// OpenCL elements
	if (!this->prepare1OExecDimensions())
	{
		model::doError(
			"Failed to dimension 1st-order task elements. Cannot continue.",
			model::errorCodes::kLevelModelStop
		);
		this->releaseResources();
		return;
	}

	if (!this->prepare1OConstants())
	{
		model::doError(
			"Failed to allocate 1st-order constants. Cannot continue.",
			model::errorCodes::kLevelModelStop
		);
		this->releaseResources();
		return;
	}

	if (!this->prepareInertialConstants())
	{
		model::doError(
			"Failed to allocate inertial constants. Cannot continue.",
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
			"Failed to create 1st-order memory buffers. Cannot continue.",
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
	if (!this->prepareInertialKernels())
	{
		model::doError(
			"Failed to prepare inertial kernels. Cannot continue.",
			model::errorCodes::kLevelModelStop
		);
		this->releaseResources();
		return;
	}


	this->logDetails();
	this->bReady = true;
}

/*
 *  Log the details and properties of this scheme instance.
 */
void CSchemeInertial::logDetails()
{
	model::log->writeDivide();
	unsigned short wColour = model::cli::colourInfoBlock;

	std::string sConfiguration = "Undefined";
	switch (this->ucConfiguration)
	{
	case model::schemeConfigurations::inertialFormula::kCacheNone:
		sConfiguration = "Disabled";
		break;
	case model::schemeConfigurations::inertialFormula::kCacheEnabled:
		sConfiguration = "Enabled";
		break;
	}

	model::log->writeLine("SIMPLIFIED INERTIAL FORMULATION SCHEME", true, wColour);
	model::log->writeLine("  Timestep mode:      " + (std::string)(this->bDynamicTimestep ? "Dynamic" : "Fixed"), true, wColour);
	model::log->writeLine("  Courant number:     " + (std::string)(this->bDynamicTimestep ? toStringExact(this->dCourantNumber) : "N/A"), true, wColour);
	model::log->writeLine("  Initial timestep:   " + Util::secondsToTime(this->dTimestep), true, wColour);
	model::log->writeLine("  Data reduction:     " + toStringExact(this->uiTimestepReductionWavefronts) + " divisions", true, wColour);
	model::log->writeLine("  Configuration:      " + sConfiguration, true, wColour);
	model::log->writeLine("  Friction effects:   " + (std::string)(this->bFrictionEffects ? "Enabled" : "Disabled"), true, wColour);
	model::log->writeLine("  Kernel queue mode:  " + (std::string)(this->bAutomaticQueue ? "Automatic" : "Fixed size"), true, wColour);
	model::log->writeLine((std::string)(this->bAutomaticQueue ? "  Initial queue:      " : "  Fixed queue:        ") + toStringExact(this->uiQueueAdditionSize) + " iteration(s)", true, wColour);
	model::log->writeLine("  Debug output:       " + (std::string)(this->bDebugOutput ? "Enabled" : "Disabled"), true, wColour);

	model::log->writeDivide();
}

/*
 *  Concatenate together the code for the different elements required
 */
bool CSchemeInertial::prepareCode()
{
	bool bReturnState = true;

	oclModel->appendCodeFromResource("CLDomainCartesian_H");
	oclModel->appendCodeFromResource("CLFriction_H");
	oclModel->appendCodeFromResource("CLDynamicTimestep_H");
	oclModel->appendCodeFromResource("CLSchemeInertial_H");
	oclModel->appendCodeFromResource("CLBoundaries_H");

	oclModel->appendCodeFromResource("CLDomainCartesian_C");
	oclModel->appendCodeFromResource("CLFriction_C");
	oclModel->appendCodeFromResource("CLDynamicTimestep_C");
	oclModel->appendCodeFromResource("CLSchemeInertial_C");
	oclModel->appendCodeFromResource("CLBoundaries_C");

	bReturnState = oclModel->compileProgram();

	return bReturnState;
}

/*
 *  Allocate constants using the settings herein
 */
bool CSchemeInertial::prepareInertialConstants()
{
	//CDomainCartesian*	pDomain	= static_cast<CDomainCartesian*>( this->pDomain );

	// --
	// Size of local cache arrays
	// --

	switch (this->ucCacheConstraints)
	{
	case model::cacheConstraints::inertialFormula::kCacheActualSize:
		oclModel->registerConstant("INE_DIM1", std::to_string(this->ulCachedWorkgroupSizeX));
		oclModel->registerConstant("INE_DIM2", std::to_string(this->ulCachedWorkgroupSizeY));
		break;
	case model::cacheConstraints::inertialFormula::kCacheAllowUndersize:
		oclModel->registerConstant("INE_DIM1", std::to_string(this->ulCachedWorkgroupSizeX));
		oclModel->registerConstant("INE_DIM2", std::to_string(this->ulCachedWorkgroupSizeY));
		break;
	case model::cacheConstraints::inertialFormula::kCacheAllowOversize:
		oclModel->registerConstant("INE_DIM1", std::to_string(this->ulCachedWorkgroupSizeX));
		oclModel->registerConstant("INE_DIM2", std::to_string(this->ulCachedWorkgroupSizeY == 16 ? 17 : ulCachedWorkgroupSizeY));
		break;
	}

	return true;
}

/*
 *  Create kernels using the compiled program
 */
bool CSchemeInertial::prepareInertialKernels()
{
	bool						bReturnState = true;
	CExecutorControlOpenCL* pExecutor = cModel->getExecutor();
	CDomain*					pDomain				= this->pDomain;
	COCLDevice*		pDevice				= pExecutor->getDevice();

	// --
	// Inertial scheme kernels
	// --

	if ( this->ucConfiguration == model::schemeConfigurations::inertialFormula::kCacheNone )
	{
		oclKernelFullTimestep = oclModel->getKernel( "ine_cacheDisabled" );
		oclKernelFullTimestep->setGroupSize( this->ulNonCachedWorkgroupSizeX, this->ulNonCachedWorkgroupSizeY );
		oclKernelFullTimestep->setGlobalSize( this->ulNonCachedGlobalSizeX, this->ulNonCachedGlobalSizeY );
		COCLBuffer* aryArgsFullTimestep[] = { oclBufferTimestep, oclBufferCellBed, oclBufferCellStates, oclBufferCellStatesAlt, oclBufferCellManning, oclBufferUsePoleni, oclBuffer_opt_zxmax, oclBuffer_opt_cx, oclBuffer_opt_zymax, oclBuffer_opt_cy };
		oclKernelFullTimestep->assignArguments( aryArgsFullTimestep );
	}
	if ( this->ucConfiguration == model::schemeConfigurations::inertialFormula::kCacheEnabled )
	{
		oclKernelFullTimestep = oclModel->getKernel( "ine_cacheEnabled" );
		oclKernelFullTimestep->setGroupSize( this->ulCachedWorkgroupSizeX, this->ulCachedWorkgroupSizeY );
		oclKernelFullTimestep->setGlobalSize( this->ulCachedGlobalSizeX, this->ulCachedGlobalSizeY );
		COCLBuffer* aryArgsFullTimestep[] = { oclBufferTimestep, oclBufferCellBed, oclBufferCellStates, oclBufferCellStatesAlt, oclBufferCellManning };	
		oclKernelFullTimestep->assignArguments( aryArgsFullTimestep );
	}

	return bReturnState;
}

/*
 *  Release all OpenCL resources consumed using the OpenCL methods
 */
void CSchemeInertial::releaseResources()
{
	this->bReady = false;

	model::log->writeLine("Releasing scheme resources held for OpenCL.");

	this->releaseInertialResources();
	this->release1OResources();
}

/*
 *  Release all OpenCL resources consumed using the OpenCL methods
 */
void CSchemeInertial::releaseInertialResources()
{
	this->bReady = false;

	model::log->writeLine("Releasing inertial scheme resources held for OpenCL.");

	// Nothing to do?
}

/*
 *  Set the cache configuration to use
 */
void	CSchemeInertial::setCacheMode( unsigned char ucCacheMode )
{
	this->ucConfiguration = ucCacheMode;
}

/*
 *  Get the cache configuration in use
 */
unsigned char	CSchemeInertial::getCacheMode()
{
	return this->ucConfiguration;
}

/*
 *  Set the cache constraints
 */
void	CSchemeInertial::setCacheConstraints( unsigned char ucCacheConstraints )
{
	this->ucCacheConstraints = ucCacheConstraints;
}

/*
 *  Get the cache constraints
 */
unsigned char	CSchemeInertial::getCacheConstraints()
{
	return this->ucCacheConstraints;
}
