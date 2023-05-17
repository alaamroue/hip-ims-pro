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
 *  Raster dataset handling class
 * ------------------------------------------
 *
 */
#include <boost/lexical_cast.hpp>
#include <algorithm>

#include "../common.h"
#include "CRasterDataset.h"
#include "../Domain/Cartesian/CDomainCartesian.h"

using std::min;
using std::max;

/*
 *  Default constructor
 */
CRasterDataset::CRasterDataset()
{
	this->bAvailable = false;
}

/*
 *  Destructor
 */
CRasterDataset::~CRasterDataset(void)
{

}

/*
 *  Register types before doing anything else in GDAL
 */
void	CRasterDataset::registerAll()
{

}

/*
 *  Cleanup all memory allocations (except GDAL still has leaks...)
 */
void	CRasterDataset::cleanupAll()
{

}

/*
 *  Open a file as the underlying dataset
 */
bool	CRasterDataset::openFileRead( std::string sFilename )
{


	return true;
}

/*
 *  Open a file for output
 */
bool	CRasterDataset::domainToRaster( 
			const char*			cDriver, 
			std::string			sFilename,
			CDomainCartesian*	pDomain,
			unsigned char		ucValue
		)
{
	return true;
}

/*
 *  Read in metadata about the layer (e.g. resolution data)
 */
void	CRasterDataset::readMetadata()
{

	// TODO: Spatial reference systems (EPSG codes etc.) once needed by CDomain
}

/*
 *  Write details to the log file
 */
void	CRasterDataset::logDetails()
{
	if ( !this->bAvailable ) return;

	pManager->log->writeDivide();
	pManager->log->writeLine( "Dataset driver:      " + std::string( this->cDriverDescription ) );
	pManager->log->writeLine( "Dataset driver name: " + std::string( this->cDriverLongName ) );
	pManager->log->writeLine( "Dataset band count:  " + toString( this->uiBandCount ) );
	pManager->log->writeLine( "Cell dimensions:     [" + toString( this->ulColumns ) + ", " + toString( this->ulRows ) + "]" );
	pManager->log->writeLine( "Cell resolution:     [" + toString( this->dResolutionX ) + ", " + toString( this->dResolutionY ) + "]" );
	pManager->log->writeLine( "Lower-left offset:   [" + toString( this->dOffsetX ) + ", " + toString( this->dOffsetY ) + "]" );
	pManager->log->writeDivide();
}

/*
 *  Apply the dimensions taken from the raster dataset to form the domain class structure
 */
bool	CRasterDataset::applyDimensionsToDomain( CDomainCartesian*	pDomain )
{
	if ( !this->bAvailable ) return false;

	pManager->log->writeLine( "Dimensioning domain from raster dataset." );

	pDomain->setProjectionCode( 0 );					// Unknown
	pDomain->setUnits( "m" );							
	pDomain->setCellResolution( this->dResolutionX );	
	pDomain->setRealDimensions( this->dResolutionX * this->ulColumns, this->dResolutionY * this->ulRows );	
	pDomain->setRealOffset( this->dOffsetX, this->dOffsetY );			
	pDomain->setRealExtent( this->dOffsetY + this->dResolutionY * this->ulRows,
						    this->dOffsetX + this->dResolutionX * this->ulColumns,
							this->dOffsetY,
							this->dOffsetX );

	return true;
}

/*
 *  Apply some data to the domain from this raster's first band
 */
bool	CRasterDataset::applyDataToDomain( unsigned char ucValue, CDomainCartesian* pDomain )
{

	return true;
}

/*
 *  Is the domain the right dimension etc. to apply data from this raster?
 */
bool	CRasterDataset::isDomainCompatible( CDomainCartesian* pDomain )
{
	if ( pDomain->getCols() != this->ulColumns ) return false;
	if ( pDomain->getRows() != this->ulRows) return false;
	
	// Assume yes for now
	// TODO: Add extra checks
	return true;
}

/*
 *	Create a transformation structure between the loaded raster dataset and
 *	a gridded boundary
 */
CBoundaryGridded::SBoundaryGridTransform* CRasterDataset::createTransformationForDomain(CDomainCartesian* pDomain)
{
	CBoundaryGridded::SBoundaryGridTransform *pReturn = new CBoundaryGridded::SBoundaryGridTransform;
	double dDomainExtent[4], dDomainResolution;

	pDomain->getCellResolution(&dDomainResolution);
	pDomain->getRealExtent( &dDomainExtent[0], &dDomainExtent[1], &dDomainExtent[2], &dDomainExtent[3] );

	pReturn->dSourceResolution = this->dResolutionX;
	pReturn->dTargetResolution = dDomainResolution;

	pReturn->dOffsetWest  = -( fmod( (dDomainExtent[3] - this->dOffsetX), this->dResolutionX ) );
	pReturn->dOffsetSouth = -( fmod( (dDomainExtent[2] - this->dOffsetY), this->dResolutionX ) );

	pReturn->uiColumns = (unsigned int)( ceil(dDomainExtent[1] / this->dResolutionX) - floor(dDomainExtent[3] / this->dResolutionX) );
	pReturn->uiRows    = (unsigned int)( ceil(dDomainExtent[0] / this->dResolutionX) - floor(dDomainExtent[2] / this->dResolutionX) );

	// TODO: Add proper check here to make sure the two domains actually align and fully overlap
	// otherwise we're likely to segfault.

	pReturn->ulBaseWest  = (unsigned long)max( 0.0, floor( ( dDomainExtent[3] - this->dOffsetX ) / this->dResolutionX ) );
	pReturn->ulBaseSouth = (unsigned long)max( 0.0, floor( ( dDomainExtent[2] - this->dOffsetY ) / this->dResolutionX ) );

	return pReturn;
}

/*
 *	Create an array for use as a boundary condition
 */
double*		CRasterDataset::createArrayForBoundary( CBoundaryGridded::SBoundaryGridTransform *sTransform )
{

	return nullptr;
}

/*
 *  Get some details for a data source type, like a full name we can use in the log
 */
void	CRasterDataset::getValueDetails( unsigned char ucValue, std::string* sValueName )
{
	switch( ucValue )
	{
	case model::rasterDatasets::dataValues::kBedElevation:
		*sValueName = "bed elevation";
		break;
	case model::rasterDatasets::dataValues::kDepth:
		*sValueName  = "depth";
		break;
	case model::rasterDatasets::dataValues::kFreeSurfaceLevel:
		*sValueName  = "free-surface level";
		break;
	case model::rasterDatasets::dataValues::kVelocityX:
		*sValueName  = "velocity in X-direction";
		break;
	case model::rasterDatasets::dataValues::kVelocityY:
		*sValueName  = "velocity in Y-direction";
		break;
	case model::rasterDatasets::dataValues::kDischargeX:
		*sValueName  = "discharge in X-direction";
		break;
	case model::rasterDatasets::dataValues::kDischargeY:
		*sValueName  = "discharge in Y-direction";
		break;
	case model::rasterDatasets::dataValues::kManningCoefficient:
		*sValueName = "manning coefficients";
		break;
	case model::rasterDatasets::dataValues::kDisabledCells:
		*sValueName  = "disabled cells";
		break;
	case model::rasterDatasets::dataValues::kMaxFSL:
		*sValueName  = "maximum FSL";
		break;
	case model::rasterDatasets::dataValues::kMaxDepth:
		*sValueName  = "maximum depth";
		break;
	case model::rasterDatasets::dataValues::kFroudeNumber:
		*sValueName  = "froude number";
		break;
	default:
		*sValueName  = "unknown value";
		break;
	}
}