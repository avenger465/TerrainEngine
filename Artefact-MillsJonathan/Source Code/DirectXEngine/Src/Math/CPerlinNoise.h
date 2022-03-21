//----------------------------------------------//
// Perlin Noise Algorithm Created by Ken Perlin //
//----------------------------------------------//
#pragma once
#include "tepch.h"

//Linear Interpolate between two points with reference to time
#define LERP(t, a, b) (a + t * (b-a)) 

//Perform a DotProduct between two points
#define DOTPRODUCT(x1, y1, x2, y2) (x1 * x2 + y1 * y2)

class CPerlinNoise
{
	//The permutation List that will be used to get the noise values
	std::vector<int> permutationList;
public:

	//Constructor with a Seed 
	CPerlinNoise(unsigned int seed);

	//Deconstructor
	~CPerlinNoise() {}

	//The Noise function to generate a value at the selected position
	double noise(double x, double y, double z);

private:

	double fade(double t);

	//Create a gradient with the position
	double grad(int hash, double x, double y, double z);
};