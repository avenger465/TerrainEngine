#pragma once
#include "tepch.h"
#include <wincrypt.h>
class DiamondSquare
{
private:
	int m_Size = 0;
	double m_Scale = 0.0;

	float m_Spread = 0.0f;
	float m_SpreadReduction;

public:
	DiamondSquare(int size, float spread, float spreadReduction);
	~DiamondSquare();

	void process(std::vector<std::vector<float>>& temp);
	void _on_start(std::vector<std::vector<float>>& temp);

	void _on_end();

	double fRand2(float fMin, float dMax);
	void timeReset();
};