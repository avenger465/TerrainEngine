#pragma once
#include "BasicScene/BaseScene.h"
#include "System/System.h"
#include "Math/CPerlinNoise.h"
#include "Math/DiamondSquare.h"

class TerrainGenerationScene :
    public BaseScene
{
	const int SizeOfTerrain = 128;

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
	void BuildHeightMap();

	void BuildPerlinHeightMap(int Amplitude, float frequency, bool bBrownianMotion);
	void BrownianMotion(int Amplitude, float frequency, int octaves);
	void RigidNoise(int Amplitude, float frequency);
	void InverseRigidNoise(int Amplitude, float frequency);
	void DiamondSquareMap(double range);

private:

	CVector3 CameraPosition{ 1.5f, 2500, -4500 };
	CVector3 CameraRotation{ ToRadians(33.6f), 0.0f, 0.0f };
	ImGuiWindowFlags windowFlags = 0;

	int resolution = 500;
	int seed = 0;

	std::vector<std::vector<float>> HeightMap;
	float* Map;


	/*HeightMapType* m_HeightMap;*/

	bool enableTerrain = true;
	bool enableLights = false;
	bool darkStyle = true;

	float frequency = 0.45f;
	int amplitude = 45;
	int octaves = 10;

	float Range = 2.0;

	Model* TerrainModel;

	CVector3 TerrainYScale = { 10, 10, 10 };
};