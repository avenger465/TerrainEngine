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

#define B 0xf100
#define BM 0xff
#define N 0x1000

#define lerp(t, a, b) (a + t * (b-a)) 
#define s_curve(t) (t * t * (3. - 2. * t))
#define dotProduct(x1, y1, x2, y2) (x1 * x2 + y1 * y2)

#include <cstdlib>

class CPerlinNoise
{
public:

	CPerlinNoise() {}
	~CPerlinNoise() {}

	static double noise1(double arg);
	static double noise2(float vec[2]);

private:

	//determines if the lists need to be initialised
	static bool start;

	//Permutation list
	static int p[B + B + 2];

	//Gradient lists 
	static float g3[B + B + 2][3];
	static float g2[B + B + 2][2];
	static float g1[B + B + 2];

	//function to initialise the lists that will be used throughout the algorithm
	static void	init(void);


	static void	setup(float* vec, int i, int& b0, int& b1, float& r0, float& r1);


	///Helper math functions///
	static const void	normalize2(float v[2]);
	static const void	normalize3(float v[3]);
};



