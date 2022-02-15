#pragma once
class DiamondSquare
{
private:
	double random_range;
	double min_val;
	double max_val;

	float** map;
	int size;

	int range;

public:
	DiamondSquare(float** array, int s);
	~DiamondSquare();

	float** process();
	void _on_start();
	void _on_end();
	void diamondStep(int, int);
	void squareStep(int, int);

	double dRand(double dMin, double dMax);
};

