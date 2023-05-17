#pragma once
#include <iostream>
#include <iomanip>


class Normalplain {
public:
	unsigned long sizex;
	unsigned long sizey;
	unsigned long size;
	float** bedElevation;

	Normalplain(unsigned long, unsigned long);
	unsigned long getSize();
	unsigned long getSizeX();
	unsigned long getSizeY();
	double getBedElevation(unsigned long index);
	float getBedElevation(unsigned long, unsigned long);
	void setBedElevation(unsigned long, unsigned long, float);
	void setBedElevation(unsigned long index, float value);
	void SetBedElevationMountain();
	void SetBedElevationMountainDef();
	void outputShape();
	double getManning(unsigned long index);
};