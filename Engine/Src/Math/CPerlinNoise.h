/*
* 
* Class implementing Perlin noise
* 
* This class is mainly implemented from https://mrl.nyu.edu/~perlin/doc/oscar.html
* and converts it to C++
* 
* |author Ken Perlin|
*
*/
#pragma once
#include "tepch.h"

#define LERP(t, a, b) (a + t * (b-a)) 
#define S_CURVE(t) (t * t * (3. - 2. * t))
#define DOTPRODUCT(x1, y1, x2, y2) (x1 * x2 + y1 * y2)

#include <cstdlib>

class CPerlinNoise
{
	std::vector<int> p;
public:

	CPerlinNoise();
	CPerlinNoise(unsigned int seed);
	~CPerlinNoise() {}

	double noise(double x, double y, double z);

private:

	double fade(double t);
	double grad(int hash, double x, double y, double z);
	//determines if the lists need to be initialised

	///Helper math functions///
	static const void	normalize2(float v[2]);
	static const void	normalize3(float v[3]);
};