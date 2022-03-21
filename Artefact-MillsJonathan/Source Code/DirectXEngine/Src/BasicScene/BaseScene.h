#include "tepch.h"
#include "project/Common.h"
#include "Utility/CResourceManager.h"
#include "BasicScene/Camera.h"
#include "BasicScene/CLight.h"
#include "Utility/Input.h"
#include "Data/Mesh.h"
#include "Data/Model.h"
#include "Data/State.h"
#include "Shaders/Shader.h"
#include "Utility/ColourRGBA.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"


#pragma once
class BaseScene
{
public:
	//--------------------------------------------------------------------------------------
	// Scene Geometry and Layout
	//--------------------------------------------------------------------------------------

	//Function to setup all the geometry to be used in the scene
	virtual bool InitGeometry(std::string& LastError) = 0;

	//Function to setup the scene 
	virtual bool InitScene() = 0;

	// Release the geometry resources created above
	virtual void ReleaseResources() = 0;

	//Function to render scene from the Camera's perspective
	virtual void RenderSceneFromCamera(Camera* camera) = 0;

	//--------------------------------------------------------------------------------------
	// Scene Render and Update
	//--------------------------------------------------------------------------------------
	//Function to render the scene, called every frame
	virtual void RenderScene(float frameTime) = 0;

	// frameTime is the time passed since the last frame
	virtual void UpdateScene(float frameTime, HWND HWnd) = 0;

	//Function to contain all of the ImGui code
	virtual void IMGUI() = 0;

protected:

	PerFrameConstants gPerFrameConstants;
	ID3D11Buffer* gPerFrameConstantBuffer;

	PerModelConstants gPerModelConstants;
	ID3D11Buffer* gPerModelConstantBuffer;

	CResourceManager* resourceManager;
	Camera* MainCamera;
	Model* GroundModel;

	//-------------------//
	// Light Information //
	//-------------------//
	CLight* Light;
	float LightScale = 15000.0f;
	CVector3 LightColour = { 0.9922f, 0.7217f, 0.0745f };
	CVector3 LightPosition = { 5000.0f, 13000.0f, 5000.0f };

	//-------------------------//
	// Basic Scene Information //
	//-------------------------//
	CVector3 gAmbientColour = { 0.2f, 0.2f, 0.3f };
	ColourRGBA gBackgroundColor = { 0.2f, 0.2f, 0.3f, 1.0f };

	std::ostringstream frameTimeMs;

	bool lockFPS = true;
	std::string FPS_String;
	int FPS;

	// Dimensions of scene texture - controls quality of rendered scene
	int textureWidth = 900;
	int textureHeight = 900;

	// The scene texture - each frame it is rendered to, then it is used as a texture for model
	ID3D11Texture2D* SceneTexture = nullptr; // This object represents the memory used by the texture on the GPU
	ID3D11RenderTargetView* SceneRenderTarget = nullptr; // This object is used when we want to render to the texture above
	ID3D11ShaderResourceView* SceneTextureSRV = nullptr; // This object is used to give shaders access to the texture above (SRV = shader resource view)

	ID3D11Texture2D* SceneDepthStencil = nullptr; // This object represents the memory used by the texture on the GPU
	ID3D11DepthStencilView* SceneDepthStencilView = nullptr; // This object is used when we want to use the texture above as the depth buffer
};