#pragma once
#include <iostream>
#include <iomanip>
#include "common.h"


class Normalplain {
public:
	unsigned long sizex;
	unsigned long sizey;
	unsigned long size;
	double** bedElevation;

	Normalplain(unsigned long, unsigned long);
	unsigned long getSize();
	unsigned long getSizeX();
	unsigned long getSizeY();
	double getBedElevation(unsigned long index);
	double getBedElevation(unsigned long, unsigned long);
	void setBedElevation(unsigned long, unsigned long, double);
	void setBedElevation(unsigned long index, double value);
	void setBedElevation(cl_double* src);
	void SetBedElevationMountain();
	double getManning(unsigned long);
	void outputShape();
};