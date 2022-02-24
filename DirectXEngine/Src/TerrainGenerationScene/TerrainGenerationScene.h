#pragma once
#include "BasicScene/BaseScene.h"
#include "System/System.h"
#include "Math/CPerlinNoise.h"
#include "Math/DiamondSquare.h"
#include "Math/CVector3.h"

class TerrainGenerationScene :
    public BaseScene
{
	const int SizeOfTerrain = 256;

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
	void BrownianMotion(float Amplitude, float frequency, int octaves);
	void RigidNoise();
	void InverseRigidNoise();
	void DiamondSquareMap();
	void NormaliseHeightMap(float normaliseAmount);

private:

	CVector3 CameraPosition{ 503.95f, 8869.55f, -9255.2f };
	CVector3 CameraRotation{ ToRadians(45.6f), 0.0f, 0.0f };
	ImGuiWindowFlags windowFlags = 0;

	int resolution = 500;
	int seed = 0;

	std::vector<std::vector<float>> HeightMap;
	std::vector<std::vector<CVector3>> NormalMap;

	float* Map;

	bool enableTerrain = true;
	bool enableLights = false;

	float frequency = 0.125f;
	float Amplitude = 200.0;
	int octaves = 5;

	float AmplitudeReduction = 0.33f;
	float FrequencyMultiplier = 1.5f;


	float Spread = 30.0;
	float SpreadReduction = 2.0f;

	Model* TerrainModel;

	CVector3 TerrainYScale = { 10, 30, 10 };
};