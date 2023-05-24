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
 *  Benchmark functionality for time-tracking
 * ------------------------------------------
 *
 */

// Includes
#include "common.h"
#include "CBenchmark.h"

/*
 *  Constructor
 */
CBenchmark::CBenchmark( bool bStart )
{
	this->bRunning = false;

	if ( bStart )
		this->start();
}

/*
 *  Destructor
 */
CBenchmark::~CBenchmark(void)
{
	// ...
}

/*
 *  Fetch the current time and return the value in seconds
 */
double CBenchmark::getCurrentTime()
{
	/*
	 *  Performance counters vary from one platform to
	 *  another.
	 */
	BOOL bQueryState;

	LARGE_INTEGER liPerfCount, liPerfFreq;
	double dPerfCount, dPerfFreq;

	// Trap errors (1=success)
	bQueryState  = QueryPerformanceCounter( &liPerfCount );
	bQueryState += QueryPerformanceFrequency( &liPerfFreq );

	if ( bQueryState < 2 )
		model::doError(
			"A high performance time counter cannot be obtained on this system.",
			model::errorCodes::kLevelFatal
		);

	// Deal with signed 64-bit ints
	dPerfCount = (double)liPerfCount.QuadPart;
	dPerfFreq  = (double)liPerfFreq.QuadPart;

	// Adjust for seconds
	return dPerfCount / dPerfFreq;

}

/*
 *  Start counting
 */
void CBenchmark::start()
{
	this->dStartTime = this->getCurrentTime();
	this->dEndTime	 = 0;
	this->bRunning	 = true;
}

/*
 *  End the counting
 */
void CBenchmark::finish()
{
	if ( !this->bRunning ) 
		return;

	this->dEndTime = this->getCurrentTime();
	this->bRunning = false;
}

/*
 *  Fetch all the result metric values in a structure
 */
CBenchmark::sPerformanceMetrics* CBenchmark::getMetrics()
{
	if ( this->bRunning )
		this->dEndTime = this->getCurrentTime();

	this->sMetrics.dSeconds			= this->dEndTime - this->dStartTime;
	this->sMetrics.dMilliseconds	= sMetrics.dSeconds * 1000;
	this->sMetrics.dHours			= sMetrics.dSeconds / 3600;

	return &this->sMetrics;
}
