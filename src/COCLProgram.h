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
 *  OpenCL program
 * ------------------------------------------
 *
 */
#ifndef HIPIMS_OPENCL_OCLPROGRAM_H
#define HIPIMS_OPENCL_OCLPROGRAM_H

#include <unordered_map>

#include "CExecutorControlOpenCL.h"
#include "COCLDevice.h"

class COCLKernel;
class COCLProgram
{
public:
	COCLProgram(
		CExecutorControlOpenCL*	execController,
		COCLDevice*	device
	);
	~COCLProgram();
	CExecutorControlOpenCL*		getController()							{ return execController; }
	COCLDevice*					getDevice()								{ return device; }
	cl_context					getContext()							{ return clContext; }
	bool						isCompiled()							{ return bCompiled; }
	bool						compileProgram( bool = true );
	void						appendCode( OCL_RAW_CODE );
	void						prependCode( OCL_RAW_CODE );
	void						appendCodeFromResource( std::string );
	void						prependCodeFromResource( std::string );
	void						clearCode();
	COCLKernel*					getKernel( const char * );
	std::string					getCompileLog();
	void						addCompileParameter( std::string );
	bool						registerConstant( std::string, std::string );
	bool						removeConstant( std::string );		
	void						clearConstants();		
	void						setForcedSinglePrecision( bool );
	unsigned char				getFloatForm()						{ return ( bForceSinglePrecision ? model::floatPrecision::kSingle : model::floatPrecision::kDouble ); };
	unsigned char				getFloatSize()						{ return ( bForceSinglePrecision ? sizeof( cl_float ) : sizeof( cl_double ) ); };
	CLog* logger;

protected:
	OCL_RAW_CODE				getConstantsHeader( void );		
	OCL_RAW_CODE				getExtensionsHeader( void );			

	CExecutorControlOpenCL*		execController;
	COCLDevice*					device;
	cl_context					clContext;
	cl_program					clProgram;
	OCL_CODE_STACK				oclCodeStack;
	bool						bCompiled;
	bool						bForceSinglePrecision;
	std::string					sCompileParameters;
	std::unordered_map<std::string,std::string>					
								uomConstants;

friend class COCLKernel;
friend class COCLBuffer;
};

#endif
