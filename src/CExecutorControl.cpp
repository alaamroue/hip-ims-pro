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
 *  Executor Control Base Class
 * ------------------------------------------
 *
 */

// TODO: Remove this class entirely (we only deal with OpenCL
// executors for the purposes of this model now).

// Includes
#include "common.h"

#include "CExecutorControl.h"
#include "CExecutorControlOpenCL.h"

/*
 *  Constructor
 */
CExecutorControl::CExecutorControl(void)
{
	// This stub class is not enough to be ready
	// The child classes should change this value when they're
	// done initialising.
	this->setState( model::executorStates::executorError );
}

/*
 *  Destructor
 */
CExecutorControl::~CExecutorControl(void)
{
	// ...
}

/*
 *  Create a new executor of the specified type (static func)
 */
CExecutorControl*	CExecutorControl::createExecutor( unsigned char cType, CModel* cModel)
{
	switch ( cType )
	{
		case model::executorTypes::executorTypeOpenCL:
			return new CExecutorControlOpenCL(cModel);
		break;
	}

	return NULL;
}

/*
 *  Is this executor ready to run models?
 */
bool CExecutorControl::isReady( void )
{
	return this->state == model::executorStates::executorReady;
}

/*
 *  Set the ready state of this executor
 */
void CExecutorControl::setState( unsigned int iState )
{
	this->state = iState;
}

/*
 *  Set the device filter to determine what types of device we
 *  can use to execute the model.
 */
void CExecutorControl::setDeviceFilter( unsigned int uiFilters )
{
	this->deviceFilter = uiFilters;
}

/*
 *  Return any current device filters
 */
unsigned int CExecutorControl::getDeviceFilter()
{
	return this->deviceFilter;
}

