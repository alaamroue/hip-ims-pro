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
#include <boost/lexical_cast.hpp>

#include "../common.h"
#include "../Boundaries/CBoundaryMap.h"
#include "../Boundaries/CBoundary.h"
#include "CScheme.h"
#include "CSchemeGodunov.h"
#include "CSchemeMUSCLHancock.h"
#include "CSchemeInertial.h"
#include "../Domain/CDomain.h"
#include "../Domain/Cartesian/CDomainCartesian.h"
#include "../Datasets/CXMLDataset.h"
#include "../Datasets/CRasterDataset.h"

/*
 *  Default constructor
 */
CScheme::CScheme()
{
	pManager->log->writeLine( "Scheme base-class instantiated." );

	// Not ready by default
	this->bReady				= false;
	this->bRunning				= false;
	this->bThreadRunning		= false;
	this->bThreadTerminated		= false;

	this->bAutomaticQueue		= true;
	this->uiQueueAdditionSize	= 1;
	this->dCourantNumber		= 0.5;
	this->dTimestep				= 0.001;
	this->bDynamicTimestep		= true;
	this->bFrictionEffects		= true;
	this->dTargetTime			= 0.0;
	this->uiBatchSkipped		= 0;
	this->uiBatchSuccessful		= 0;
	this->dBatchTimesteps		= 0.0;
}

/*
 *  Destructor
 */
CScheme::~CScheme(void)
{
	pManager->log->writeLine( "The abstract scheme class was unloaded from memory." );
}


/*
 *  Simple check for if the scheme is ready to run
 */
bool	CScheme::isReady()
{
	return this->bReady;
}

/*
 *  Set the queue mode
 */
void	CScheme::setQueueMode( unsigned char ucQueueMode )
{
	this->bAutomaticQueue = ( ucQueueMode == model::queueMode::kAuto );
}

/*
 *  Get the queue mode
 */
unsigned char	CScheme::getQueueMode()
{
	return ( this->bAutomaticQueue ? model::queueMode::kAuto : model::queueMode::kFixed );
}

/*
 *  Set the queue size (or initial)
 */
void	CScheme::setQueueSize( unsigned int uiQueueSize )
{
	this->uiQueueAdditionSize = uiQueueSize;
}

/*
 *  Get the queue size (or initial)
 */
unsigned int	CScheme::getQueueSize()
{
	return this->uiQueueAdditionSize;
}

/*
 *  Set the Courant number
 */
void	CScheme::setCourantNumber( double dCourantNumber )
{
	this->dCourantNumber = dCourantNumber;
}

/*
 *  Get the Courant number
 */
double	CScheme::getCourantNumber()
{
	return this->dCourantNumber;
}

/*
 *  Set the timestep mode
 */
void	CScheme::setTimestepMode( unsigned char ucMode )
{
	this->bDynamicTimestep = ( ucMode == model::timestepMode::kCFL );
}

/*
 *  Get the timestep mode
 */
unsigned char	CScheme::getTimestepMode()
{
	return ( this->bDynamicTimestep ? model::timestepMode::kCFL : model::timestepMode::kFixed );
}

/*
 *  Set the timestep
 */
void	CScheme::setTimestep( double dTimestep )
{
	this->dTimestep = dTimestep;
}

/*
 *  Get the timestep
 */
double	CScheme::getTimestep()
{
	return fabs(this->dTimestep);
}

/*
 *  Enable/disable friction effects
 */
void	CScheme::setFrictionStatus( bool bEnabled )
{
	this->bFrictionEffects = bEnabled;
}

/*
 *  Get enabled/disabled for friction
 */
bool	CScheme::getFrictionStatus()
{
	return this->bFrictionEffects;
}

/*
 *  Set the target time
 */
void	CScheme::setTargetTime( double dTime )
{
	this->dTargetTime = dTime;
}

/*
 *  Get the target time
 */
double	CScheme::getTargetTime()
{
	return this->dTargetTime;
}

/*
 *	Are we in the middle of a batch?
 */
bool	CScheme::isRunning()
{
	return bRunning;
}