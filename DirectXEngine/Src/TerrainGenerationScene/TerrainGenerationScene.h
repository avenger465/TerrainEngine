#pragma once
#include "BasicScene/BaseScene.h"
#include "System/System.h"
#include <random>
#include <fstream>
#include "Math/CPerlinNoise.h"
#include "Math/DiamondSquare.h"
#include "Math/CVector3.h"

class TerrainGenerationScene :
    public BaseScene
{
	const int SizeOfTerrain = 256;
	const int SizeOfTerrainVertices = 255;

	virtual bool InitGeometry(std::string& LastError) override;

	// Layout the scene
	// Returns true on success
	virtual bool InitScene() override;

	// Release the geometry resources created above
	virtual void ReleaseResources() override;

	virtual void RenderSceneFromCamera(Camera* camera) override;

	//--------------------------------------------------------------------------------------
	// Scene Render and Update
	//--------------------------------------------------------------------------------------

	virtual void RenderScene(float frameTime) override;

	// frameTime is the time passed since the last frame
	virtual void UpdateScene(float frameTime, HWND HWnd) override;

	virtual void IMGUI() override;


private:
	void BuildHeightMap(float height);

	void BuildPerlinHeightMap(float amplitude, float frequency, bool bBrownianMotion);
	void PerlinNoiseWithOctaves(float Amplitude, float frequency, int octaves);
	void RigidNoise();
	void InverseRigidNoise();
	void DiamondSquareMap();
	void Redistribution(float power);
	void Terracing(float terracingMultiplier);
	void NormaliseHeightMap(float normaliseAmount);

	void UpdateFoliagePosition();

private:

	CVector3 CameraPosition{ 5500.55f, 7602.11f, -7040.85f };
	CVector3 CameraRotation{ ToRadians(34.3775f), 0.0f, 0.0f };
	ImGuiWindowFlags windowFlags = 0;

	int resolution = 500;
	int seed = 0;

	std::vector<Model*> PlantModels;
	int plantResizeAmount = 2;
	int CurrentPlantVectorSize = 2;

	std::vector<std::vector<float>> HeightMap;

	bool enableTerrain = true;
	bool enableWireFrame = false;

	float frequency = 0.125f;
	float Amplitude = 200.0;
	int octaves = 5;

	float AmplitudeReduction  = 0.33f;
	float FrequencyMultiplier = 1.5f;
	float RedistributionPower = 0.937f;
	float terracingMultiplier = 1.1f;

	float Spread = 30.0;
	float SpreadReduction = 2.0f;

	CVector3 TerrainYScale = { 10, 30, 10 };
};