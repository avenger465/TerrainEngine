#include "tepch.h"
#include "project/Common.h"
#include "Utility/CResourceManager.h"
#include "project/Camera.h"
#include "project/CLight.h"
#include "Utility/Input.h"
#include "project/Mesh.h"
#include "project/Model.h"
#include "project/State.h"
#include "project/Shader.h"
#include "Utility/ColourRGBA.h"

#pragma once
class BaseScene
{
public:
	//--------------------------------------------------------------------------------------
// Scene Geometry and Layout
//--------------------------------------------------------------------------------------

// Prepare the geometry required for the scene
// Returns true on success
	virtual bool InitGeometry(std::string& LastError) = 0;

	// Layout the scene
	// Returns true on success
	virtual bool InitScene() = 0;

	// Release the geometry resources created above
	virtual void ReleaseResources() = 0;

	virtual void RenderSceneFromCamera(Camera* camera) = 0;

	//--------------------------------------------------------------------------------------
	// Scene Render and Update
	//--------------------------------------------------------------------------------------

	virtual void RenderScene() = 0;

	// frameTime is the time passed since the last frame
	virtual void UpdateScene(float frameTime, HWND HWnd) = 0;

	int viewportWidth;
	int viewportHeight;

protected:

	PerFrameConstants gPerFrameConstants;
	ID3D11Buffer* gPerFrameConstantBuffer;

	PerModelConstants gPerModelConstants;
	ID3D11Buffer* gPerModelConstantBuffer;

	CResourceManager* resourceManager;
	Camera* gCamera;

	std::ostringstream frameTimeMs;

	bool lockFPS = true;
	std::string FPS;
};

