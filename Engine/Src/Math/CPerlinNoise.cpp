#include "CPerlinNoise.h"


//bool	CPerlinNoise::start = true;
////int		CPerlinNoise::p[B + B + 2];
//float	CPerlinNoise::g3[B + B + 2][3];
//float	CPerlinNoise::g2[B + B + 2][2];
//float	CPerlinNoise::g1[B + B + 2];

CPerlinNoise::CPerlinNoise()
{
	p = {
		151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
		8,99,37,240,21,10,23,190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
		35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,
		134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
		55,46,245,40,244,102,143,54, 65,25,63,161,1,216,80,73,209,76,132,187,208, 89,
		18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,
		250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
		189,28,42,223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167,
		43,172,9,129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,
		97,228,251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,
		107,49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
		138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180 };
	// Duplicate the permutation vector
	p.insert(p.end(), p.begin(), p.end());
}

CPerlinNoise::CPerlinNoise(unsigned int seed)
{
	p.resize(256);

	// Fill p with values from 0 to 255
	std::iota(p.begin(), p.end(), 0);

	// Initialize a random engine with seed
	std::default_random_engine engine(seed);

	// Suffle  using the above random engine
	std::shuffle(p.begin(), p.end(), engine);

	// Duplicate the permutation vector
	p.insert(p.end(), p.begin(), p.end());
}

//double CPerlinNoise::noise1(double arg)
//{
//	int bx0;
//	int bx1;
//
//	float rx0;
//	float rx1;
//
//	float sx;
//
//	float u;
//	float v;
//	float vec[1]{ arg };
//
//	if (start) {
//		start = false;
//		init();
//	}
//
//	//Initialisation of the variables
//	setup(vec, 0, bx0, bx1, rx0, rx1);
//
//
//	sx = S_CURVE(rx0);
//
//	u = rx0 * g1[p[bx0]];
//	v = rx1 * g1[p[bx1]];
//
//	//blending the two values based on the position on the curve
//	return LERP(sx, u, v);
//}

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
	int A = p[X] + Y;
	int AA = p[A] + Z;
	int AB = p[A + 1] + Z;
	int B = p[X + 1] + Y;
	int BA = p[B] + Z;
	int BB = p[B + 1] + Z;

	// Add blended results from 8 corners of cube
	double res = LERP(w, LERP(v, LERP(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)), LERP(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))), LERP(v, LERP(u, grad(p[AA + 1], x, y, z - 1), grad(p[BA + 1], x - 1, y, z - 1)), LERP(u, grad(p[AB + 1], x, y - 1, z - 1), grad(p[BB + 1], x - 1, y - 1, z - 1))));
	return (res + 1.0) / 2.0;
}

//double CPerlinNoise::noise2(float vec[2])
//{
//
//	int bx0 = 0, bx1 = 0, by0 = 0, by1 = 0;
//
//	
//	float rx0 = 0.f, rx1 = 0.f, ry0 = 0.f, ry1 = 0.f;
//
//
//	int b00 = 0, b10 = 0, b01 = 0, b11 = 0;
//
//	float sx = 0.f, sy = 0.f;
//
//	float* q, a = 0.f, b = 0.f, u = 0.f, v = 0.f;
//	int i, j;
//
//	if (start) {
//		start = 0;
//		init();
//	}
//
//	//Initialisation of the variables along the xy axis
//	setup(vec, 0, bx0, bx1, rx0, rx1);
//	setup(vec, 1, by0, by1, ry0, ry1);
//
//	//Get a pseudo-random number for the x boundaries
//	i = p[bx0];
//	j = p[bx1];
//
//	b00 = p[i + by0];
//	b10 = p[j + by0];
//	b01 = p[i + by1];
//	b11 = p[j + by1];
//
//	sx = S_CURVE(rx0);
//	sy = S_CURVE(ry0);
//
//	q = g2[b00];
//	u = DOTPRODUCT(rx0, ry0, q[0], q[1]);	
//	q = g2[b10];
//	v = DOTPRODUCT(rx1, ry0, q[0], q[1]);
//	a = LERP(sx, u, v);
//
//	q = g2[b01];
//	u = DOTPRODUCT(rx0, ry1, q[0], q[1]);
//	q = g2[b11];
//	v = DOTPRODUCT(rx1, ry1, q[0], q[1]);
//	b = LERP(sx, u, v);
//
//	//blending the final two values
//	return LERP(sy, a, b);
//}

double CPerlinNoise::fade(double t)
{
	return t * t * t * (t * (t * 6 - 15) + 10);
}

double CPerlinNoise::grad(int hash, double x, double y, double z)
{
	int h = hash & 15;
	// Convert lower 4 bits of hash into 12 gradient directions
	double u = h < 8 ? x : y,
		v = h < 4 ? y : h == 12 || h == 14 ? x : z;
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

//void CPerlinNoise::init(void)
//{
//	int i, j, k;
//
//	for (i = 0; i < B; i++) {
//		p[i] = i;
//
//		g1[i] = (float)((rand() % (B + B)) - B) / B;
//
//		for (j = 0; j < 2; j++)
//			g2[i][j] = (float)((rand() % (B + B)) - B) / B;
//		normalize2(g2[i]);
//
//		for (j = 0; j < 3; j++)
//			g3[i][j] = (float)((rand() % (B + B)) - B) / B;
//		normalize3(g3[i]);
//	}
//
//	while (--i) {
//		k = p[i];
//		p[i] = p[j = rand() % B];
//		p[j] = k;
//	}
//
//	for (i = 0; i < B + 2; i++) {
//		p[B + i] = p[i];
//		g1[B + i] = g1[i];
//		for (j = 0; j < 2; j++)
//			g2[B + i][j] = g2[i][j];
//		for (j = 0; j < 3; j++)
//			g3[B + i][j] = g3[i][j];
//	}
//}


//void CPerlinNoise::setup(float* vec, int i, int& b0, int& b1, float& r0, float& r1)
//{
//	float t = vec[i] + N;
//	b0 = ((int)t) & BM;
//	b1 = (b0 + 1) & BM;
//	r0 = t - (int)t;
//	r1 = r0 - 1.;
//}

//Normalisation of a 2D vector
const void CPerlinNoise::normalize2(float v[2])
{
	float s = sqrt(v[0] * v[0] + v[1] * v[1]);
	v[0] = v[0] / s;
	v[1] = v[1] / s;
}

//Normalisation of a 3D vector
const void CPerlinNoise::normalize3(float v[3])
{
	float s = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] = v[0] / s;
	v[1] = v[1] / s;
	v[2] = v[2] / s;
}