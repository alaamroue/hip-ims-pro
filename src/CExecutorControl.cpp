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


CExecutorControl::CExecutorControl()
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
CExecutorControl*	CExecutorControl::createExecutor( unsigned char cType)
{
	switch ( cType )
	{
		case model::executorTypes::executorTypeOpenCL:
			return new CExecutorControlOpenCL();
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

