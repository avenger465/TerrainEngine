#include "DiamondSquare.h"
#include <stdlib.h>
#include <time.h>

//Constructor to initialise the data
DiamondSquare::DiamondSquare(int size, float spread, float spreadReduction)
{
	m_Size = size + 1;
	m_Spread = spread;
	m_SpreadReduction = spreadReduction;
}

//Deconstructor 
DiamondSquare::~DiamondSquare()
{
}

//Function to reset the time to generate a new seed for the random function
void DiamondSquare::timeReset()
{
	srand(time(0));
}

//Function to go through the Diamond Square Algorithm and generate the new HeightMap
void DiamondSquare::process(std::vector<std::vector<float>>& HeightMap)
{
	//Reset the Seed of the random generator
	timeReset();

	//Set the corners of the HeightMap
	_on_start(HeightMap);

	//side length is distance of a single square side
	for (int sideLength = m_Size - 1; sideLength >= 2; sideLength /= 2, m_Spread /= m_SpreadReduction)
	{
		//side length must be >= 2 so we always have
		//a new value (if its 1 we overwrite existing values
		//on the last iteration)
		
		//each iteration we are looking at smaller squares
		//diamonds, and we decrease the variation of the offset
		 
		//half the length of the side of a square
		//or distance from diamond center to one corner
		//(just to make calcs below a little clearer)
		int halfSide = sideLength / 2;

		//SQUARE STEP
		for (int x = 0; x < m_Size - 1; x += sideLength) 
		{
			for (int y = 0; y < m_Size - 1; y += sideLength) 
			{
				//x, y is upper left corner of square
				//calculate average of existing corners
				double avg = HeightMap[x][y] //top left
						   + HeightMap[x + sideLength][y]//top right
					       + HeightMap[x][y + sideLength]//lower left
						   + HeightMap[x + sideLength][y + sideLength];//lower right
				avg /= 4.0;

				//add a random value on to the average
				HeightMap[x + halfSide][y + halfSide] = abs(avg + fRand2(-m_Spread, m_Spread));
			}
		}

		//generate the diamond values
		//since the diamonds are staggered we only move x
		//by half side
		//NOTE: if the data shouldn't wrap then x < DATA_SIZE
		//to generate the far edge values

		//DIAMOND STEP
		for (int x = 0; x < m_Size - 1; x += halfSide) 
		{
			//and y is x offset by half a side, but moved by
			//the full side length
			//NOTE: if the data shouldn't wrap then y < DATA_SIZE
			//to generate the far edge values
			for (int y = (x + halfSide) % sideLength; y < m_Size - 1; y += sideLength) 
			{
				//x, y is center of diamond
				//note we must use mod  and add DATA_SIZE for subtraction 
				//so that we can wrap around the array to find the corners
				double avg = HeightMap[(x - halfSide + m_Size - 1) % (m_Size - 1)][y] //left of center
						   + HeightMap[(x + halfSide) % (m_Size - 1)][y]  //right of center
						   + HeightMap[x][(y + halfSide) % (m_Size - 1)]  //below center
						   + HeightMap[x][(y - halfSide + m_Size - 1) % (m_Size - 1)]; //above center


				avg /= 4.0;

				//add a random value on to the average
				avg = abs(avg + fRand2(-m_Spread, m_Spread));
				//update value for center of diamond
				HeightMap[x][y] = avg;

				//wrap values on the edges, remove
				//this and adjust loop condition above
				//for non-wrapping values.
				if (x == 0)  HeightMap[m_Size - 1][y] = avg;
				if (y == 0)  HeightMap[x][m_Size - 1] = avg;	              
			}
		}
	}
}

//Function to set the corners of the HeightMap to a random value of the Spread
void DiamondSquare::_on_start(std::vector<std::vector<float>>& HeightMap)
{
	HeightMap[0][0] = fRand2(-m_Spread, m_Spread);
	HeightMap[0][m_Size - 1] = fRand2(-m_Spread, m_Spread);
	HeightMap[m_Size - 1][0] = fRand2(-m_Spread, m_Spread);
	HeightMap[m_Size - 1][m_Size - 1] = fRand2(-m_Spread, m_Spread);
}

//Function to generate a random value between two values
double DiamondSquare::fRand2(float dMin, float dMax)
{
	return dMin + (dMax - dMin) * (static_cast<double>(rand()) / RAND_MAX);
}