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
#ifndef HIPIMS_GENERAL_CLOG_H_
#define HIPIMS_GENERAL_CLOG_H_

// Includes
#include <iostream>
#include <ctime>
#include <fstream>
#include <sstream>
#include <locale>

// Namespaces

/*
 *  LOGGING CLASS
 *  CLog
 *
 *  Is a singleton class in reality, but need not be enforced.
 */
class CLog
{

	public:

		CLog( void );									// Constructor
		~CLog( void );									// Destructor

		// Public functions
		void		writeLine( std::string );				// Timestamp parameter is optional
		void		writeLine( std::string, bool );			// Output to the console & file, specify timestamp
		void		writeLine( std::string, bool, unsigned short );	// Output to the console with a colour
		void		writeError( std::string, unsigned char );	// Display an error message
		void		writeDivide( void );					// Write a line to break up the output
		void		writeDebugFile( char**, unsigned int );	// Write a debug file to the log directory
		void		clearFile();							// Clear the log file out
		void		openFile();								// Opens the log file
		void		closeFile();							// Closes the log file
		bool		isFileAvailable();						// Is the file output available?
		unsigned int getLineCount()	{ return uiLineCount; }	// Fetch no. of lines written

		void		setColour( unsigned short );			// Set the console colour
		void		resetColour();							// Reset the console colour
		void		setPath();								// Set path to default
		void		setPath( char*, size_t );				// Set path to given string
		std::string	getPath();								// Returns the path
		
	private:

		// Private variables
		char*			logPath;							// Full path to the log file
		char*			logDir;								// Directory for log files
		std::ofstream	logStream;							// Handle for the log file stream
		unsigned int	uiDebugFileID;						// Incremental tracking for debug files output
		unsigned int	uiLineCount;						// Number of lines written
#ifdef MPI_ON
		int				iProcID;							// MPI process ID
		int				iProcCount;							// MPI process count
#endif

		// Private functions
		void		writeHeader( void );					// Information about the application

};

#endif