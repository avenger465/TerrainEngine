#include "TerrainGenerationScene.h"

bool TerrainGenerationScene::InitGeometry(std::string& LastError)
{
    resourceManager = new CResourceManager();

    HeightMap.resize(SizeOfTerrain + 1, std::vector<float>(SizeOfTerrain + 1, 0));

    BuildHeightMap(1);
   
    PlantModels.resize(resizeAmount);

    resourceManager->loadTexture(L"default", "Media/v.bmp");

    // Load mesh geometry data, just like TL-Engine this doesn't create anything in the scene. Create a Model for that.
    try
    {
        resourceManager->loadGrid(L"TerrainMesh", CVector3(0, 0, 0), CVector3(1000, 0, 1000), SizeOfTerrainVertices, SizeOfTerrainVertices, HeightMap, true, true);
        resourceManager->loadMesh(L"Bush", std::string("Data/bush-01.fbx"));
        resourceManager->loadMesh(L"plant", std::string("Data/plant.fbx"), true);
        resourceManager->loadMesh(L"Light", std::string("Data/Light.x"));

    }
    catch (std::runtime_error e)  // Constructors cannot return error messages so use exceptions to catch mesh errors (fairly standard approach this)
    {
        LastError = e.what(); // This picks up the error message put in the exception (see Mesh.cpp)
        return false;
    }

    try
    {
        //resourceManager->loadTexture(L"Light", "Media/Flare.jpg");
        resourceManager->loadTexture(L"Grass", "Media/Grass1.png");
        resourceManager->loadTexture(L"Rock", "Media/rock1.png");
        resourceManager->loadTexture(L"Dirt", "Media/Dirt2.png");
        resourceManager->loadTexture(L"BarkTexture", "Media/Bark.jpg");
        resourceManager->loadTexture(L"bushTexture", "Media/BushTexture2.jpg");
        resourceManager->loadTexture(L"leaves", "Data/SampleLeaves_2.tga");
        resourceManager->loadTexture(L"leaves_AlphaMap", "Data/SampleLeaves_2_Alpha.tga");
        resourceManager->loadTexture(L"plantTexture", "Media/plant.png");
        resourceManager->loadTexture(L"plantTextureNormal", "Data/plantNormal.jpg");
        resourceManager->loadTexture(L"lightTexture", "Media/Flare.jpg");
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

    //**** Create Scene Texture ****//

    // Using a helper function to load textures from files above. Here we create the Scene texture manually
    // as we are creating a special kind of texture (one that we can render to). Many settings to prepare:
    D3D11_TEXTURE2D_DESC SceneDesc = {};
    SceneDesc.Width = textureWidth;  // Size of the portal texture determines its quality
    SceneDesc.Height = textureHeight;
    SceneDesc.MipLevels = 1; // No mip-maps when rendering to textures (or we would have to render every level)
    SceneDesc.ArraySize = 1;
    SceneDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA texture (8-bits each)
    SceneDesc.SampleDesc.Count = 1;
    SceneDesc.SampleDesc.Quality = 0;
    SceneDesc.Usage = D3D11_USAGE_DEFAULT;
    SceneDesc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE; // IMPORTANT: Indicate we will use texture as render target, and pass it to shaders
    SceneDesc.CPUAccessFlags = 0;
    SceneDesc.MiscFlags = 0;
    if (FAILED(gD3DDevice->CreateTexture2D(&SceneDesc, NULL, &SceneTexture)))
    {
        LastError = "Error creating Scene texture";
        return false;
    }

    // We created the Scene texture above, now we get a "view" of it as a render target, i.e. get a special pointer to the texture that
    // we use when rendering to it (see RenderScene function below)
    if (FAILED(gD3DDevice->CreateRenderTargetView(SceneTexture, NULL, &SceneRenderTarget)))
    {
        LastError = "Error creating Scene render target view";
        return false;
    }

    // We also need to send this texture (resource) to the shaders. To do that we must create a shader-resource "view"
    D3D11_SHADER_RESOURCE_VIEW_DESC srDesc = {};
    srDesc.Format = SceneDesc.Format;
    srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srDesc.Texture2D.MostDetailedMip = 0;
    srDesc.Texture2D.MipLevels = 1;
    if (FAILED(gD3DDevice->CreateShaderResourceView(SceneTexture, &srDesc, &SceneTextureSRV)))
    {
        LastError = "Error creating Scene shader resource view";
        return false;
    }


    //**** Create Scene Depth Buffer ****//
    //**** This depth buffer can be shared with any other textures of the same size
    SceneDesc = {};
    SceneDesc.Width = textureWidth;
    SceneDesc.Height = textureHeight;
    SceneDesc.MipLevels = 1;
    SceneDesc.ArraySize = 1;
    SceneDesc.Format = DXGI_FORMAT_D32_FLOAT; // Depth buffers contain a single float per pixel
    SceneDesc.SampleDesc.Count = 1;
    SceneDesc.SampleDesc.Quality = 0;
    SceneDesc.Usage = D3D11_USAGE_DEFAULT;
    SceneDesc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
    SceneDesc.CPUAccessFlags = 0;
    SceneDesc.MiscFlags = 0;
    if (FAILED(gD3DDevice->CreateTexture2D(&SceneDesc, NULL, &SceneDepthStencil)))
    {
        LastError = "Error creating Scene depth stencil texture";
        return false;
    }

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC sceneDescDSV = {};
    sceneDescDSV.Format = SceneDesc.Format;
    sceneDescDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    sceneDescDSV.Texture2D.MipSlice = 0;
    sceneDescDSV.Flags = 0;
    if (FAILED(gD3DDevice->CreateDepthStencilView(SceneDepthStencil, &sceneDescDSV, &SceneDepthStencilView)))
    {
        LastError = "Error creating Scene depth stencil view";
        return false;
    }

    // Create all filtering modes, blending modes etc.
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
    GroundModel = new Model(resourceManager->getMesh(L"TerrainMesh"), CVector3(0,0,0));
    Light = new CLight(resourceManager->getMesh(L"Light"), LightScale, LightColour, LightPosition, 0);

    for (int i = 0; i < PlantModels.size(); ++i)
    {
        PlantModels[i] = new Model(resourceManager->getMesh(L"plant"), CVector3(0.0f, -500.0f, 0.0f));
        PlantModels[i]->SetScale(0.1f);
    }

    //PlantModel = new Model(resourceManager->getMesh(L"plant"));
    //PlantModel->SetScale(2);

    windowFlags |= ImGuiWindowFlags_NoScrollbar;
    windowFlags |= ImGuiWindowFlags_AlwaysAutoResize;

    //// Set up camera ////

    MainCamera = new Camera();
    MainCamera->SetPosition(CameraPosition);
    MainCamera->SetRotation(CameraRotation);
    return true;
}

void TerrainGenerationScene::ReleaseResources()
{
    ReleaseStates();

    if (gPerModelConstantBuffer)  gPerModelConstantBuffer->Release();
    if (gPerFrameConstantBuffer)  gPerFrameConstantBuffer->Release();

    if (SceneTexture) SceneTexture->Release();
    if (SceneRenderTarget) SceneRenderTarget->Release();
    if (SceneTextureSRV) SceneTextureSRV->Release();
    if (SceneDepthStencil) SceneDepthStencil->Release();
    if (SceneDepthStencilView) SceneDepthStencilView->Release();

    ReleaseShaders();

    // See note in InitGeometry about why we're not using unique_ptr and having to manually delete
    delete MainCamera;     MainCamera = nullptr;
    delete GroundModel;     GroundModel = nullptr;
    for (int i = 0; i < PlantModels.size(); ++i)
    {
        delete PlantModels[i];
        PlantModels[i] = nullptr;
    }

    delete resourceManager; resourceManager = nullptr;

    delete Light; Light = nullptr;

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

    if (enableTerrain)
    {
        GroundModel->Setup(gWorldTransformVertexShader, gTerrainPixelShader);
        gD3DContext->GSSetShader(gTriangleGeometryShader, nullptr, 0);
        if (enableWireFrame)
        {
            GroundModel->SetStates(gNoBlendingState, gUseDepthBufferState, gWireframeState);
        }
        else
        {
            GroundModel->SetStates(gNoBlendingState, gUseDepthBufferState, gCullBackState);
        }
        GroundModel->SetShaderResources(0, resourceManager->getTexture(L"Grass"));
        GroundModel->SetShaderResources(1, resourceManager->getTexture(L"Rock"));
        GroundModel->SetShaderResources(2, resourceManager->getTexture(L"Dirt"));
        GroundModel->SetShaderResources(3, resourceManager->getTexture(L"Default"));
        GroundModel->Render(gPerModelConstantBuffer, gPerModelConstants);

        gD3DContext->GSSetShader(nullptr, nullptr, 0);

        for (int i = 0; i < PlantModels.size(); ++i)
        {
            PlantModels[i]->Setup(gNormalMappingVertexShader, gNormalMappingPixelShader);
            PlantModels[i]->SetShaderResources(0, resourceManager->getTexture(L"plantTexture"), 1, resourceManager->getTexture(L"plantTextureNormal"));
            PlantModels[i]->SetStates(gAlphaBlendingState, gUseDepthBufferState, gCullBackState);
            PlantModels[i]->Render(gPerModelConstantBuffer, gPerModelConstants);
        }
    }

    gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
    gD3DContext->PSSetShader(gLightModelPixelShader, nullptr, 0);

    // Select the texture and sampler to use in the pixel shader
    ID3D11ShaderResourceView* temp = resourceManager->getTexture(L"lightTexture");
    gD3DContext->PSSetShaderResources(0, 1, &temp); // First parameter must match texture slot number in the shaer
    gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler); 

    // States - additive blending, read-only depth buffer and no culling (standard set-up for blending
    gD3DContext->OMSetBlendState(gAdditiveBlendingState, nullptr, 0xffffff);
    gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
    gD3DContext->RSSetState(gCullNoneState);

    // Render all the lights in the array
     gPerModelConstants.objectColour = Light->LightColour; // Set any per-model constants apart from the world matrix just before calling render (light colour here)
     Light->RenderLight(gPerModelConstantBuffer, gPerModelConstants);

}

void TerrainGenerationScene::RenderScene(float frameTime)
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
    
    gPerFrameConstants.light1Colour = Light->LightColour * Light->LightStrength;
    gPerFrameConstants.light1Position = Light->LightModel->Position();
    gPerFrameConstants.ambientColour  = gAmbientColour;

    gPerFrameConstants.cameraPosition = MainCamera->Position();

    //// Main scene rendering ////

    // Set the back buffer as the target for rendering and select the main depth buffer.
    // When finished the back buffer is sent to the "front buffer" - which is the monitor.
    gD3DContext->OMSetRenderTargets(1, &SceneRenderTarget, SceneDepthStencilView);

    // Clear the back buffer to a fixed colour and the depth buffer to the far distance
    gD3DContext->ClearRenderTargetView(SceneRenderTarget, &gBackgroundColor.r);
    gD3DContext->ClearDepthStencilView(SceneDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Setup the viewport to the size of the main window
    D3D11_VIEWPORT vp;
    vp.Width = static_cast<FLOAT>(textureWidth);
    vp.Height = static_cast<FLOAT>(textureHeight);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    gD3DContext->RSSetViewports(1, &vp);

    // Render the scene from the main camera
    RenderSceneFromCamera(MainCamera);

    IMGUI();

    //*******************************

    //// Scene completion ////

    ImGui::Render();
    gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    //Check if multiple viewports are enabled and then call the appropriate functions
    //call the updating of viewports on the platforms side
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
    // When drawing to the off-screen back buffer is complete, we "present" the image to the front buffer (the screen)
    // Set first parameter to 1 to lock to vsync (typically 60fps)
    gSwapChain->Present(lockFPS ? 1 : 0, 0);
}

void TerrainGenerationScene::UpdateScene(float frameTime, HWND HWnd)
{

    if (KeyHit(Key_F2))
    {
        MainCamera->SetPosition(CameraPosition);
        MainCamera->SetRotation(CameraRotation);
    }
    // Toggle FPS limiting
    if (KeyHit(Key_T))  lockFPS = !lockFPS;

    // Control camera (will update its view matrix)
    MainCamera->Control(frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D);

    GroundModel->SetScale(TerrainYScale);

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
        FPS = static_cast<int>(1 / avgFrameTime + 0.5f);
        FPS_String = std::to_string(FPS);
        std::string windowTitle = "Procedural Terrain Generation - FPS: " + FPS_String;
        SetWindowTextA(HWnd, windowTitle.c_str());
        totalFrameTime = 0;
        frameCount = 0;
    }
}

void TerrainGenerationScene::UpdateFoliagePosition()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 10000);

    uint32_t OldRange = (10000 - 0);
    for (int i = 0; i < PlantModels.size(); ++i)
    {
        uint32_t randomXPos = dis(gen);
        uint32_t randomZPos = dis(gen);

        uint32_t NewXPos = (((randomXPos - 0) * SizeOfTerrain) / OldRange) + 0;
        uint32_t NewZPos = (((randomZPos - 0) * SizeOfTerrain) / OldRange) + 0;

        float Heightvalue = HeightMap[NewZPos][NewXPos];

        CVector3 position = { (float)randomXPos, (Heightvalue+0.1f), (float)randomZPos };
        position.y *= TerrainYScale.y;
        PlantModels[i]->SetScale(2);
        PlantModels[i]->SetPosition(position);
    }    
}

void TerrainGenerationScene::BuildHeightMap(float height)
{
    for (int i = 0; i <= SizeOfTerrain; ++i) {
        for (int j = 0; j <= SizeOfTerrain; ++j) {
            HeightMap[i][j] = height;
        }
    }
}

void TerrainGenerationScene::NormaliseHeightMap(float normaliseAmount)
{
    for (int i = 0; i <= SizeOfTerrain; ++i) {
        for (int j = 0; j <= SizeOfTerrain; ++j) {
            HeightMap[i][j] /= normaliseAmount;
        }
    }
}

void TerrainGenerationScene::BuildPerlinHeightMap(float amplitude, float frequency, bool bBrownianMotion)
{
    const float scale = (float)resolution / (float)SizeOfTerrain; 
    CPerlinNoise* pn = new CPerlinNoise(seed);

    for (int y = 0; y <= SizeOfTerrain; ++y) // loop through the y 
    {
        for (int x = 0; x <= SizeOfTerrain; ++x) //loop through the x
        {
            double YCoord = y * frequency * scale / 20;
            double XCoord = x * frequency * scale / 20;

            if (bBrownianMotion)
            {
                float noiseValue = (float)pn->noise(XCoord, YCoord, 0.0) * amplitude;
                HeightMap[y][x] += noiseValue;
            }
            else
            {        
                float noiseValue = (float)pn->noise(XCoord, YCoord, 0.0f) * amplitude;
                HeightMap[y][x] = noiseValue;
            }
        }
    }
}

void TerrainGenerationScene::PerlinNoiseWithOctaves(float Amplitude, float frequency, int octaves)
{
    for (int i = 0; i < octaves; ++i)
    {
        BuildPerlinHeightMap(Amplitude, frequency, true);
        Amplitude *= AmplitudeReduction;
        frequency *= FrequencyMultiplier;
    }
}

void TerrainGenerationScene::RigidNoise()
{
    const float scale = (float)resolution / (float)SizeOfTerrain; //make sure that the terrain looks consistent 
    CPerlinNoise* pn = new CPerlinNoise(seed);

    for (int i = 0; i <= SizeOfTerrain; ++i) // loop through the y 
    {
        for (int j = 0; j < SizeOfTerrain; ++j) //loop through the x
        {
            double x = j * frequency * scale / 20;
            double y = i * frequency * scale / 20;

            float value = (1.0f - abs((float)pn->noise(x, y, 0.0) * (float)Amplitude));
            HeightMap[i][j] += -value;
        }
    }
}

void TerrainGenerationScene::InverseRigidNoise()
{
    const float scale = (float)resolution / (float)SizeOfTerrain; //make sure that the terrain looks consistent 
    CPerlinNoise* pn = new CPerlinNoise(seed);

    for (int i = 0; i <= SizeOfTerrain; ++i) // loop through the y 
    {
        for (int j = 0; j <= SizeOfTerrain; ++j) //loop through the x
        {
             double x = j * frequency * scale / 20;
             double y = i * frequency * scale / 20;

             float value = (1.0f - abs((float)pn->noise(x, y, 0.0) * (float)Amplitude));
             HeightMap[i][j] += value;
        }
    }
}

void TerrainGenerationScene::DiamondSquareMap()
{
    DiamondSquare ds(SizeOfTerrain, Spread, SpreadReduction);
    ds.process(HeightMap);
}

void TerrainGenerationScene::Redistribution(float power)
{
    const float scale = (float)resolution / (float)SizeOfTerrain;
    for (int y = 0; y <= SizeOfTerrain; ++y) // loop through the y 
    {
        for (int x = 0; x <= SizeOfTerrain; ++x) //loop through the x
        {
            float noise_value = (abs(HeightMap[y][x]));
            HeightMap[y][x] = pow(noise_value, power);
        }
    }
}

void TerrainGenerationScene::Terracing(float terracingMultiplier)
{
    for (int y = 0; y <= SizeOfTerrain; ++y) // loop through the y 
    {
        for (int x = 0; x <= SizeOfTerrain; ++x) //loop through the x
        {
            float MapValue = HeightMap[y][x];
            float roundedValue = round(MapValue);
            HeightMap[y][x] = roundedValue / terracingMultiplier;
        }
    }
}

void TerrainGenerationScene::IMGUI()
{

    //---------------------------------------------------------------------------------------------------------------------//
    //First ImGui window created is a dock taken from the docking example in imgui_demo.cpp from the ImGui examples library//
    //---------------------------------------------------------------------------------------------------------------------//
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }
    else
    {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    if (!opt_padding)
        ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    //New ImGui window to contain all the buttons and sliders that the user will be able to use
    {
        {
            ImGui::Begin("Information", 0, windowFlags);;
            ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", MainCamera->Position().x,  MainCamera->Position().y,  MainCamera->Position().z);
            ImGui::Text("Camera Rotation: (%.2f, %.2f, %.2f)", MainCamera->Rotation().x,  MainCamera->Rotation().y,  MainCamera->Rotation().z);
            ImGui::Text("Ground Scale: (%.2f, %.2f, %.2f)",    GroundModel->Scale().x,    GroundModel->Scale().y,    GroundModel->Scale().z);
            ImGui::Text("Ground Position: (%.2f, %.2f, %.2f)", GroundModel->Position().x, GroundModel->Position().y, GroundModel->Position().z);
            ImGui::Text("");
            if(ImGui::Button("Toggle FPS", ImVec2(162, 20))) lockFPS = !lockFPS;
            ImGui::End();
        }
        ImGui::Begin("Terrain Generation", 0, windowFlags);
        ImGui::Checkbox("Render Terrain", &enableTerrain);
       
        if (enableTerrain)
        {

            //----------------------//
            // Reset Terrain Button //
            //----------------------//
            ImGui::Text("");
            if (ImGui::Button("Reset Terrain", ImVec2(162, 20)))
            {
                BuildHeightMap(1);
                GroundModel->ResizeModel(HeightMap, SizeOfTerrainVertices, CVector3(0, 0, 0), CVector3(1000, 0, 1000));

            }

            ImGui::SameLine();

            //---------------------//
            // Reset Camera Button //
            //---------------------//
            if (ImGui::Button("Reset Camera", ImVec2(162, 20)))
            {
                MainCamera->SetPosition(CameraPosition);
                MainCamera->SetRotation(CameraRotation);
            }
            if (ImGui::Button("Reset Terrain Settings", ImVec2(162, 20)))
            {
                frequency = 0.125f;
                Amplitude = 200.0f;
                resolution = 500;
                seed = 0;
                TerrainYScale = { 10, 30, 10 };
                RedistributionPower = 0.937f;
                terracingMultiplier = 1.1f;

                octaves = 5;
                AmplitudeReduction = 0.33f;
                FrequencyMultiplier = 1.5f;
                Spread = 30.0;
                SpreadReduction = 2.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Toggle WireFrame", ImVec2(162, 20))) enableWireFrame = !enableWireFrame;
            ImGui::Text("");

            //---------------------//
            // Terrain Information //
            //---------------------//
            ImGui::SliderFloat("Terrain Frequency", &frequency, 0.0f, 0.25f);
            ImGui::SliderFloat("Terrain amplitude", &Amplitude, 100.0f, 300.0f);
            ImGui::SliderInt("Terrain Resolution", &resolution, 250, 750);
            ImGui::SliderInt("Perlin Noise Seed", &seed, 0, 250);
            ImGui::SliderFloat("Terrain Scale", &TerrainYScale.y, 0.5f, 60.0f);
            ImGui::SliderFloat("Redistribution Power", &RedistributionPower, 0.900f, 0.975f);
            ImGui::SliderFloat("Terracing multiplier", &terracingMultiplier, 0.950f, 1.25f);
            ImGui::Text("");

            //------------------------------------------------------//
            // Generate new Terrain with the Perlin Noise Algorithm //
            //------------------------------------------------------//
            if (ImGui::Button("Perlin Noise", ImVec2(162, 20)))
            {
                BuildPerlinHeightMap(Amplitude, frequency, false);
                NormaliseHeightMap(2.0f);
                GroundModel->ResizeModel(HeightMap, SizeOfTerrainVertices, CVector3(0, 0, 0), CVector3(1000, 0, 1000));
                UpdateFoliagePosition();
            }

            //-----------------------------------------------------//
            // Generate new Terrain with the Rigid Noise Algorithm //
            //-----------------------------------------------------//
            ImGui::SameLine();
            if (ImGui::Button("Rigid Noise", ImVec2(162, 20)))
            {
                RigidNoise();
                NormaliseHeightMap(2.0f);
                GroundModel->ResizeModel(HeightMap, SizeOfTerrainVertices, CVector3(0, 0, 0), CVector3(1000, 0, 1000));
                UpdateFoliagePosition();
            }

            //-------------------------------------------------------------//
            // Generate new Terrain with the Inverse Rigid Noise Algorithm //
            //-------------------------------------------------------------//
            if (ImGui::Button("Inverse Rigid Noise", ImVec2(162, 20)))
            {
                InverseRigidNoise();
                NormaliseHeightMap(2.0f);
                GroundModel->ResizeModel(HeightMap, SizeOfTerrainVertices, CVector3(0, 0, 0), CVector3(1000, 0, 1000));
                UpdateFoliagePosition();
            }
            ImGui::SameLine();
            if (ImGui::Button("Redistribute", ImVec2(162, 20)))
            {
                Redistribution(RedistributionPower);
                GroundModel->ResizeModel(HeightMap, SizeOfTerrainVertices, CVector3(0, 0, 0), CVector3(1000, 0, 1000));
                UpdateFoliagePosition();
            }
            if (ImGui::Button("Terracing Multiplier", ImVec2(162, 20)))
            {
                Terracing(terracingMultiplier);
                GroundModel->ResizeModel(HeightMap, SizeOfTerrainVertices, CVector3(0, 0, 0), CVector3(1000, 0, 1000));
                UpdateFoliagePosition();
            }
            ImGui::SameLine();
            if (ImGui::Button("Display Height Map", ImVec2(162, 20)))
            {
                std::ofstream File("PerlinHeightMap.txt");

                for (int i = 0; i <= SizeOfTerrain; ++i) {
                    for (int j = 0; j <= SizeOfTerrain; ++j) {
                        File << HeightMap[i][j] * GroundModel->Scale().y << " ";
                    }
                }
                File.close();
            }

            ImGui::Text("");
            ImGui::Separator();
            ImGui::Text("");
            ImGui::SliderInt("Number of Octaves", &octaves, 1, 20);
            ImGui::SliderFloat("Amplitude Reduction", &AmplitudeReduction, 0.1f, 0.5f);
            ImGui::SliderFloat("Frequency Multiplier", &FrequencyMultiplier, 1.0f, 2.0f);
            ImGui::SliderFloat("Diamond Square Spread", &Spread, 10.0f, 40.0f);
            ImGui::SliderFloat("Diamond Square SpreadReduction", &SpreadReduction, 2.0f, 2.5f);
            ImGui::Text("");
            if (ImGui::Button("Perlin with Octaves", ImVec2(162, 20)))
            {
                BuildHeightMap(1);
                PerlinNoiseWithOctaves(Amplitude, frequency, octaves);
                NormaliseHeightMap(2.0f);
                GroundModel->ResizeModel(HeightMap, SizeOfTerrainVertices, CVector3(0, 0, 0), CVector3(1000, 0, 1000));
                UpdateFoliagePosition();
            }
            ImGui::SameLine();
            if (ImGui::Button("Diamond Square", ImVec2(162, 20)))
            {
                HeightMap.resize(SizeOfTerrain + 1, std::vector<float>(SizeOfTerrain + 1, 0));
                BuildHeightMap(1);
                DiamondSquareMap();
                GroundModel->ResizeModel(HeightMap, SizeOfTerrainVertices, CVector3(0, 0, 0), CVector3(1000, 0, 1000));
                UpdateFoliagePosition();
                /*HeightMap.resize(SizeOfTerrain, std::vector<float>(SizeOfTerrain, 0));*/
            }
            ImGui::Text("");
            ImGui::Separator();

            ImGui::Text("Number of plants per chunk: %i", PlantModels.size());
            ImGui::SliderInt("Change size of TestVector", &resizeAmount, 1, 20);
            if (ImGui::Button("Update Size"))
            {
                if (resizeAmount != CurrentPlantVectorSize)
                {
                    CurrentPlantVectorSize = resizeAmount;
                    PlantModels.resize(resizeAmount);
                    for (int i = 0; i < PlantModels.size(); ++i)
                    {
                        PlantModels[i] = new Model(resourceManager->getMesh(L"plant"));
                        PlantModels[i]->SetPosition(CVector3(0.0f, -1500.0f, 0.0f));
                        PlantModels[i]->SetScale(0.1f);
                    }
                    UpdateFoliagePosition();
                }
            }
        }
        ImGui::End();
    }

    //New ImGui window to contain the current scene rendered to a 2DTexture//
    {
        ImGui::Begin("Scene", 0, windowFlags);
        ImGui::Image(SceneTextureSRV, ImVec2((float)textureWidth, (float)textureHeight));
        ImGui::End();
    }

    ImGui::End();
}