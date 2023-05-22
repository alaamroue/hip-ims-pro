#include "Normalplain.h"

Normalplain::Normalplain(unsigned long sizex, unsigned long sizey) {
	this->sizex = sizex;
	this->sizey = sizey;
	this->size = sizex * sizey;

	// Allocate memory for the data array
	this->bedElevation = new float* [sizex];
	for (unsigned long i = 0; i < sizex; i++) {
		this->bedElevation[i] = new float[sizey];
	}

}

unsigned long Normalplain::getSizeX() {
	return this->sizex;
}

unsigned long Normalplain::getSizeY() {
	return this->sizey;
}

unsigned long Normalplain::getSize() {
	return this->size;
}

double Normalplain::getBedElevation(unsigned long index) {
	return this->bedElevation[(unsigned long)floor(index / this->getSizeX())][(unsigned long) index % this->getSizeY()];
}

float Normalplain::getBedElevation(unsigned long indexX, unsigned long indexY) {
	return this->bedElevation[indexX][indexY];
}

void Normalplain::setBedElevation(unsigned long indexX, unsigned long indexY, float value) {
	this->bedElevation[indexX][indexY] = value;
}

void Normalplain::setBedElevation(unsigned long index, float value) {
	this->bedElevation[(unsigned long)floor(index / this->getSizeX())][(unsigned long)index % this->getSizeY()] = value;
}

void Normalplain::setBedElevation(cl_double4* src) {
	for (unsigned long i = 0; i < 10; i++) {
		for (unsigned long j = 0; j < 10; j++) {
			this->setBedElevation(i, j, src[i * 10 + j].s[0]);
		}
	}
}

void Normalplain::SetBedElevationMountain() {
	unsigned long SizeX = this->getSizeX();
	float value;
	unsigned long const SIZE = this->getSize() / SizeX;
	for (unsigned long i = 0; i < SIZE; i++) {
		for (unsigned long j = 0; j < SIZE; j++) {
			value =  sqrt(i * i + j*j) / SizeX/10;
			if (i > SizeX * 0.7 && i< SizeX * 0.8 && j > SizeX * 0.7 && j < SizeX * 0.8)
				value = sqrt(2) / 10;

			this->setBedElevation(i, j, value);
		}
	}
}

void Normalplain::outputShape() {
	unsigned long size = this->getSize()/10;
	double value;
	Color::Modifier red(Color::FG_RED);
	Color::Modifier green(Color::FG_GREEN);
	Color::Modifier sgreen(Color::sFG_GREEN);
	Color::Modifier def(Color::FG_DEFAULT);
	std::cout << std::fixed;
	std::cout << std::setprecision(2);
	std::cout << std::endl;

	for (unsigned long i = 0; i < this->getSizeX() ; i++) {
		for (unsigned long j = 0; j < this->getSizeY(); j++) {
			value = this->getBedElevation(i, j);
			if (value-2 > 100+i*10+j) {
				std::cout << sgreen << value << def << " ";
			}
			else if (value > 100 + i * 10 + j) {
				std::cout << green << value << def << " ";
			
			}
			else if (value < 100 + i * 10 + j) {
				std::cout << red << value << def << " ";
			}
			else {
				std::cout << value << " ";
			
			}

		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

double Normalplain::getManning(unsigned long index) {
	return 0.0286;
}