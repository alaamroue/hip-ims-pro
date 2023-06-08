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
 *  Domain Class: Cartesian grid
 * ------------------------------------------
 *
 */
#ifndef HIPIMS_DOMAIN_CARTESIAN_CDOMAINCARTESIAN_H_
#define HIPIMS_DOMAIN_CARTESIAN_CDOMAINCARTESIAN_H_

#include "CDomain.h"

/*
 *  DOMAIN CLASS
 *  CDomainCartesian
 *
 *  Stores the relevant information required for a
 *  domain based on a simple cartesian grid.
 */
class CDomainCartesian : public CDomain 
{

	public:
		CDomainCartesian(CModel* cModel);													// 
		~CDomainCartesian( void );												// Destructor

		// Public variables
		// ...

		// Public functions
		// - Replacements for CDomain stubs
		virtual	unsigned char	getType()										{ return model::domainStructureTypes::kStructureCartesian; };	// Fetch a type code
		virtual	CDomainBase::DomainSummary getSummary();						// Fetch summary information for this domain
		void			logDetails();											// Log details about the domain
		void			readDomain();											// Read Output from gpu
		double*			readDomain_opt_h();										// Read Output flow hieght from gpu to double*
		double*			readDomain_opt_dsdt();									// Read Output dsdt value from gpu to double*
		double*			readDomain_vx();										// Read Output velocity in x-direction from gpu to double*
		double*			readDomain_vy();										// Read Output velocity in y-direction from gpu to double*
		// - Specific to cartesian grids
		void			setRowsCount( unsigned long );							// Set number of cells in a row
		void			setColsCount( unsigned long );							// Set number of cells in a columns
		void			setRealOffset( double, double );						// Set real domain offset (X, Y) for lower-left corner
		void			getRealOffset( double*, double* );						// Fetch real domain offset (X, Y) for lower-left corner
		void			setCellResolution( double );							// Set cell resolution
		void			getCellResolution( double* );							// Fetch cell resolution
		unsigned long	getRows();												// Get the number of rows in the domain
		unsigned long	getCols();												// Get the number of columns in the domain
		virtual unsigned long	getCellID( unsigned long, unsigned long );		// Get the cell ID using an X and Y index
		double			getVolume();											// Calculate the amount of volume in all the cells


	private:

		// Private variables
		double			dCellResolution;
		unsigned long	ulRows;
		unsigned long	ulCols;


};

#endif
