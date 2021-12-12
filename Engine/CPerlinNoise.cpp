#include "CPerlinNoise.h"


bool	CPerlinNoise::start = true;
int		CPerlinNoise::p[B + B + 2];
float	CPerlinNoise::g3[B + B + 2][3];
float	CPerlinNoise::g2[B + B + 2][2];
float	CPerlinNoise::g1[B + B + 2];

double CPerlinNoise::noise1(double arg)
{
	int bx0;
	int bx1;

	float rx0;
	float rx1;

	float sx;

	float u;
	float v;
	float vec[1]{ arg };

	if (start) {
		start = false;
		init();
	}

	//Initialisation of the variables
	setup(vec, 0, bx0, bx1, rx0, rx1);


	sx = s_curve(rx0);

	u = rx0 * g1[p[bx0]];
	v = rx1 * g1[p[bx1]];

	//blending the two values based on the position on the curve
	return lerp(sx, u, v);
}

double CPerlinNoise::noise2(float vec[2])
{

	int bx0 = 0, bx1 = 0, by0 = 0, by1 = 0;

	
	float rx0 = 0.f, rx1 = 0.f, ry0 = 0.f, ry1 = 0.f;


	int b00 = 0, b10 = 0, b01 = 0, b11 = 0;

	float sx = 0.f, sy = 0.f;

	float* q, a = 0.f, b = 0.f, u = 0.f, v = 0.f;
	int i, j;

	if (start) {
		start = 0;
		init();
	}

	//Initialisation of the variables along the xy axis
	setup(vec, 0, bx0, bx1, rx0, rx1);
	setup(vec, 1, by0, by1, ry0, ry1);

	//Get a pseudo-random number for the x boundaries
	i = p[bx0];
	j = p[bx1];

	b00 = p[i + by0];
	b10 = p[j + by0];
	b01 = p[i + by1];
	b11 = p[j + by1];

	sx = s_curve(rx0);
	sy = s_curve(ry0);

	q = g2[b00];
	u = dotProduct(rx0, ry0, q[0], q[1]);	
	q = g2[b10];
	v = dotProduct(rx1, ry0, q[0], q[1]);	
	a = lerp(sx, u, v);						

	q = g2[b01];
	u = dotProduct(rx0, ry1, q[0], q[1]);	
	q = g2[b11];
	v = dotProduct(rx1, ry1, q[0], q[1]);	
	b = lerp(sx, u, v);						

	//blending the final two values
	return lerp(sy, a, b);
}

void CPerlinNoise::init(void)
{
	int i, j, k;

	for (i = 0; i < B; i++) {
		p[i] = i;

		g1[i] = (float)((rand() % (B + B)) - B) / B;

		for (j = 0; j < 2; j++)
			g2[i][j] = (float)((rand() % (B + B)) - B) / B;
		normalize2(g2[i]);

		for (j = 0; j < 3; j++)
			g3[i][j] = (float)((rand() % (B + B)) - B) / B;
		normalize3(g3[i]);
	}

	while (--i) {
		k = p[i];
		p[i] = p[j = rand() % B];
		p[j] = k;
	}

	for (i = 0; i < B + 2; i++) {
		p[B + i] = p[i];
		g1[B + i] = g1[i];
		for (j = 0; j < 2; j++)
			g2[B + i][j] = g2[i][j];
		for (j = 0; j < 3; j++)
			g3[B + i][j] = g3[i][j];
	}
}


void CPerlinNoise::setup(float* vec, int i, int& b0, int& b1, float& r0, float& r1)
{
	float t = vec[i] + N;
	b0 = ((int)t) & BM;
	b1 = (b0 + 1) & BM;
	r0 = t - (int)t;
	r1 = r0 - 1.;
}

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