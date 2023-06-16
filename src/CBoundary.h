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
 *  Domain boundary handling class
 * ------------------------------------------
 *
 */
#ifndef HIPIMS_BOUNDARIES_CBOUNDARY_H_
#define HIPIMS_BOUNDARIES_CBOUNDARY_H_

#include "common.h"
#include "CCSVDataset.h"

#define BOUNDARY_DEPTH_IGNORE			0
#define BOUNDARY_DEPTH_IS_FSL			1
#define BOUNDARY_DEPTH_IS_DEPTH			2
#define BOUNDARY_DEPTH_IS_CRITICAL		3

#define BOUNDARY_DISCHARGE_IGNORE		0
#define BOUNDARY_DISCHARGE_IS_DISCHARGE	1
#define BOUNDARY_DISCHARGE_IS_VELOCITY	2
#define BOUNDARY_DISCHARGE_IS_VOLUME	3


// Class stubs
class COCLBuffer;
class COCLDevice;
class COCLProgram;
class COCLKernel;

class CBoundary
{
public:
	CBoundary( CDomain* = NULL );
	~CBoundary();

	virtual bool					setupFromConfig(XMLElement*, std::string) = 0;
	virtual void					prepareBoundary(COCLDevice*, COCLProgram*, COCLBuffer*, COCLBuffer*,
												    COCLBuffer*, COCLBuffer*, COCLBuffer*) = 0;
	virtual void					applyBoundary(COCLBuffer*) = 0;
	virtual void					streamBoundary(double) = 0;
	virtual void					cleanBoundary() = 0;
	virtual void					importMap(CCSVDataset*)				{};
	std::string						getName()							{ return sName; };

	static int			uiInstances;

protected:	

	CDomain*			pDomain;
	COCLKernel*			oclKernel;
	std::string			sName;

	/*
	unsigned int		iType;
	unsigned int		iDepthValue;
	unsigned int		iDischargeValue;
	unsigned int		uiTimeseriesLength;
	cl_double			dTimeseriesInterval;

	double				dTotalVolume;

	friend class CBoundaryMap;
	*/
};

#endif
