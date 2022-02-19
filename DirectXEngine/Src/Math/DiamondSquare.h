#pragma once
#include "tepch.h"
class DiamondSquare
{
private:
	float values[128 * 128];

	int size = 0;
	double scale = 0.0;

	float* _copy;

	double range = 1.0f;

	int max = 0;

public:
	DiamondSquare(int s, double r);
	~DiamondSquare();


	void process(std::vector<std::vector<float>>& temp);
	void _on_start(std::vector<std::vector<float>>& temp);

	void Divide(int size);
	void Set(int x, int y, float Value);
	float Get(int x, int y);

	//void Square( int size, int x, int y, float offset);
	void Square(int sideLength, int halfSide, std::vector<std::vector<float>>& temp);
	//void Diamond(int x, int y, int size, float offset);
	void Diamond(int sideLength, int halfSide, std::vector<std::vector<float>>& temp);

	void _on_end();

	double fRand2(double fMin, double dMax);
	void timeReset();
};

