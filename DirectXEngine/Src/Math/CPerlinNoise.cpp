#include "CPerlinNoise.h"

CPerlinNoise::CPerlinNoise(unsigned int seed)
{
	//Resize the Permutation List
	permutationList.resize(256);

	// Fill the permutationList with values from 0 to 255
	std::iota(permutationList.begin(), permutationList.end(), 0);

	// Initialize a random engine with seed
	std::default_random_engine engine(seed);

	// Randomly shuffle the Permutation List
	std::shuffle(permutationList.begin(), permutationList.end(), engine);

	// Duplicate the permutation vector
	permutationList.insert(permutationList.end(), permutationList.begin(), permutationList.end());
}

double CPerlinNoise::noise(double x, double y, double z)
{
	// Find the unit cube that contains the point
	int X = (int)floor(x) & 255;
	int Y = (int)floor(y) & 255;
	int Z = (int)floor(z) & 255;

	// Find relative x, y,z of point in cube
	x -= floor(x);
	y -= floor(y);
	z -= floor(z);

	// Compute fade curves for each of x, y, z
	double u = fade(x);
	double v = fade(y);
	double w = fade(z);

	// Hash coordinates of the 8 cube corners
	int A = permutationList[X] + Y;
	int AA = permutationList[A] + Z;
	int AB = permutationList[A + 1] + Z;
	int B = permutationList[X + 1] + Y;
	int BA = permutationList[B] + Z;
	int BB = permutationList[B + 1] + Z;

	// Add blended results from 8 corners of cube
	double res = LERP(w, LERP(v, LERP(u, grad(permutationList[AA], x, y, z), grad(permutationList[BA], x - 1, y, z)), LERP(u, grad(permutationList[AB], x, y - 1, z), grad(permutationList[BB], x - 1, y - 1, z))), LERP(v, LERP(u, grad(permutationList[AA + 1], x, y, z - 1), grad(permutationList[BA + 1], x - 1, y, z - 1)), LERP(u, grad(permutationList[AB + 1], x, y - 1, z - 1), grad(permutationList[BB + 1], x - 1, y - 1, z - 1))));
	return (res + 1.0) / 2.0;
}

double CPerlinNoise::fade(double t)
{
	return t * t * t * (t * (t * 6 - 15) + 10);
}

//Create a gradient with the position
double CPerlinNoise::grad(int hash, double x, double y, double z)
{
	int h = hash & 15;
	// Convert lower 4 bits of hash into 12 gradient directions
	double u = h < 8 ? x : y,
		v = h < 4 ? y : h == 12 || h == 14 ? x : z;
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}