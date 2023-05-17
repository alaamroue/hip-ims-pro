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
 *  CSV handling class
 * ------------------------------------------
 *
 */
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

#include "../common.h"
#include "CCSVDataset.h"

/* 
 *  Constructor
 */
CCSVDataset::CCSVDataset(
		std::string		sCSVFilename
	)
{
	sFilename	= sCSVFilename;
	bReadFile	= false;
}

/*
 *  Destructor
 */
CCSVDataset::~CCSVDataset()
{
	// ...
}


bool CCSVDataset::readFile()
{
	return true;
}
