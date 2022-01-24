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

#include <fstream>

#include "Math/CVector2.h" 
#include "Math/CVector3.h" 
#include "Math/CMatrix4x4.h"
#include "Math/MathHelpers.h"     // Helper functions for maths
#include "Utility/GraphicsHelpers.h" // Helper functions to unclutter the code here

#include "Utility/ColourRGBA.h" 
#include "Math/CPerlinNoise.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

int resolution = 25;
const int sizeOfArray = resolution * resolution;

//--------------------------------------------------------------------------------------
// Scene Data
//--------------------------------------------------------------------------------------
// Addition of Mesh, Model and Camera classes have greatly simplified this section
// Geometry data has gone to Mesh class. Positions, rotations, matrices have gone to Model and Camera classes

// Constants controlling speed of movement/rotation (measured in units per second because we're using frame time)
const float ROTATION_SPEED = 2.0f;  // 2 radians per second for rotation
const float MOVEMENT_SPEED = 150.0f; // 50 units per second for movement (what a unit of length is depends on 3D model - i.e. an artist decision usually)

int terrainResolution = 128;
// Meshes, models and cameras, same meaning as TL-Engine. Meshes prepared in InitGeometry function, Models & camera in InitScene

Model* gGround;
Model* gTerrain;
Model* gSky;
Mesh* skky;

CResourceManager* resourceManager;
Camera* gCamera;

std::ostringstream frameTimeMs;

bool enableTerrain = true;
bool enableLights = false;

float frequency = 0.45;
int amplitude = 45;

// Store lights in an array in this exercise
const int NUM_LIGHTS = 2;
CLight* gLights[NUM_LIGHTS];

float LightScale[NUM_LIGHTS] = {10.0f, 30.0f};

CVector3 LightsColour[NUM_LIGHTS] = { {1.0f, 0.8f, 1.0f},
                                      {1.0f, 0.8f, 0.2f} };

CVector3 LightsPosition[NUM_LIGHTS] = { { 60, 10, 0 },
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
int sizeOfTerrain = 100;
CVector3 TerrainYScale = {5, 5, 5};



//std::array<std::array<float, resolution>, resolution> heightMap;
float* heightMap;

void BuildHeightMap()
{
    float n = (resolution + 1) * (resolution + 1);
    heightMap = new float[(resolution + 1) * (resolution + 1)];
    float height = 1.0f;

    for (int i = 0; i <= resolution; ++i) {
        for (int j = 0; j <= resolution; ++j) {
            heightMap[(i * (resolution + 1)) + j] = height;
            //heightMap[i][j] = height;
        }
    }
}

void BuildPerlinHeightMap(int Amplitude, float frequency)
{
    BuildHeightMap();
    const float scale = sizeOfTerrain / resolution; //make sure that the terrain looks consistent 


    for (int i = 0; i <= resolution; ++i) // loop through the y 
    {
        for (int j = 0; j <= resolution; ++j) //loop through the x
        {
            float twoPoints[2] = { j * frequency * scale, i * frequency * scale };

            heightMap[(i * (resolution + 1)) + j] = CPerlinNoise::noise2(twoPoints) * Amplitude;
        }
    }
}

//--------------------------------------------------------------------------------------
// Initialise scene geometry, constant buffers and states
//--------------------------------------------------------------------------------------

// Prepare the geometry required for the scene
// Returns true on success
bool InitGeometry(std::string LastError)
{
    resourceManager = new CResourceManager();
    BuildHeightMap();
    //BuildPerlinHeightMap(amplitude, frequency);
    // Initialise texture manager with the default texture
  
    resourceManager->loadTexture(L"default", "Media/v.bmp");

    // Load mesh geometry data, just like TL-Engine this doesn't create anything in the scene. Create a Model for that.
    try 
    {
        resourceManager->loadMesh(L"GroundMesh", std::string("Src/Data/Hills.x"));
        resourceManager->loadMesh(L"LightMesh", std::string("Src/Data/Light.x"));
        resourceManager->loadMesh(L"SphereMesh", std::string("Src/Data/Sphere.x"));
        resourceManager->loadGrid(L"TerrainMesh", CVector3(-200, 0, -200), CVector3(200, 0, 200), resolution, resolution, heightMap, true, true);
        resourceManager->loadMesh(L"SkyMesh", std::string("Src/Data/Skybox.x"));
    }
    catch (std::runtime_error e)  // Constructors cannot return error messages so use exceptions to catch mesh errors (fairly standard approach this)
    {
        LastError = e.what(); // This picks up the error message put in the exception (see Mesh.cpp)
        return false;
    }

    //-----------------------//
    //  LOADING OF TEXTURES  //
    //-----------------------//

    try
    {
        resourceManager->loadTexture(L"Light", "Media/Flare.jpg");
        resourceManager->loadTexture(L"Grass", "Media/Grass.png");
        resourceManager->loadTexture(L"Rock", "Media/rock1.png");
        resourceManager->loadTexture(L"Dirt", "Media/Dirt2.png");
        resourceManager->loadTexture(L"Moon", "Media/moon.jpg");
    }
    catch (std::runtime_error e)
    {
        LastError = e.what();
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

    // Load the shaders required for the geometry we will use (see Shader.cpp / .h)
    if (!LoadShaders(LastError))
    {
        LastError = "Error loading shaders";
        return false;
    }

    // Create GPU-side constant buffers to receive the gPerFrameConstants and gPerModelConstants structures above
    // These allow us to pass data from CPU to shaders such as lighting information or matrices
    // See the comments above where these variable are declared and also the UpdateScene function
    gPerFrameConstantBuffer = CreateConstantBuffer(sizeof(gPerFrameConstants));
    gPerModelConstantBuffer = CreateConstantBuffer(sizeof(gPerModelConstants));
    if (gPerFrameConstantBuffer == nullptr || gPerModelConstantBuffer == nullptr)
    {
        LastError = "Error creating constant buffers";
        return false;
    }
    
  	// Create all filtering modes, blending modes etc. used by the app (see State.cpp/.h)
	if (!CreateStates(LastError))
	{
		LastError = "Error creating states";
		return false;
	}

	return true;
}

// Prepare the scene
// Returns true on success
bool InitScene()
{
    //// Set up scene ////
    gGround = new Model(resourceManager->getMesh(L"TerrainMesh"));
    //gGround = new Model(resourceManager->getMesh(L"SphereMesh"));
    //skky = resourceManager->getMesh(L"TerrainMesh");
    //gTerrain = new Model(resourceManager->getMesh(L"TerrainMesh"));
    //gTerrain->SetPosition({ 0,30,0 });

    // Light set-up - using an array this time
    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        gLights[i] = new CLight(resourceManager->getMesh(L"LightMesh"), LightScale[i], LightsColour[i], LightsPosition[i], pow(LightScale[i], 0.7f));
    }

    //// Set up camera ////

    gCamera = new Camera();
    //gCamera->SetPosition({ 0, 270, -500 });
    gCamera->SetPosition({ 12.5, 910, -1657 });
    gCamera->SetRotation({ ToRadians(28.6f), 0.0f, 0.0f });

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
    delete gTerrain; gTerrain = nullptr;
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
    gD3DContext->GSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);
    gD3DContext->PSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);


    //// Render skinned models ////
    gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler);


    //// Render non-skinned models ////
    // Select which shaders to use next
    // Render lit models, only change textures for each onee

    // Detach the water height map from being a source texture so it can be used as a render target again next frame (if you don't do this DX emits lots of warnings)
    
    if (enableTerrain)
    {
        gGround->Setup(gWorldTransformVertexShader, gTerrainPixelShader);
        gD3DContext->GSSetShader(gTriangleGeometryShader, nullptr, 0);
        gGround->SetStates(gNoBlendingState, gUseDepthBufferState, gCullNoneState);
        gGround->SetShaderResources(0, resourceManager->getTexture(L"Grass"));
        gGround->SetShaderResources(1, resourceManager->getTexture(L"Rock"));
        gGround->SetShaderResources(2, resourceManager->getTexture(L"Dirt"));
        gPerModelConstants.explodeAmount = 1;
        gGround->Render();
    }
    gD3DContext->GSSetShader(nullptr, nullptr, 0);

    //// Render lights ////
    if (enableLights)
    {
        // Select which shaders to use next
        gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
        gD3DContext->PSSetShader(gLightModelPixelShader, nullptr, 0);

        // Select the texture and sampler to use in the pixel shader
        ID3D11ShaderResourceView* LightTexture = resourceManager->getTexture(L"Light");
        gD3DContext->PSSetShaderResources(0, 1, &LightTexture); // First parameter must match texture slot number in the shaer
        

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

    //*********************//
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
    vp.Width  = static_cast<FLOAT>(1280);
    vp.Height = static_cast<FLOAT>(960);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    gD3DContext->RSSetViewports(1, &vp);

    // Render the scene from the main camera
    RenderSceneFromCamera(gCamera);

    ImGui::Begin("Controls", 0, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("");
    ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", gCamera->Position().x, gCamera->Position().y, gCamera->Position().z );
    ImGui::Text("Camera Rotation: (%.2f, %.2f, %.2f)", gCamera->Rotation().x, gCamera->Rotation().y, gCamera->Rotation().z );
    ImGui::Text("Ground Scale: (%.2f, %.2f, %.2f)", gGround->Scale().x, gGround->Scale().y, gGround->Scale().z );
    ImGui::Text("Ground Position: (%.2f, %.2f, %.2f)", gGround->Position().x, gGround->Position().y, gGround->Position().z );
    ImGui::Text("");


    ImGui::Checkbox("Render Terrain", &enableTerrain);
    if (enableTerrain)
    {
        ImGui::Text("");
        ImGui::SliderFloat("Terrain Frequency", &frequency, 0.01, 0.5);
        ImGui::SliderInt("Terrain amplitude", &amplitude, 5, 45);
        ImGui::SliderInt("Terrain Resolution", &sizeOfTerrain, 100, 135);
        //ImGui::SliderFloat("Terrain Scale", &TerrainYScale.y, 0.1, 10);
        ImGui::Text("");
        if (ImGui::Button("Generate Perlin Height Map"))
        {
            BuildPerlinHeightMap(amplitude, frequency);
            gGround->ResizeModel(heightMap, resolution);
        }
        if (ImGui::Button("Reset Terrain"))
        {
            BuildHeightMap();
            gGround->ResizeModel(heightMap, resolution);

        }
        ImGui::Button("Midpoint Displacement");
        ImGui::Text("");
        if (ImGui::Button("Display Perlin Height Map"))
        {
            std::ofstream MyFile("PerlinHeightMap.txt");
            int index = 0;
            float height;
            for (int i = 0; i <= resolution; ++i)
            {
                for (int j = 0; j <= resolution; ++j)
                {
                    height = heightMap[index];
                    MyFile << height << " ";
                    index++;
                }
                MyFile << "\n";
            }
            MyFile.close();
            index = 0;
        }
        ImGui::Text("");
        ImGui::Separator();
    }

    ImGui::Checkbox("Render Lights", &enableLights);
    if (enableLights)
    {
        ImGui::Text("");
        ImGui::ColorEdit3("Light One Colour", &gLights[0]->LightColour.x);
        ImGui::ColorEdit3("Light Two Colour", &gLights[1]->LightColour.x);
        ImGui::Text("");
        ImGui::SliderFloat("Light One Scale", &gLights[0]->LightStrength, 0.0f, 30.0f, "%.1f");
        ImGui::SliderFloat("Light Two Scale", &gLights[1]->LightStrength, 0.0f, 30.0f, "%.1f");
        ImGui::Text("");
        ImGui::Separator();
    }

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
void UpdateScene(float frameTime, HWND HWnd)
{
    // Orbit the light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
	static float rotate = 0.0f;
    static bool go = true;
	gLights[0]->LightModel->SetPosition( gGround->Position() + CVector3{ cos(rotate) * gLightOrbit, 10, sin(rotate) * gLightOrbit } );
    if (go)  rotate -= gLightOrbitSpeed * frameTime;
    if (KeyHit(Key_1))  go = !go;

	// Control camera (will update its view matrix)
	gCamera->Control(frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D );
    gGround->Control(NULL, frameTime, Key_I, Key_K, Key_J, Key_L, Key_U, Key_O, Key_Period, Key_Comma);
    gGround->SetScale(TerrainYScale);

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
        FPS = std::to_string(static_cast<int>(1 / avgFrameTime + 0.5f));
        std::string windowTitle = "Procedural Terrain Generation - FPS: " + FPS;
        SetWindowTextA(HWnd, windowTitle.c_str());
        totalFrameTime = 0;
        frameCount = 0;
    }
}
