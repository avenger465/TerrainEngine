#include "TerrainGenerationScene.h"

bool TerrainGenerationScene::InitGeometry(std::string& LastError)
{
    resourceManager = new CResourceManager();
    BuildHeightMap();

    resourceManager->loadTexture(L"default", "Media/v.bmp");

    // Load mesh geometry data, just like TL-Engine this doesn't create anything in the scene. Create a Model for that.
    try
    {
        resourceManager->loadMesh(L"GroundMesh", std::string("Data/Hills.x"));
        resourceManager->loadMesh(L"LightMesh", std::string("Data/Light.x"));
        resourceManager->loadMesh(L"SphereMesh", std::string("Data/Sphere.x"));
        resourceManager->loadGrid(L"TerrainMesh", CVector3(-200, 0, -200), CVector3(200, 0, 200), resolution, resolution, heightMap, true, true);
        resourceManager->loadGrid(L"TerrainMesh1", CVector3(200, 0, -200), CVector3(600, 0, 200), resolution, resolution, heightMap, true, true);
        resourceManager->loadMesh(L"SkyMesh", std::string("Data/Skybox.x"));
    }
    catch (std::runtime_error e)  // Constructors cannot return error messages so use exceptions to catch mesh errors (fairly standard approach this)
    {
        LastError = e.what(); // This picks up the error message put in the exception (see Mesh.cpp)
        return false;
    }

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

    if (!LoadShaders(LastError))
    {
        LastError = "Error loading shaders";
        return false;
    }

    // Create GPU-side constant buffers to receive the gPerFrameConstants and gPerModelConstants structures above
    // These allow us to pass data from CPU to shaders such as lighting information or matrices
    // See the comments above where these variable are declared and also the UpdateScene function
    gPerFrameConstantBuffer = CreateConstantBuffer(sizeof(PerFrameConstants));
    gPerModelConstantBuffer = CreateConstantBuffer(sizeof(PerModelConstants));
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

bool TerrainGenerationScene::InitScene()
{
    //// Set up scene ////
    gGround = new Model(resourceManager->getMesh(L"TerrainMesh"), CVector3(1000,0,0));
    //gTerrain= new Model(resourceManager->getMesh(L"TerrainMesh"), CVector3(400,0,0));

    // Light set-up - using an array this time
    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        gLights[i] = new CLight(resourceManager->getMesh(L"LightMesh"), LightScale[i], LightsColour[i], LightsPosition[i], pow(LightScale[i], 0.7f));
    }

    //// Set up camera ////

    gCamera = new Camera();
    //gCamera->SetPosition({ 0, 270, -500 });
    gCamera->SetPosition({ -2.5, 1323, -2410 });
    gCamera->SetRotation({ ToRadians(28.6f), 0.0f, 0.0f });
    return true;
}

void TerrainGenerationScene::ReleaseResources()
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
    delete gCamera;     gCamera = nullptr;
    delete gGround;     gGround = nullptr;
    delete gTerrain; gTerrain = nullptr;
}

void TerrainGenerationScene::RenderSceneFromCamera(Camera* camera)
{
    // Set camera matrices in the constant buffer and send over to GPU
    gPerFrameConstants.viewMatrix = camera->ViewMatrix();
    gPerFrameConstants.projectionMatrix = camera->ProjectionMatrix();
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
        gGround->SetStates(gNoBlendingState, gUseDepthBufferState, gCullBackState);
        gGround->SetShaderResources(0, resourceManager->getTexture(L"Grass"));
        gGround->SetShaderResources(1, resourceManager->getTexture(L"Rock"));
        gGround->SetShaderResources(2, resourceManager->getTexture(L"Dirt"));
        gPerModelConstants.explodeAmount = 1;
        gGround->Render(gPerModelConstantBuffer, gPerModelConstants);
        //gTerrain->Render(gPerModelConstantBuffer, gPerModelConstants);
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
            gLights[i]->RenderLight(gPerModelConstantBuffer, gPerModelConstants);
        }
    }
}

void TerrainGenerationScene::RenderScene()
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
    gPerFrameConstants.light1Colour = gLights[0]->LightColour * gLights[0]->LightStrength;
    gPerFrameConstants.light1Position = gLights[0]->LightModel->Position();
    gPerFrameConstants.light2Colour = gLights[1]->LightColour * gLights[1]->LightStrength;
    gPerFrameConstants.light2Position = gLights[1]->LightModel->Position();
    
    gPerFrameConstants.ambientColour = gAmbientColour;
    gPerFrameConstants.specularPower = gSpecularPower;
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
    vp.Width = static_cast<FLOAT>(viewportWidth);
    vp.Height = static_cast<FLOAT>(viewportHeight);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    gD3DContext->RSSetViewports(1, &vp);

    // Render the scene from the main camera
    RenderSceneFromCamera(gCamera);

    IMGUI();

    //*******************************

    //// Scene completion ////

    ImGui::Render();
    gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());


    // When drawing to the off-screen back buffer is complete, we "present" the image to the front buffer (the screen)
    // Set first parameter to 1 to lock to vsync (typically 60fps)
    gSwapChain->Present(lockFPS ? 1 : 0, 0);
}

void TerrainGenerationScene::UpdateScene(float frameTime, HWND HWnd)
{
    // Orbit the light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
    static float rotate = 0.0f;
    static bool go = true;
    gLights[0]->LightModel->SetPosition(gGround->Position() + CVector3{ cos(rotate) * gLightOrbit, 10, sin(rotate) * gLightOrbit });
    if (go)  rotate -= gLightOrbitSpeed * frameTime;
    if (KeyHit(Key_1))  go = !go;

    // Control camera (will update its view matrix)
    gCamera->Control(frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D);
    //gTerrain->Control(0,frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D);
    gGround->SetScale(TerrainYScale);
    //gTerrain->SetScale(TerrainYScale);

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

void TerrainGenerationScene::BuildHeightMap()
{
    float b = (resolution) * (resolution);
    heightMap = new float*[resolution];
    float height = 1.0f;
    int index = 0;
    for (int i = 0; i <= resolution; ++i) {
        heightMap[i] = new float[resolution];
        for (int j = 0; j <= resolution; ++j) {
            heightMap[i][j] = height;
            index++;
            //heightMap[i][j] = height;
        }
    }
}

void TerrainGenerationScene::BuildPerlinHeightMap(int Amplitude, float frequency)
{
    const float scale = (float)sizeOfTerrain / (float)resolution; //make sure that the terrain looks consistent 
    CPerlinNoise* pn = new CPerlinNoise(seed);

    for (int i = 0; i <= resolution; ++i) // loop through the y 
    {
        for (int j = 0; j <= resolution; ++j) //loop through the x
        {
            double x = j * frequency * scale / 20;
            double y = i * frequency * scale / 20;
            heightMap[i][j] += (float)pn->noise(x, y, 0.8)* 20;//(float)Amplitude;
        }
    }
}

void TerrainGenerationScene::BrownianMotion(int Amplitude, float frequency, int octaves)
{
    BuildHeightMap();
    for (int i = 0; i < octaves; ++i)
    {
        BuildPerlinHeightMap(Amplitude, frequency);
        Amplitude = (int)(Amplitude * 0.5f);
        frequency *= 2;
    }
}

void TerrainGenerationScene::RigidNoise(int Amplitude, float frequency)
{
    const float scale = (float)sizeOfTerrain / (float)resolution; //make sure that the terrain looks consistent 
    CPerlinNoise* pn = new CPerlinNoise(seed);

    for (int i = 0; i <= resolution; ++i) // loop through the y 
    {
        for (int j = 0; j <= resolution; ++j) //loop through the x
        {
            double x = j * frequency * scale / 20;
            double y = i * frequency * scale / 20;
            heightMap[i][j]/*(i * (resolution + 1)) + j]*/ += -(1.0f - abs((float)pn->noise(x, y, 0.8) * (float)Amplitude));
        }
    }
}

void TerrainGenerationScene::InverseRigidNoise(int Amplitude, float frequency)
{
    const float scale = (float)sizeOfTerrain / (float)resolution; //make sure that the terrain looks consistent 
    CPerlinNoise* pn = new CPerlinNoise(seed);

    for (int i = 0; i <= resolution; ++i) // loop through the y 
    {
        for (int j = 0; j <= resolution; ++j) //loop through the x
        {
             double x = j * frequency * scale / 20;
             double y = i * frequency * scale / 20;
            heightMap[i][j]/*(i * (resolution + 1)) + j]*/ += (1.0f - abs((float)pn->noise(x, y, 0.8) * (float)Amplitude));
        }
    }
}

void TerrainGenerationScene::DiamondSquareMap()
{
    DiamondSquare ds(heightMap, resolution);
    heightMap = ds.process();


}

void TerrainGenerationScene::IMGUI()
{
    ImGui::Begin("Controls", 0, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", gCamera->Position().x, gCamera->Position().y, gCamera->Position().z);
    ImGui::Text("Camera Rotation: (%.2f, %.2f, %.2f)", gCamera->Rotation().x, gCamera->Rotation().y, gCamera->Rotation().z);
    ImGui::Text("Ground Scale: (%.2f, %.2f, %.2f)", gGround->Scale().x, gGround->Scale().y, gGround->Scale().z);
    ImGui::Text("Ground Position: (%.2f, %.2f, %.2f)", gGround->Position().x, gGround->Position().y, gGround->Position().z);
    ImGui::Text("Frequency: (%.2f)", frequency);
    ImGui::Text("Amplitude: (%i)", amplitude);
    ImGui::Text("");
    ImGui::Separator();
    ImGui::Checkbox("Render Terrain", &enableTerrain);
    if (enableTerrain)
    {
        ImGui::Text("");
        if (ImGui::Button("Reset Terrain"))
        {
            BuildHeightMap();
            gGround->ResizeModel(heightMap, resolution, CVector3(-200, 0, -200), CVector3(200, 0, 200));

        }
        ImGui::Text("");
        ImGui::SliderFloat("Terrain Frequency", &frequency, 0.01f, 0.5f);
        ImGui::SliderInt("Terrain amplitude", &amplitude, 5, 45);
        ImGui::SliderInt("Terrain Resolution", &sizeOfTerrain, 200, 500);
        ImGui::SliderInt("Perlin Noise Seed", &seed, 1, 250);
        ImGui::SliderFloat("Terrain Scale", &TerrainYScale.y, 1.0f, 10.0f);
        ImGui::Text("");
        if (ImGui::Button("Generate Perlin Height Map"))
        {
            BuildPerlinHeightMap(amplitude, frequency);
            gGround->ResizeModel(heightMap, resolution, CVector3(-200, 0, -200), CVector3(200, 0, 200));
            //gTerrain->ResizeModel(heightMap, resolution, CVector3(198, 0, -200), CVector3(598, 0, 200));
        }
        if (ImGui::Button("Rigid Noise"))
        {
            RigidNoise(amplitude, frequency);
            gGround->ResizeModel(heightMap, resolution, CVector3(-200, 0, -200), CVector3(200, 0, 200));
        }
        if (ImGui::Button("Inverse Rigid Noise"))
        {
            InverseRigidNoise(amplitude, frequency);
            gGround->ResizeModel(heightMap, resolution, CVector3(-200, 0, -200), CVector3(200, 0, 200));
        }
        ImGui::Text("");
        ImGui::Separator();
        ImGui::SliderInt("Brownian Octaves", &octaves, 1, 20);
        if (ImGui::Button("Brownian Motion"))
        {
            BrownianMotion(amplitude, frequency, 10);
            gGround->ResizeModel(heightMap, resolution, CVector3(-200, 0, -200), CVector3(200, 0, 200));
        }
        if (ImGui::Button("Midpoint Displacement"))
        {
            DiamondSquareMap();
            gGround->ResizeModel(heightMap, resolution, CVector3(-200, 0, -200), CVector3(200, 0, 200));
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
}
