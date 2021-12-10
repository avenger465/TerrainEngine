//--------------------------------------------------------------------------------------
// Scene geometry and layout preparation
// Scene rendering & update
//--------------------------------------------------------------------------------------
#include "tepch.h"

#include "Scene.h"
#include "Mesh.h"
#include "Model.h"
#include "Camera.h"
#include "State.h"
#include "Shader.h"
#include "Utility/Input.h"
#include "Common.h"
#include "../Utility/CResourceManager.h"
#include "CLight.h"


#include "Math/CVector2.h" 
#include "Math/CVector3.h" 
#include "Math/CMatrix4x4.h"
#include "Math/MathHelpers.h"     // Helper functions for maths
#include "Utility/GraphicsHelpers.h" // Helper functions to unclutter the code here

#include "Utility/ColourRGBA.h" 

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"


//--------------------------------------------------------------------------------------
// Scene Data
//--------------------------------------------------------------------------------------
// Addition of Mesh, Model and Camera classes have greatly simplified this section
// Geometry data has gone to Mesh class. Positions, rotations, matrices have gone to Model and Camera classes

// Constants controlling speed of movement/rotation (measured in units per second because we're using frame time)
const float ROTATION_SPEED = 2.0f;  // 2 radians per second for rotation
const float MOVEMENT_SPEED = 50.0f; // 50 units per second for movement (what a unit of length is depends on 3D model - i.e. an artist decision usually)

int terrainResolution = 128;
// Meshes, models and cameras, same meaning as TL-Engine. Meshes prepared in InitGeometry function, Models & camera in InitScene

Model* gCharacter;
Model* gCrate;
Model* gGround;

CResourceManager* resourceManager;
Camera* gCamera;

std::ostringstream frameTimeMs;

bool enableTerrain = true;
bool enableLights = true;
bool enableObjects = true;

float frequency = 0.12;
int amplitude = 10;

// Store lights in an array in this exercise
const int NUM_LIGHTS = 2;
CLight* gLights[NUM_LIGHTS];

float LightScale[NUM_LIGHTS] = {10.0f, 40.0f};

CVector3 LightsColour[NUM_LIGHTS] = { {1.0f, 0.8f, 1.0f},
                                      {1.0f, 0.8f, 0.2f} };

CVector3 LightsPosition[NUM_LIGHTS] = { { 30, 10, 0 },
                                        { -10, 25, -30 } };

// Additional light information
CVector3 gAmbientColour = { 0.2f, 0.2f, 0.3f }; // Background level of light (slightly bluish to match the far background, which is dark blue)
float    gSpecularPower = 256; // Specular power controls shininess - same for all models in this app

ColourRGBA gBackgroundColor = { 0.2f, 0.2f, 0.3f, 1.0f };

// Variables controlling light1's orbiting of the cube
const float gLightOrbit = 20.0f;
const float gLightOrbitSpeed = 0.7f;

// Lock FPS to monitor refresh rate, which will typically set it to 60fps. Press 'p' to toggle to full fps
bool lockFPS = true;
std::string FPS;

//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
// Variables sent over to the GPU each frame
// The structures are now in Common.h
// IMPORTANT: Any new data you add in C++ code (CPU-side) is not automatically available to the GPU
//            Anything the shaders need (per-frame or per-model) needs to be sent via a constant buffer

PerFrameConstants gPerFrameConstants;      // The constants that need to be sent to the GPU each frame (see common.h for structure)
ID3D11Buffer*     gPerFrameConstantBuffer; // The GPU buffer that will recieve the constants above

PerModelConstants gPerModelConstants;      // As above, but constant that change per-model (e.g. world matrix)
ID3D11Buffer*     gPerModelConstantBuffer; // --"--

//--------------------------------------------------------------------------------------
// Initialise scene geometry, constant buffers and states
//--------------------------------------------------------------------------------------

// Prepare the geometry required for the scene
// Returns true on success
bool InitGeometry()
{
    // Initialise texture manager with the default texture
    resourceManager = new CResourceManager(gD3DDevice, gD3DContext);
    resourceManager->loadTexture(L"default", L"Media/DefaultDiffuse.png");

    // Load mesh geometry data, just like TL-Engine this doesn't create anything in the scene. Create a Model for that.
    try 
    {
        resourceManager->loadMesh(L"ManMesh",std::string("Src/Data/Man.x"));
        resourceManager->loadMesh(L"GroundMesh", std::string("Src/Data/Hills.x"));
        resourceManager->loadMesh(L"CrateMesh", std::string("Src/Data/CargoContainer.x"));
        resourceManager->loadMesh(L"LightMesh", std::string("Src/Data/Light.x"));
    }
    catch (std::runtime_error e)  // Constructors cannot return error messages so use exceptions to catch mesh errors (fairly standard approach this)
    {
        gLastError = e.what(); // This picks up the error message put in the exception (see Mesh.cpp)
        return false;
    }

    //m_Terrain = new TerrainMesh(gD3DDevice, gD3DContext);
    //shader = new LightShader(gD3DDevice, gHWnd);

    //terrainResolution = m_Terrain->GetResolution();
    //m_Terrain->sendData(gD3DContext);
    //shader->setShaderParameters(gD3DContext, textureMgr->getTexture(L"grass"),textureMgr->getTexture(L"rock"), textureMgr->getTexture(L"dirt"), textureMgr->getTexture(L"sand"), 0.0f, false);
    //shader->render(gD3DContext, m_Terrain->getIndexCount());
    //delete m_Terrain; m_Terrain = nullptr;
    //TerrainMesh* m_Terrain;
    //LightShader* shader;
    //TextureManager* textureMgr;

    // Load the shaders required for the geometry we will use (see Shader.cpp / .h)
    if (!LoadShaders())
    {
        gLastError = "Error loading shaders";
        return false;
    }

    // Create GPU-side constant buffers to receive the gPerFrameConstants and gPerModelConstants structures above
    // These allow us to pass data from CPU to shaders such as lighting information or matrices
    // See the comments above where these variable are declared and also the UpdateScene function
    gPerFrameConstantBuffer = CreateConstantBuffer(sizeof(gPerFrameConstants));
    gPerModelConstantBuffer = CreateConstantBuffer(sizeof(gPerModelConstants));
    if (gPerFrameConstantBuffer == nullptr || gPerModelConstantBuffer == nullptr)
    {
        gLastError = "Error creating constant buffers";
        return false;
    }

    //-----------------------//
    //  LOADING OF TEXTURES  //
    //-----------------------//
    resourceManager->loadTexture(L"Cargo", L"Src/Data/CargoA.dds");
    //resourceManager->loadTexture(L"Grass", L"Src/Data/GrassDiffuseSpecular.dds");
    resourceManager->loadTexture(L"Light", L"Media/Flare.jpg");
    resourceManager->loadTexture(L"Character", L"Src/Data/ManDiffuseSpecular.dds");
    resourceManager->loadTexture(L"Grass", L"Media/Grass2.jpg");
    resourceManager->loadTexture(L"Rock", L"Media/rock.png");
    resourceManager->loadTexture(L"Dirt", L"Media/dirtColour.jpg");

  	// Create all filtering modes, blending modes etc. used by the app (see State.cpp/.h)
	if (!CreateStates())
	{
		gLastError = "Error creating states";
		return false;
	}

	return true;
}

// Prepare the scene
// Returns true on success
bool InitScene()
{
    //// Set up scene ////

    gCharacter = new Model(resourceManager->getMesh(L"ManMesh"));
    gCrate     = new Model(resourceManager->getMesh(L"CrateMesh"));
    gGround = new Model(resourceManager->getMesh(L"GroundMesh"));


	// Initial positions
	gCharacter->SetPosition({ 25, 1, 10 });
    gCharacter->SetScale(1.0f);
    gCharacter->SetRotation({ 0.0f, ToRadians(140.0f), 0.0f });
	gCrate-> SetPosition({ 45, 0, 45 });
	gCrate-> SetScale( 6.0f );
	gCrate-> SetRotation({ 0.0f, ToRadians(-50.0f), 0.0f });


    // Light set-up - using an array this time
    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        gLights[i] = new CLight(resourceManager->getMesh(L"LightMesh"), LightScale[i], LightsColour[i], LightsPosition[i], pow(LightScale[i], 0.7f));
    }

    //// Set up camera ////

    gCamera = new Camera();
    gCamera->SetPosition({ 25, 12,-10 });
    gCamera->SetRotation({ ToRadians(13.0f), ToRadians(15.0f), 0.0f });

    return true;
}


// Release the geometry and scene resources created above
void ReleaseResources()
{
    ReleaseStates();

    if (gPerModelConstantBuffer)  gPerModelConstantBuffer->Release();
    if (gPerFrameConstantBuffer)  gPerFrameConstantBuffer->Release();

    ReleaseShaders();

    // See note in InitGeometry about why we're not using unique_ptr and having to manually delete
    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        delete gLights[i]->LightModel;  gLights[i]->LightModel = nullptr;
    }
    delete gCamera;     gCamera    = nullptr;
    delete gGround;     gGround    = nullptr;
    delete gCrate;      gCrate     = nullptr;
    delete gCharacter;  gCharacter = nullptr;
}



//--------------------------------------------------------------------------------------
// Scene Rendering
//--------------------------------------------------------------------------------------

// Render everything in the scene from the given camera
void RenderSceneFromCamera(Camera* camera)
{
    // Set camera matrices in the constant buffer and send over to GPU
    gPerFrameConstants.viewMatrix           = camera->ViewMatrix();
    gPerFrameConstants.projectionMatrix     = camera->ProjectionMatrix();
    gPerFrameConstants.viewProjectionMatrix = camera->ViewProjectionMatrix();
    UpdateConstantBuffer(gPerFrameConstantBuffer, gPerFrameConstants);

    // Indicate that the constant buffer we just updated is for use in the vertex shader (VS) and pixel shader (PS)
    gD3DContext->VSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer); // First parameter must match constant buffer number in the shader 
    gD3DContext->PSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);


    //// Render skinned models ////
    gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler);

    if (enableObjects)
    {
        // Select which shaders to use next
        gCharacter->Setup(gSkinningVertexShader, gPixelLightingPixelShader);

        // States - no blending, normal depth buffer and culling
        gCharacter->SetStates(gNoBlendingState, gUseDepthBufferState, gCullBackState);

        // Select the approriate textures and sampler to use in the pixel shader
        gCharacter->SetShaderResources(0, resourceManager->getTexture(L"Character"));

        gCharacter->Render();

        gCrate->Setup(gPixelLightingVertexShader);
        gCrate->SetShaderResources(0, resourceManager->getTexture(L"Cargo"));
        gCrate->Render();
    }


    //// Render non-skinned models ////

    // Select which shaders to use next

    // Render lit models, only change textures for each onee
    

    if (enableTerrain)
    {
        gGround->Setup(gPixelLightingVertexShader, gTerrainPixelShader);
        gGround->SetStates(gNoBlendingState, gUseDepthBufferState, gCullBackState);
        gGround->SetShaderResources(0, resourceManager->getTexture(L"Grass"));
        gGround->SetShaderResources(1, resourceManager->getTexture(L"Rock"));
        gGround->SetShaderResources(2, resourceManager->getTexture(L"Dirt"));
        gGround->Render();
    }

    //// Render lights ////
    if (enableLights)
    {
        // Select which shaders to use next
        gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
        gD3DContext->PSSetShader(gLightModelPixelShader, nullptr, 0);

        // Select the texture and sampler to use in the pixel shader
        ID3D11ShaderResourceView* LightTexture = resourceManager->getTexture(L"Light");
        gD3DContext->PSSetShaderResources(0, 1, &LightTexture); // First parameter must match texture slot number in the shaer
        gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler);

        // States - additive blending, read-only depth buffer and no culling (standard set-up for blending
        gD3DContext->OMSetBlendState(gAdditiveBlendingState, nullptr, 0xffffff);
        gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
        gD3DContext->RSSetState(gCullNoneState);

        // Render all the lights in the array
        for (int i = 0; i < NUM_LIGHTS; ++i)
        {
            gPerModelConstants.objectColour = gLights[i]->LightColour; // Set any per-model constants apart from the world matrix just before calling render (light colour here)
            gLights[i]->RenderLight();
        }
    }
}

// Rendering the scene
void RenderScene()
{

    //IMGUI
    //*******************************
    // Prepare ImGUI for this frame
    //*******************************

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    //*******************************

    //// Common settings ////

    // Set up the light information in the constant buffer
    // Don't send to the GPU yet, the function RenderSceneFromCamera will do that
    gPerFrameConstants.light1Colour   = gLights[0]->LightColour * gLights[0]->LightStrength;
    gPerFrameConstants.light1Position = gLights[0]->LightModel->Position();
    gPerFrameConstants.light2Colour   = gLights[1]->LightColour * gLights[1]->LightStrength;
    gPerFrameConstants.light2Position = gLights[1]->LightModel->Position();

    gPerFrameConstants.ambientColour  = gAmbientColour;
    gPerFrameConstants.specularPower  = gSpecularPower;
    gPerFrameConstants.cameraPosition = gCamera->Position();
    gPerFrameConstants.enableLights = enableLights;

    //// Main scene rendering ////

    // Set the back buffer as the target for rendering and select the main depth buffer.
    // When finished the back buffer is sent to the "front buffer" - which is the monitor.
    gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);

    // Clear the back buffer to a fixed colour and the depth buffer to the far distance
    gD3DContext->ClearRenderTargetView(gBackBufferRenderTarget, &gBackgroundColor.r);
    gD3DContext->ClearDepthStencilView(gDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Setup the viewport to the size of the main window
    D3D11_VIEWPORT vp;
    vp.Width  = static_cast<FLOAT>(gViewportWidth);
    vp.Height = static_cast<FLOAT>(gViewportHeight);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    gD3DContext->RSSetViewports(1, &vp);


    // Render the scene from the main camera
    RenderSceneFromCamera(gCamera);

    ImGui::Begin("Controls");

    ImGui::Checkbox("Render Terrain", &enableTerrain);
    if (enableTerrain)
    {
        ImGui::Separator();
        ImGui::Text("");
        ImGui::Button("Perlin");
        ImGui::Text("");
        ImGui::SliderFloat("Terrain Frequency", &frequency, 0.01, 0.5);
        ImGui::SliderInt("Terrain amplitude", &amplitude, 5, 45);
        ImGui::Text("");
        ImGui::Button("Midpoint Displacement");
        ImGui::Text("");
        ImGui::Separator();
    }

    ImGui::Checkbox("Render Lights", &enableLights);
    if (enableLights)
    {
        ImGui::Separator();
        ImGui::Text("");
        ImGui::ColorEdit3("Light One Colour", &gLights[0]->LightColour.x);
        ImGui::ColorEdit3("Light Two Colour", &gLights[1]->LightColour.x);
        ImGui::Text("");
        ImGui::SliderFloat("Light One Scale", &gLights[0]->LightStrength, 1.0f, 90.0f);
        ImGui::SliderFloat("Light Two Scale", &gLights[1]->LightStrength, 1.0f, 90.0f);
        ImGui::Text("");
        ImGui::Separator();
    }
    ImGui::Checkbox("Render Objects", &enableObjects);


    ImGui::End();

    //*******************************

    //// Scene completion ////

    ImGui::Render();
    gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());


    // When drawing to the off-screen back buffer is complete, we "present" the image to the front buffer (the screen)
    // Set first parameter to 1 to lock to vsync (typically 60fps)
    gSwapChain->Present(lockFPS ? 1 : 0, 0);
}


//--------------------------------------------------------------------------------------
// Scene Update
//--------------------------------------------------------------------------------------

// Update models and camera. frameTime is the time passed since the last frame
void UpdateScene(float frameTime)
{
	// Control character part. First parameter is node number - index from flattened depth-first array of model parts. 0 is root
	gCharacter->Control(17, frameTime, Key_I, Key_K, Key_J, Key_L, Key_U, Key_O, Key_Period, Key_Comma );

    // Orbit the light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
	static float rotate = 0.0f;
    static bool go = true;
	gLights[0]->LightModel->SetPosition( gCharacter->Position() + CVector3{ cos(rotate) * gLightOrbit, 10, sin(rotate) * gLightOrbit } );
    if (go)  rotate -= gLightOrbitSpeed * frameTime;
    if (KeyHit(Key_1))  go = !go;

	// Control camera (will update its view matrix)
	gCamera->Control(frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D );


    // Toggle FPS limiting
    if (KeyHit(Key_P))  lockFPS = !lockFPS;

    // Show frame time / FPS in the window title //
    const float fpsUpdateTime = 0.5f; // How long between updates (in seconds)
    static float totalFrameTime = 0;
    static int frameCount = 0;
    totalFrameTime += frameTime;
    ++frameCount;
    if (totalFrameTime > fpsUpdateTime)
    {
        // Displays FPS rounded to nearest int, and frame time (more useful for developers) in milliseconds to 2 decimal places
        float avgFrameTime = totalFrameTime / frameCount;
        std::ostringstream frameTimeMs;
        frameTimeMs.precision(2);
        frameTimeMs << std::fixed << avgFrameTime * 1000;
        FPS = std::to_string(static_cast<int>(1 / avgFrameTime + 0.5f));
        std::string windowTitle = "CO2409 Week 22: Skinning - Frame Time: " + frameTimeMs.str() +
                                  "ms, FPS: " + FPS;
        SetWindowTextA(gHWnd, windowTitle.c_str());
        totalFrameTime = 0;
        frameCount = 0;
    }
}
