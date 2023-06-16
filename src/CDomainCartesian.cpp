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
 *  Domain Class: Cartesian grid
 * ------------------------------------------
 *
 */
#include <limits>
#include <stdio.h>
#include <cstring>

#include "common.h"
#include "CModel.h"
#include "CDomainManager.h"
#include "CDomain.h"
#include "CScheme.h"

#include "CExecutorControlOpenCL.h"
#include "CDomainCartesian.h"

/*
 *  Constructor
 */
CDomainCartesian::CDomainCartesian(void)
{
	// Default values will trigger errors on validation
	this->dCellResolution			= std::numeric_limits<double>::quiet_NaN();
	this->ulRows					= std::numeric_limits<unsigned long>::quiet_NaN();
	this->ulCols					= std::numeric_limits<unsigned long>::quiet_NaN();
}

/*
 *  Destructor
 */
CDomainCartesian::~CDomainCartesian(void)
{
	// ...
}


/*
 *  Does the domain contain all of the required data yet?
 */
bool	CDomainCartesian::validateDomain( bool bQuiet )
{
	// Got a resolution?
	if ( this->dCellResolution == NAN )
	{
		if ( !bQuiet ) model::doError(
			"Domain cell resolution not defined",
			model::errorCodes::kLevelWarning
		);
		return false;
	}

	if (this->ulRows == NAN || this->ulCols == NAN)
	{
		if (!bQuiet) model::doError(
			"Rows/Cols have not been defined",
			model::errorCodes::kLevelWarning
		);
		return false;
	}

	return true;
}

/*
 *  Create the required data structures etc.
 */
void	CDomainCartesian::prepareDomain()
{
	if ( !this->validateDomain( true ) ) 
	{
		model::doError(
			"Cannot prepare the domain. Invalid specification.",
			model::errorCodes::kLevelModelStop
		);
		return;
	}

	this->bPrepared = true;
	this->logDetails();
}

/*
 *  Write useful information about this domain to the log file
 */
void	CDomainCartesian::logDetails()
{
	model::log->writeDivide();
	unsigned short	wColour			= model::cli::colourInfoBlock;

	model::log->writeLine( "REGULAR CARTESIAN GRID DOMAIN", true, wColour );
	if ( this->ulProjectionCode > 0 ) 
	{
		model::log->writeLine( "  Projection:        EPSG:" + toString( this->ulProjectionCode ), true, wColour );
	} else {
		model::log->writeLine( "  Projection:        Unknown", true, wColour );
	}
	model::log->writeLine( "  Device number:     " + toString( this->pDevice->uiDeviceNo ), true, wColour );
	model::log->writeLine( "  Cell count:        " + toString( this->ulCellCount ), true, wColour );
	model::log->writeLine( "  Cell resolution:   " + toString( this->dCellResolution ) + this->cUnits, true, wColour );
	model::log->writeLine( "  Cell dimensions:   [" + toString( this->ulCols ) + ", " + 
														 toString( this->ulRows ) + "]", true, wColour );
	model::log->writeLine( "  Real dimensions:   [" + toString( this->dRealDimensions[ kAxisX ] ) + this->cUnits + ", " + 
														 toString( this->dRealDimensions[ kAxisY ] ) + this->cUnits + "]", true, wColour );

	model::log->writeDivide();
}

/*
 *  Set real domain dimensions (X, Y)
 */
void	CDomainCartesian::setRealDimensions( double dSizeX, double dSizeY )
{
	this->dRealDimensions[ kAxisX ]	= dSizeX;
	this->dRealDimensions[ kAxisY ]	= dSizeY;
	this->updateCellStatistics();
}

/*	
 *  Fetch real domain dimensions (X, Y)
 */
void	CDomainCartesian::getRealDimensions( double* dSizeX, double* dSizeY )
{
	*dSizeX = this->dRealDimensions[ kAxisX ];
	*dSizeY = this->dRealDimensions[ kAxisY ];
}

/*
 *  Set real domain offset (X, Y) for lower-left corner
 */
void	CDomainCartesian::setRealOffset( double dOffsetX, double dOffsetY )
{
	this->dRealOffset[ kAxisX ]	= dOffsetX;
	this->dRealOffset[ kAxisY ]	= dOffsetY;
}

/*
 *  Fetch real domain offset (X, Y) for lower-left corner
 */
void	CDomainCartesian::getRealOffset( double* dOffsetX, double* dOffsetY )
{
	*dOffsetX = this->dRealOffset[ kAxisX ];
	*dOffsetY = this->dRealOffset[ kAxisY ];
}

/*
 *  Set real domain extent (Clockwise: N, E, S, W)
 */
void	CDomainCartesian::setRealExtent( double dEdgeN, double dEdgeE, double dEdgeS, double dEdgeW )
{
	this->dRealExtent[ kEdgeN ]	= dEdgeN;
	this->dRealExtent[ kEdgeE ]	= dEdgeE;
	this->dRealExtent[ kEdgeS ]	= dEdgeS;
	this->dRealExtent[ kEdgeW ]	= dEdgeW;
	//this->updateCellStatistics();
}

/*
 *  Fetch real domain extent (Clockwise: N, E, S, W)
 */
void	CDomainCartesian::getRealExtent( double* dEdgeN, double* dEdgeE, double* dEdgeS, double* dEdgeW ) 
{
	*dEdgeN = this->dRealExtent[kEdgeN];
	*dEdgeE = this->dRealExtent[kEdgeE];
	*dEdgeS = this->dRealExtent[kEdgeS];
	*dEdgeW = this->dRealExtent[kEdgeW];
}

/*
 *  Set cell resolution
 */
void	CDomainCartesian::setCellResolution( double dResolution )
{
	this->dCellResolution = dResolution;
	this->updateCellStatistics();
}

/*
 *   Fetch cell resolution
 */
void	CDomainCartesian::getCellResolution( double* dResolution )
{
	*dResolution = this->dCellResolution;
}

/*
 *  Set domain units
 */
void	CDomainCartesian::setUnits( char* cUnits )
{
	if ( std::strlen( cUnits ) > 2 ) 
	{
		model::doError(
			"Domain units can only be two characters",
			model::errorCodes::kLevelWarning
		);
		return;
	}

	// Store the units (2 char max!)
	std::strcpy( &this->cUnits[0], cUnits );
}

/*
 *  Return a couple of characters representing the units in use
 */
char*	CDomainCartesian::getUnits()
{
	return &this->cUnits[0];
}

/*
 *  Set the EPSG projection code currently in use
 *  0 = Not defined
 */
void	CDomainCartesian::setProjectionCode( unsigned long ulProjectionCode )
{
	this->ulProjectionCode = ulProjectionCode;
}

/*
 *  Return the EPSG projection code currently in use
 */
unsigned long	CDomainCartesian::getProjectionCode()
{
	return this->ulProjectionCode;
}

/*
 *  Update basic statistics on the number of cells etc.
 */
void	CDomainCartesian::updateCellStatistics()
{
	// Do we have enough information to proceed?...

	// Got a resolution?
	if ( this->dCellResolution == NAN )
	{
		return;
	}

	// Got a size?
	if ( ( std::isnan( this->dRealDimensions[ kAxisX ] ) || 
		   std::isnan( this->dRealDimensions[ kAxisY ] ) ) &&
		 ( std::isnan( this->dRealExtent[ kEdgeN ] ) || 
		   std::isnan( this->dRealExtent[ kEdgeE ] ) || 
		   std::isnan( this->dRealExtent[ kEdgeS ] ) || 
		   std::isnan( this->dRealExtent[ kEdgeW ] ) ) )
	{
		return;
	}

	// We've got everything we need...
	this->ulRows	= static_cast<unsigned long>(
		this->dRealDimensions[ kAxisY ] / this->dCellResolution
	);
	this->ulCols	= static_cast<unsigned long>(
		this->dRealDimensions[ kAxisX ] / this->dCellResolution
	);
	this->ulCellCount = this->ulRows * this->ulCols;
}

/*
 *  
 */
void	CDomainCartesian::setCols(unsigned long value)
{
	this->ulCols = value;
	this->updateCellStatistics();
}

/*
 *  
 */
void	CDomainCartesian::setRows(unsigned long value)
{
	this->ulRows = value;
	this->updateCellStatistics();
}

/*
 *  Return the total number of rows in the domain
 */
unsigned long	CDomainCartesian::getRows()
{
	return this->ulRows;
}

/*
 *  Return the total number of columns in the domain
 */
unsigned long	CDomainCartesian::getCols()
{
	return this->ulCols;
}

/*
 *  Get a cell ID from an X and Y index
 */
unsigned long	CDomainCartesian::getCellID( unsigned long ulX, unsigned long ulY )
{
	return ( ulY * this->getCols() ) + ulX;
}

/*
 *  Get a cell ID from an X and Y coordinate
 */
unsigned long	CDomainCartesian::getCellFromCoordinates( double dX, double dY )
{
	unsigned long ulX	= floor( ( dX - dRealOffset[ 0 ] ) / dCellResolution );
	unsigned long ulY	= floor( ( dY - dRealOffset[ 2 ] ) / dCellResolution );
	return getCellID( ulX, ulY );
}




/*
 *  Calculate the total volume present in all of the cells
 */
double	CDomainCartesian::getVolume()
{
	double dVolume = 0.0;

	for( unsigned int i = 0; i < this->ulCellCount; ++i )
	{
		if ( this->isDoublePrecision() )
		{
			dVolume += ( this->dCellStates[i].s[0] - this->dBedElevations[i] ) *
					   this->dCellResolution * this->dCellResolution;
		} else {
			dVolume += ( this->fCellStates[i].s[0] - this->fBedElevations[i] ) *
					   this->dCellResolution * this->dCellResolution;
		}
	}

	return dVolume;
}

/*
 *  Add a new output
 */
void	CDomainCartesian::addOutput( sDataTargetInfo pOutput )
{
	this->pOutputs.push_back( pOutput );
}

/*
 *  Manipulate the topography to impose boundary conditions
 */
void	CDomainCartesian::imposeBoundaryModification(unsigned char ucDirection, unsigned char ucType)
{
	unsigned long ulMinX, ulMaxX, ulMinY, ulMaxY;

	if (ucDirection == edge::kEdgeE) 
		{ ulMinY = 0; ulMaxY = this->ulRows - 1; ulMinX = this->ulCols - 1; ulMaxX = this->ulCols - 1; };
	if (ucDirection == edge::kEdgeW)
		{ ulMinY = 0; ulMaxY = this->ulRows - 1; ulMinX = 0; ulMaxX = 0; };
	if (ucDirection == edge::kEdgeN)
		{ ulMinY = this->ulRows - 1; ulMaxY = this->ulRows - 1; ulMinX = 0; ulMaxX = this->ulCols - 1; };
	if (ucDirection == edge::kEdgeS)
		{ ulMinY = 0; ulMaxY = 0; ulMinX = 0; ulMaxX = this->ulCols - 1; };

	for (unsigned long x = ulMinX; x <= ulMaxX; x++)
	{
		for (unsigned long y = ulMinY; y <= ulMaxY; y++)
		{
			if (ucType == CDomainCartesian::boundaryTreatment::kBoundaryClosed)
			{
				this->setBedElevation(
					this->getCellID( x, y ),
					9999.9
				);
			}
		}
	}
}

/*
 *  Write output files to disk
 */
void	CDomainCartesian::writeOutputs()
{
	// Read the data back first...
	// TODO: Review whether this is necessary, isn't it a sync point anyway?
	pDevice->blockUntilFinished();
	pScheme->readDomainAll();
	pDevice->blockUntilFinished();

	//TODO: Alaa handle reads
}

/*
 *	Fetch summary information for this domain
 */
CDomainBase::DomainSummary CDomainCartesian::getSummary()
{
	CDomainBase::DomainSummary pSummary;
	
	pSummary.bAuthoritative = true;

	pSummary.uiDomainID		= this->uiID;
	pSummary.uiNodeID = 0;
	pSummary.uiLocalDeviceID = this->getDevice()->getDeviceID();
	pSummary.dEdgeNorth		= this->dRealExtent[kEdgeN];
	pSummary.dEdgeEast		= this->dRealExtent[kEdgeE];
	pSummary.dEdgeSouth		= this->dRealExtent[kEdgeS];
	pSummary.dEdgeWest		= this->dRealExtent[kEdgeW];
	pSummary.ulColCount		= this->ulCols;
	pSummary.ulRowCount		= this->ulRows;
	pSummary.ucFloatPrecision = ( this->isDoublePrecision() ? model::floatPrecision::kDouble : model::floatPrecision::kSingle );
	pSummary.dResolution	= this->dCellResolution;

	return pSummary;
}

