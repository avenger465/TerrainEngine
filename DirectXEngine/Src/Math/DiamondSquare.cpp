#include "DiamondSquare.h"

#include <stdlib.h>
#include <time.h>

DiamondSquare::DiamondSquare(int s, double r)
{
	size = s + 1;
	range = r;
	max = size - 1;
}

DiamondSquare::~DiamondSquare()
{
}

void DiamondSquare::timeReset()
{
	srand(time(0));
}

void DiamondSquare::Divide(int size)
{
	//int x;
	//int y;
	//int half = size / 2;
	//float scale = range * size;
	//if (half < 1) return;
	//for (y = half; y < max; y += size)
	//{
	//	for (x = half; x < max; x += size)
	//	{
	//		float temp = fRand2(0, 2);
	//		Square(x, y, half,temp);
	//	}
	//}

	//for (y = 0; y < max; y += half)
	//{
	//	for (x = (y + half) % size; x < max; x += size)
	//	{
	//		float temp = fRand2(0, 2);
	//		Diamond(x, y, half, temp);
	//	}
	//}
	//Divide(size / 2);

}

void DiamondSquare::Set(int x, int y, float Value)
{
	_copy[x + size * y] = Value;
}

float DiamondSquare::Get(int x, int y)
{
	if (x < 0 || x > max || y < 0 || y > max) return -1;
	return _copy[x + size * y];
}

//void DiamondSquare::Square(int size, int x, int y, float stepSize)
void DiamondSquare::Square(int sideLength, int halfSide, std::vector<std::vector<float>>& temp)
{
	//float temp1 = Get(x - size, y - size);
	//float temp2 = Get(x + size, y - size);
	//float temp3 = Get(x + size, y + size);
	//float temp4 = Get(x - size, y + size);
	//float ave = (temp1 + temp2 + temp3 + temp4) / 4.0f;
	//float temp = ave + offset;
	//Set(x, y, temp);


	for (int x = 0; x < size - 1; x += sideLength)
	{
		for (int y = 0; y < size - 1; y += sideLength)
		{
			double avg = temp[x][y] + temp[x + sideLength][y] + temp[x][y + sideLength] + temp[x + sideLength][y + sideLength];
			avg /= 4.0;
			temp[x + halfSide][y + halfSide] = (float)avg + fRand2(-range, range);
		}
	}
}

//void DiamondSquare::Diamond(int x, int y, int size, float offset)
void DiamondSquare::Diamond(int sideLength, int halfSide, std::vector<std::vector<float>>& temp)
{
	//float temp1 = Get(x, y - size);
	//float temp2 = Get(x + size,y);
	//float temp3 = Get(x, y + size);
	//float temp4 = Get(x - size,y);
	//float ave = (temp1 + temp2 + temp3 + temp4) / 4.0f;
	//Set(x, y, ave + offset);

	for (int x = 0; x < size - 1; x += halfSide)
	{
		for (int y = (x + halfSide) % sideLength; y < size - 1; y += sideLength)
		{
			double avg = temp[(x - halfSide + size - 1) % (size - 1)][y] +
				temp[(x + halfSide) % (size - 1)][y] +
				temp[x][(y + halfSide) % (size - 1)] +
				temp[x][(y - halfSide + size - 1) % (size - 1)];
			avg /= 4.0 + fRand2(-range, range);
			temp[x][y] = (float)avg;

			if (x == 0) temp[size - 1][y] = (float)avg;
			if (y == 0) temp[x][size - 1] = (float)avg;
		}
	}
}

void DiamondSquare::process(std::vector<std::vector<float>>& temp)
{
	timeReset();
	_on_start(temp);


	for (int sideLength = size - 1; sideLength >= 2; sideLength /= 2, range /= 2)
	{
		int halfSide = sideLength / 2;

		Square(sideLength, halfSide, temp);
		Diamond(sideLength, halfSide, temp);
	}

	//Set(0, 0, 0.5f);
	//Set(max, 0, 0.5f);
	//Set(max, max, 0.5f);
	//Set(0, max, 0.5f);

	//unsigned int stepSize = (size - 1) * 2;
	//do
	//{
	//	stepSize /= 2;

	//	for (unsigned int i = 0; i < size; i += stepSize)
	//		for (unsigned int j = 0; j < size; j += stepSize)
	//			Square(size, i, j, stepSize);

	//} while (stepSize > 1);

	//Divide(size);
	//return _copy;
	// 
	//double h = 0.75;
	//for (int sideLength = 128; sideLength >= 2; sideLength /= 2, h /= 2.0)
	//{
	//	int halfSide = sideLength / 2;

	//	for (int x = 0; x < 128; x += sideLength)
	//	{
	//		for (int y = 0; y < 129 - 1; y += sideLength)
	//		{
	//			double avg = temp[x][y] + temp[x + sideLength][y] + temp[x][y + sideLength] + temp[x + sideLength][y + sideLength];
	//			avg /= 4.0;
	//			temp[x + halfSide][y + halfSide] = (float)(avg + (fRand2(-1, 1) * 2 * h) - h);
	//		}
	//	}

	//	for (int x = 0; x < 128; x += sideLength)
	//	{
	//		for (int y = (x + halfSide) % sideLength; y < 128; y += sideLength)
	//		{
	//			float avg =
	//				temp[(x - halfSide + 128) % (128)][y] + //left of center
	//				temp[(x + halfSide) % (128)][y] + //right of center
	//				temp[x][(y + halfSide) % (128)] + //below center
	//				temp[x][(y - halfSide + 128) % (128)]; //above center
	//			avg /= 4.0;

	//			avg = (float)(avg + (fRand2(-1, 1) * 2 * h) - h);
	//			temp[x][y] = avg;
	//			if (x == 0) temp[128][y] = avg;
	//			if (y == 0) temp[x][128] = avg;
	//		}
	//	}
	//}

	_on_end();
}

void DiamondSquare::_on_start(std::vector<std::vector<float>>& temp)
{
	temp[0][0] = 0.2f;
	temp[0][size - 1] = 0.2f;
	temp[size - 1][0] = 0.2f;
	temp[size - 1][size - 1] = 0.2f;
}

void DiamondSquare::_on_end()
{

}

double DiamondSquare::fRand2(double dMin, double dMax)
{
	double d = (double)rand() / RAND_MAX;
	double n = dMax - dMin;
	double m = dMin + d;
	double o = m * n;
	return o;
}
