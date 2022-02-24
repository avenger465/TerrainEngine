/**
* \class C Perlin Noise
*
* \brief Implementation of the original Perlin Noise algorithm
*
* This class takes most of the original C implementation of Perlin Noise from https://mrl.nyu.edu/~perlin/doc/oscar.html
* and builds it in a class for C++.
* As much as possible, the code has been kept the same as Ken's implementation, except where it would be overly confusing.
* There is much scope to improve this implementation, and this makes none of the suggested improvements from Ken's "Improved Perlin Noise"
* nor does it have the code for 3D Noise
*
* As such, this class is mostly for educational purposes.
*
* \author Gaz Robinson, from Ken Perlin
*/
#pragma once
#include <cstdlib>

//These should be constants, but I'm sticking to the original implementation for clarity
#define B	0x100	//256
#define BM	0xff	//255
#define N	0x1000	//4096

class Perlin {
public:
	//1D and 2D noise functions
	static double	noise1(double arg);
	static double	noise2(float vec[2]);


private:
	///Initialisation///
		//Does the class need to be initialised
	static bool		start;

	//Set up the permuation and gradient tables
	static int		p[B + B + 2];
	static float	g3[B + B + 2][3];
	static float	g2[B + B + 2][2];
	static float	g1[B + B + 2];

	static void		init(void);
	static void		setup(float* vec, int i, int& b0, int& b1, float& r0, float& r1);

	///Helper math functions///
	static const void	normalize2(float v[2]);
	static const void	normalize3(float v[3]);
	//Easing interpolation
	static const inline float	s_curve(float t) {
		return (t * t * (3. - 2. * t));
	}
	//Linear interpolation
	static const inline float	lerp(float t, float a, float b) { return (a + t * (b - a)); }
	static const inline float	dotProduct(float x1, float y1, float x2, float y2) {
		return  x1 * x2 + y1 * y2;
	}

	///Constructor/Destructor
		//Prevent instances being created
	Perlin() {}
	~Perlin() {}
};