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
 *  Domain boundary handling class
 * ------------------------------------------
 *
 */
#ifndef HIPIMS_BOUNDARIES_CBOUNDARYUNIFORM_H_
#define HIPIMS_BOUNDARIES_CBOUNDARYUNIFORM_H_

#include "common.h"
#include "CBoundary.h"

class CBoundaryUniform : public CBoundary
{
public:
	CBoundaryUniform( CDomain* = NULL );
	~CBoundaryUniform();

	//virtual bool					setupFromConfig();
	virtual void					prepareBoundary(COCLDevice*, COCLProgram*, COCLBuffer*, COCLBuffer*,
													COCLBuffer*, COCLBuffer*, COCLBuffer*);
	virtual void					applyBoundary(COCLBuffer*);
	virtual void					streamBoundary(double);
	virtual void					cleanBoundary();
	void							setValue(unsigned char a)			{ ucValue = a; };
	void							setVariablesBasedonData();

	//void							importTimeseries(CCSVDataset*);

	unsigned char					ucValue;

	double							dTotalVolume;
	double							dTimeseriesLength;
	double							dTimeseriesInterval;

	struct sTimeseriesUniform
	{
		cl_double		dTime;
		cl_double		dComponent;	
	};

	sTimeseriesUniform*				pTimeseries;
	unsigned int					uiTimeseriesLength;

	COCLBuffer*						pBufferTimeseries;
	COCLBuffer*						pBufferConfiguration;

	struct sConfigurationSP
	{
		cl_uint			TimeseriesEntries;
		cl_float		TimeseriesInterval;
		cl_float		TimeseriesLength;
		cl_uint			Definition;
	};
	struct sConfigurationDP
	{
		cl_uint			TimeseriesEntries;
		cl_double		TimeseriesInterval;
		cl_double		TimeseriesLength;
		cl_uint			Definition;
	};

	int size;
};

#endif
