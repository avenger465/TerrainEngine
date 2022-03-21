#pragma once
#include "tepch.h"
#include <wincrypt.h>
class DiamondSquare
{
//----------------------//
// Construction / Usage	//
//----------------------//
public:
	//Constructor
	DiamondSquare(int size, float spread, float spreadReduction);

	//Destructor
	~DiamondSquare();
	
	//Function to reset the time to generate a new seed for the random function
	void timeReset();

	//Function to go through the Diamond Square Algorithm and generate the new HeightMap
	void process(std::vector<std::vector<float>>& HeightMap);

	//Function to set the corners of the HeightMap to a random value of the Spread
	void _on_start(std::vector<std::vector<float>>& HeightMap);

	//Function to generate a random value between two values
	double fRand2(float fMin, float dMax);
	

//-------------//
// Member data //
//-------------//
private:
	//size of the HeightMap
	int m_Size = 0;

	//The Spread of values through the HeightMap
	float m_Spread = 0.0f;

	//The amount the Spread gets divided by each loop
	float m_SpreadReduction;
};