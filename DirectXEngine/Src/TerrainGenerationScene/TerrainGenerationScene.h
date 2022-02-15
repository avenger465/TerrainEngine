#pragma once
#include "BasicScene/BaseScene.h"
#include "Math/CPerlinNoise.h"
#include "Math/DiamondSquare.h"

class TerrainGenerationScene :
    public BaseScene
{
	bool InitGeometry(std::string& LastError);

	// Layout the scene
	// Returns true on success
	bool InitScene();

	// Release the geometry resources created above
	void ReleaseResources();

	void RenderSceneFromCamera(Camera* camera);

	//--------------------------------------------------------------------------------------
	// Scene Render and Update
	//--------------------------------------------------------------------------------------

	void RenderScene(float frameTime);

	// frameTime is the time passed since the last frame
	void UpdateScene(float frameTime, HWND HWnd);


private:
	void BuildHeightMap();

	void BuildPerlinHeightMap(int Amplitude, float frequency, bool bBrownianMotion);
	void BrownianMotion(int Amplitude, float frequency, int octaves);
	void RigidNoise(int Amplitude, float frequency);
	void InverseRigidNoise(int Amplitude, float frequency);
	void DiamondSquareMap();

	void IMGUI();

public:
	float** heightMap;

private:
	int sizeOfTerrain = 200;
	int resolution = 200;
	int seed = 0;

	bool enableTerrain = true;
	bool enableLights = false;

	float frequency = 0.45f;
	int amplitude = 45;
	int octaves = 10;

	Model* TerrainModel;

	CVector3 TerrainYScale = { 5, 5, 5 };

};