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
 *  Benchmark class and structures for time-tracking
 * ------------------------------------------
 *
 */
#ifndef HIPIMS_GENERAL_CBENCHMARK_H_
#define HIPIMS_GENERAL_CBENCHMARK_H_

class CBenchmark
{

	public:

		CBenchmark( bool );									// Constructor
		~CBenchmark( void );								// Destructor

		// Public variables
		// ...

		// Public structures
		struct sPerformanceMetrics							// Performance return values
		{
			double dMilliseconds;
			double dSeconds;
			double dHours;
			double sTime;
		};

		// Public functions
		void					start( void );				// Start counting
		void					finish( void );				// End counting
		bool					isRunning( void ) { return bRunning; }		// Is it counting?
		sPerformanceMetrics*	getMetrics( void );			// Fetch the results

	private:

		// Private variables
		bool					bRunning;					// Is the timer currently running?
		double					dStartTime;					// Start of counting
		double					dEndTime;					// End of counting
		sPerformanceMetrics		sMetrics;					// The most recent metrics

		// Private functions
		double	getCurrentTime();							// Fetch the time in seconds from the CPU

};

#endif
