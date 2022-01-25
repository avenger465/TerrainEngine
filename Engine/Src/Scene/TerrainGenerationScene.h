#pragma once
#include "BaseScene.h"
#include "Math/CPerlinNoise.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

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

	void RenderScene();

	// frameTime is the time passed since the last frame
	void UpdateScene(float frameTime, HWND HWnd);

private:
	void BuildHeightMap();

	void BuildPerlinHeightMap(int Amplitude, float frequency);
	void BrownianMotion(int Amplitude, float frequency, int octaves);
	void RigidNoise(int Amplitude, float frequency);
	void InverseRigidNoise(int Amplitude, float frequency);

	void IMGUI();

public:
	float* heightMap;

private:
	int sizeOfTerrain = 200;
	int resolution = 200;
	int seed;

	bool enableTerrain = true;
	bool enableLights = false;

	float frequency = 0.45;
	int amplitude = 45;
	int octaves = 10;

	Model* gGround;
	Model* gTerrain;

	static const int NUM_LIGHTS = 2;
	CLight* gLights[NUM_LIGHTS];
	float LightScale[NUM_LIGHTS] = { 10.0f, 30.0f };

	CVector3 LightsColour[NUM_LIGHTS] = { {1.0f, 0.8f, 1.0f},
										  {1.0f, 0.8f, 0.2f} };

	CVector3 LightsPosition[NUM_LIGHTS] = { { 60, 10, 0 },
											{ -10, 25, -30 } };

	CVector3 TerrainYScale = { 5, 5, 5 };

	CVector3 gAmbientColour = { 0.2f, 0.2f, 0.3f }; // Background level of light (slightly bluish to match the far background, which is dark blue)
	float    gSpecularPower = 256; // Specular power controls shininess - same for all models in this app
	ColourRGBA gBackgroundColor = { 0.2f, 0.2f, 0.3f, 1.0f };

	// Variables controlling light1's orbiting of the cube
	const float gLightOrbit = 20.0f;
	const float gLightOrbitSpeed = 0.7f;
};