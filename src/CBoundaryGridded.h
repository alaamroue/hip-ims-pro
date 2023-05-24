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
#ifndef HIPIMS_BOUNDARIES_CBOUNDARYGRIDDED_H_
#define HIPIMS_BOUNDARIES_CBOUNDARYGRIDDED_H_

#include "common.h"
#include "CBoundary.h"

class CBoundaryGridded : public CBoundary
{
public:
	CBoundaryGridded(CDomain* = NULL);
	~CBoundaryGridded();

	virtual bool					setupFromConfig();
	virtual void					prepareBoundary(COCLDevice*, COCLProgram*, COCLBuffer*, COCLBuffer*,
													COCLBuffer*, COCLBuffer*, COCLBuffer*);
	virtual void					applyBoundary(COCLBuffer*);
	virtual void					streamBoundary(double);
	virtual void					cleanBoundary();
	void							setValue(unsigned char a)				{ ucValue = a; };

	struct SBoundaryGridTransform
	{
		double			dSourceResolution;
		double			dTargetResolution;
		double			dOffsetSouth;
		double			dOffsetWest;
		unsigned int	uiRows;
		unsigned int	uiColumns;
		unsigned long	ulBaseSouth;
		unsigned long	ulBaseWest;
	};

	class CBoundaryGriddedEntry
	{
	public:
		CBoundaryGriddedEntry(double, double*);
		~CBoundaryGriddedEntry();

		double			dTime;
		double*			dValues;
		void*			getBufferData(unsigned char, SBoundaryGridTransform*);
	};

protected:

	struct sConfigurationSP
	{
		cl_float		TimeseriesInterval;
		cl_float		GridResolution;
		cl_float		GridOffsetX;
		cl_float		GridOffsetY;
		cl_ulong		TimeseriesEntries;
		cl_ulong		Definition;
		cl_ulong		GridRows;
		cl_ulong		GridCols;
	};
	struct sConfigurationDP
	{
		cl_double		TimeseriesInterval;
		cl_double		GridResolution;
		cl_double		GridOffsetX;
		cl_double		GridOffsetY;
		cl_ulong		TimeseriesEntries;
		cl_ulong		Definition;
		cl_ulong		GridRows;
		cl_ulong		GridCols;
	};


	unsigned char					ucValue;

	double							dTotalVolume;
	double							dTimeseriesLength;
	double							dTimeseriesInterval;

	CBoundaryGriddedEntry**			pTimeseries;
	SBoundaryGridTransform*			pTransform;
	unsigned int					uiTimeseriesLength;

	COCLBuffer*						pBufferTimeseries;
	COCLBuffer*						pBufferConfiguration;
};

#endif
