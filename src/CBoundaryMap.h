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
 *  Domain boundary mapping class (i.e. cells - timeseries etc)
 * ------------------------------------------
 *
 */
#ifndef HIPIMS_BOUNDARIES_CBOUNDARYMAP_H_
#define HIPIMS_BOUNDARIES_CBOUNDARYMAP_H_

#include <vector>
#include "common.h"
#include "opencl.h"
#include "COCLProgram.h"

using std::unordered_map;

// Class stubs
class CBoundary;
class CDomain;
class COCLBuffer;
class COCLDevice;
class COCLProgram;
class COCLKernel;

class CBoundaryMap
{
public:
	CBoundaryMap( CDomain* );
	~CBoundaryMap();

	//bool							setupFromConfig();
	//CBoundary*						getBoundaryByName( std::string );

	void							prepareBoundaries( COCLProgram*, COCLBuffer*, COCLBuffer*, COCLBuffer*, COCLBuffer*, COCLBuffer* );
	void							applyBoundaries( COCLBuffer* );
	void							streamBoundaries( double );

	unsigned int					getBoundaryCount();
	void							applyDomainModifications();
	CDomain*						pDomain;
	typedef unordered_map<std::string, CBoundary*> mapBoundaries_t;
	mapBoundaries_t					mapBoundaries;

private:	
	
	unsigned char					ucBoundaryTreatment[4];

};

#endif
