#include "TerrainGenerationScene.h"

std::vector<float> m_HeightMap(129 * 129, 0);

bool TerrainGenerationScene::InitGeometry(std::string& LastError)
{
    resourceManager = new CResourceManager();

    Map = new float[(SizeOfTerrain+1) * (SizeOfTerrain+1)];

    HeightMap.resize(SizeOfTerrain + 1, std::vector<float>(SizeOfTerrain + 1, 0));
    BuildHeightMap();
   

    resourceManager->loadTexture(L"default", "Media/v.bmp");

    // Load mesh geometry data, just like TL-Engine this doesn't create anything in the scene. Create a Model for that.
    try
    {
        resourceManager->loadMesh(L"GroundMesh", std::string("Data/Hills.x"));
        //resourceManager->loadMesh(L"LightMesh", std::string("Data/Light.x"));
        resourceManager->loadMesh(L"SphereMesh", std::string("Data/Sphere.x"));
        resourceManager->loadGrid(L"TerrainMesh", CVector3(-200, 0, -200), CVector3(200, 0, 200), SizeOfTerrain, SizeOfTerrain, HeightMap, Map, true, true);
        resourceManager->loadMesh(L"SkyMesh", std::string("Data/Skybox.x"));
    }
    catch (std::runtime_error e)  // Constructors cannot return error messages so use exceptions to catch mesh errors (fairly standard approach this)
    {
        LastError = e.what(); // This picks up the error message put in the exception (see Mesh.cpp)
        return false;
    }

    try
    {
        //resourceManager->loadTexture(L"Light", "Media/Flare.jpg");
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
    //gTerrain= new Model(resourceManager->getMesh(L"TerrainMesh"), CVector3(400,0,0));

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

    ReleaseShaders();

    // See note in InitGeometry about why we're not using unique_ptr and having to manually delete
    delete MainCamera;     MainCamera = nullptr;
    delete GroundModel;     GroundModel = nullptr;
    delete TerrainModel; TerrainModel = nullptr;
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
        GroundModel->SetStates(gNoBlendingState, gUseDepthBufferState, gCullBackState);
        GroundModel->SetShaderResources(0, resourceManager->getTexture(L"Grass"));
        GroundModel->SetShaderResources(1, resourceManager->getTexture(L"Rock"));
        GroundModel->SetShaderResources(2, resourceManager->getTexture(L"Dirt"));
        GroundModel->SetShaderResources(3, resourceManager->getTexture(L"Default"));
        gPerModelConstants.explodeAmount = 1;
        GroundModel->Render(gPerModelConstantBuffer, gPerModelConstants);
        //gTerrain->Render(gPerModelConstantBuffer, gPerModelConstants);
    }
    gD3DContext->GSSetShader(nullptr, nullptr, 0);
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
    
    gPerFrameConstants.ambientColour  = gAmbientColour;
    gPerFrameConstants.specularPower  = gSpecularPower;
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

void TerrainGenerationScene::BuildHeightMap()
{

    float height = 1.0f;
    for (int i = 0; i <= SizeOfTerrain; ++i) {
        for (int j = 0; j <= SizeOfTerrain; ++j) {
            m_HeightMap.at(i + j) = height;
            Map[(i * SizeOfTerrain) + j] = height;
            HeightMap[i][j] = height;
        }
    }
}

void TerrainGenerationScene::BuildPerlinHeightMap(int Amplitude, float frequency, bool bBrownianMotion)
{
    const float scale = (float)resolution / (float)SizeOfTerrain; //make sure that the terrain looks consistent 
    CPerlinNoise* pn = new CPerlinNoise(seed);

    for (int i = 0; i <= SizeOfTerrain; ++i) // loop through the y 
    {
        for (int j = 0; j <= SizeOfTerrain; ++j) //loop through the x
        {
            double x = j * frequency * scale / 20;
            double y = i * frequency * scale / 20;
            if (!bBrownianMotion) {
                HeightMap[i][j] += (float)pn->noise(x, y, 0.8) * 20;
            }
            else
            {
                HeightMap[i][j] = (float)pn->noise(x, y, 0.8) * 20;
            }
        }
    }
}

void TerrainGenerationScene::BrownianMotion(int Amplitude, float frequency, int octaves)
{
    for (int i = 0; i < octaves; ++i)
    {
        BuildPerlinHeightMap(Amplitude, frequency, true);
        Amplitude = (int)(Amplitude * 0.5f);
        frequency *= 2;
    }
}

void TerrainGenerationScene::RigidNoise(int Amplitude, float frequency)
{
    const float scale = (float)resolution / (float)SizeOfTerrain; //make sure that the terrain looks consistent 
    CPerlinNoise* pn = new CPerlinNoise(seed);

    for (int i = 0; i <= SizeOfTerrain; ++i) // loop through the y 
    {
        for (int j = 0; j <= SizeOfTerrain; ++j) //loop through the x
        {
            double x = j * frequency * scale / 20;
            double y = i * frequency * scale / 20;
            m_HeightMap.at(i + j) += -(1.0f - abs((float)pn->noise(x, y, 0.8) * (float)Amplitude));
            Map[(i * SizeOfTerrain) + j] += -(1.0f - abs((float)pn->noise(x, y, 0.8) * (float)Amplitude));
            HeightMap[i][j] += -(1.0f - abs((float)pn->noise(x, y, 0.8) * (float)Amplitude));
        }
    }
}

void TerrainGenerationScene::InverseRigidNoise(int Amplitude, float frequency)
{
    const float scale = (float)resolution / (float)SizeOfTerrain; //make sure that the terrain looks consistent 
    CPerlinNoise* pn = new CPerlinNoise(seed);

    for (int i = 0; i <= SizeOfTerrain; ++i) // loop through the y 
    {
        for (int j = 0; j <= SizeOfTerrain; ++j) //loop through the x
        {
             double x = j * frequency * scale / 20;
             double y = i * frequency * scale / 20;
             m_HeightMap.at(i + j) += (1.0f - abs((float)pn->noise(x, y, 0.8) * (float)Amplitude));
             Map[(i * SizeOfTerrain) + j] += (1.0f - abs((float)pn->noise(x, y, 0.8) * (float)Amplitude));
             HeightMap[i][j] += (1.0f - abs((float)pn->noise(x, y, 0.8) * (float)Amplitude));
        }
    }
}

void TerrainGenerationScene::DiamondSquareMap(double range)
{
    DiamondSquare ds(SizeOfTerrain, range);
    ds.process(HeightMap);
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
            ImGui::Begin("Information", 0, windowFlags);
            //ImGui::Spacing();
            ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", MainCamera->Position().x, MainCamera->Position().y, MainCamera->Position().z);
            ImGui::Text("Camera Rotation: (%.2f, %.2f, %.2f)", MainCamera->Rotation().x, MainCamera->Rotation().y, MainCamera->Rotation().z);
            ImGui::Text("Ground Scale: (%.2f, %.2f, %.2f)", GroundModel->Scale().x, GroundModel->Scale().y, GroundModel->Scale().z);
            ImGui::Text("Ground Position: (%.2f, %.2f, %.2f)", GroundModel->Position().x, GroundModel->Position().y, GroundModel->Position().z);
            ImGui::Text("");
            if(ImGui::Button("Toggle FPS", ImVec2(162, 20)))
            {
                lockFPS = !lockFPS;
            }
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
                BuildHeightMap();
                GroundModel->ResizeModel(HeightMap, Map, SizeOfTerrain, CVector3(-200, 0, -200), CVector3(200, 0, 200));

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
                frequency = 0.45f;
                amplitude = 45;
                resolution = 500;
                seed = 0;
                TerrainYScale = { 10, 10, 10 };
                octaves = 10;
                Range = 2.0;
            }
            ImGui::Text("");

            //---------------------//
            // Terrain Information //
            //---------------------//
            ImGui::SliderFloat("Terrain Frequency", &frequency, 0.01f, 0.5f);
            ImGui::SliderInt("Terrain amplitude", &amplitude, 5, 45);
            ImGui::SliderInt("Terrain Resolution", &resolution, 250, 750);
            ImGui::SliderInt("Perlin Noise Seed", &seed, 0, 250);
            ImGui::SliderFloat("Terrain Scale", &TerrainYScale.y, 1.0f, 20.0f);
            ImGui::Text("");

            //------------------------------------------------------//
            // Generate new Terrain with the Perlin Noise Algorithm //
            //------------------------------------------------------//
            if (ImGui::Button("Perlin Noise", ImVec2(162, 20)))
            {
                BuildHeightMap();
                BuildPerlinHeightMap(amplitude, frequency, false);
                GroundModel->ResizeModel(HeightMap, Map, SizeOfTerrain, CVector3(-200, 0, -200), CVector3(200, 0, 200));
            }

            //-----------------------------------------------------//
            // Generate new Terrain with the Rigid Noise Algorithm //
            //-----------------------------------------------------//
            ImGui::SameLine();
            if (ImGui::Button("Rigid Noise", ImVec2(162, 20)))
            {
                BuildHeightMap();
                RigidNoise(amplitude, frequency);
                GroundModel->ResizeModel(HeightMap, Map, SizeOfTerrain, CVector3(-200, 0, -200), CVector3(200, 0, 200));
            }

            //-------------------------------------------------------------//
            // Generate new Terrain with the Inverse Rigid Noise Algorithm //
            //-------------------------------------------------------------//
            if (ImGui::Button("Inverse Rigid Noise", ImVec2(162, 20)))
            {
                BuildHeightMap();
                InverseRigidNoise(amplitude, frequency);
                GroundModel->ResizeModel(HeightMap, Map, SizeOfTerrain, CVector3(-200, 0, -200), CVector3(200, 0, 200));
            }

            ImGui::Text("");
            ImGui::Separator();
            ImGui::Text("");
            ImGui::SliderInt("Brownian Octaves", &octaves, 1, 20);
            ImGui::SliderFloat("Diamond Square Range", &Range, 1.0f, 10.0f);
            ImGui::Text("");
            if (ImGui::Button("Brownian Motion", ImVec2(162, 20)))
            {
                BuildHeightMap();
                BrownianMotion(amplitude, frequency, 10);
                GroundModel->ResizeModel(HeightMap, Map, SizeOfTerrain, CVector3(-200, 0, -200), CVector3(200, 0, 200));
            }
            ImGui::SameLine();
            if (ImGui::Button("Diamond Square", ImVec2(162, 20)))
            {
                BuildHeightMap();
                DiamondSquareMap((double)Range);
                GroundModel->ResizeModel(HeightMap, Map, SizeOfTerrain, CVector3(-200, 0, -200), CVector3(200, 0, 200));
            }
            ImGui::Text("");
            ImGui::Separator();
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