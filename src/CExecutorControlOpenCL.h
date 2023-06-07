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
 *  Handles cascading execution and enumeration 
 *  on OpenCL devices
 * ------------------------------------------
 *
 */
#ifndef HIPIMS_OPENCL_EXECUTORS_CEXECUTORCONTROLOPENCL_H_
#define HIPIMS_OPENCL_EXECUTORS_CEXECUTORCONTROLOPENCL_H_

#include <vector>
#include "CExecutorControl.h"
#include "opencl.h"

// Type aliases
typedef char*						OCL_RAW_CODE;
typedef std::vector<char*>			OCL_CODE_STACK;

class COCLDevice;

/*
 *  [OPENCL IMPLEMENTATION]
 *  EXECUTOR CONTROL CLASS
 *  CExecutorControl
 *
 *  Controls the model execution
 */
class CExecutorControlOpenCL: public CExecutorControl
{

	public:

		CExecutorControlOpenCL( void);								// Constructor
		~CExecutorControlOpenCL( void );							// Destructor

		// Public variables
		// ...

		// Friendships
		friend class			COCLDevice;							// Allow the devices access to these private vars

		// Public functions
		COCLDevice*				getDevice();						// Fetch the currently selected device
		COCLDevice*				getDevice( unsigned int );			// Fetch a device pointer
		void					selectDevice();						// Automatically select the best device
		void					selectDevice( unsigned int );		// Manually select the device to use for execution
		OCL_RAW_CODE			getOCLCode( std::string );			// Fetch OpenCL code stored in our resources
		bool					createDevices( void );				// Creates new classes for each device
		unsigned int			getDeviceCount( void )		{ return clDeviceTotal; }		// Returns the number of devices in the system
		unsigned int			getDeviceCurrent( void )	{ return uiSelectedDeviceID; }	// Returns the active device
		void					logPlatforms(void);						// Write platform details to the log

	private:

		// Private structs
		struct		sPlatformInfo
		{
			char*				cProfile;
			char*				cVersion;
			char*				cName;
			char*				cVendor;
			char*				cExtensions;
			cl_uint				uiDeviceCount;
		};

		// Private variables
		sPlatformInfo*			platformInfo;					// Platform details
		cl_platform_id *  	clPlatforms;					// Array of OpenCL platforms
		cl_uint					clPlatformCount;				// Number of platforms
		cl_uint					clDeviceTotal;					// Total number of devices
		std::vector<COCLDevice*>							// Dynamic array of device controller classes
								pDevices;				
		unsigned int			uiSelectedDeviceID;				// The selected device for use in execution

		// Private functions
		char*					getPlatformInfo( unsigned int, cl_platform_info );	// Fetches information about the platform
		bool					getPlatforms( void );						// Discovers the platforms available
};

#endif
