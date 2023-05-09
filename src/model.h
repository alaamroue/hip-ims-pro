#pragma once
namespace model
{

	// Application author details
	const std::string appName = "High-performance Integrated Modelling System";
	const std::string appAuthor = "Luke S. Smith and Qiuhua Liang";
	const std::string appContact = "luke@smith.ac";
	const std::string appUnit = "School of Civil Engineering and Geosciences";
	const std::string appOrganisation = "Newcastle University";
	const std::string appRevision = "$Revision: 717 $";

	// Application version details
	const unsigned int appVersionMajor = 0;	// Major 
	const unsigned int appVersionMinor = 2;	// Minor
	const unsigned int appVersionRevision = 0;	// Revision

	// Data structures used in interop
	struct DomainData
	{
		double			dResolution;
		double			dWidth;
		double			dHeight;
		double			dCornerWest;
		double			dCornerSouth;
		unsigned long	ulCellCount;
		unsigned long	ulRows;
		unsigned long	ulCols;
		unsigned long	ulBoundaryCells;
		unsigned long	ulBoundaryOthers;
	};


	//extern	CModel* pManager;

}