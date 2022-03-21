#include "TerrainGenerationScene.h"

//Function to setup all the geometry to be used in the scene
bool TerrainGenerationScene::InitGeometry(std::string& LastError)
{
    //Create a new resourceManager
    resourceManager = new CResourceManager();

    //Update the size of the HeightMap
    // --Has to be equal to 2^n + 1 in order for the Diamond Square algorithm to work on the HeightMap
    HeightMap.resize(SizeOfTerrain + 1, std::vector<float>(SizeOfTerrain + 1, 0));

    //Build the HeightMap with the value of 1
    BuildHeightMap(1);
   
    //Update the size of the PlantModels vectors
    PlantModels.resize(plantResizeAmount);

    //Load the texture that will be used if a texture cannot be loaded in correctly
    resourceManager->loadTexture(L"default", "Media/DefaultDiffuse.png");

    // Load mesh geometry data, just like TL-Engine this doesn't create anything in the scene. Create a Model for that.
    // add the meshes to the resourceManager to make getting these meshes later easier
    try
    {
        resourceManager->loadGrid(L"TerrainMesh", TerrainMeshMinPt, TerrainMeshMaxPt, SizeOfTerrainVertices, SizeOfTerrainVertices, HeightMap);
        resourceManager->loadMesh(L"plant", std::string("Data/Plant.fbx"), true);
        resourceManager->loadMesh(L"Light", std::string("Data/Light.x"));

    }
    catch (std::runtime_error e)  // Constructors cannot return error messages so use exceptions to catch mesh errors (fairly standard approach this)
    {
        LastError = e.what();     // This picks up the error message put in the exception (see Mesh.cpp)
        return false;
    }

    //Load the textures that will be used with the models to the resource manager
    try
    {
        resourceManager->loadTexture(L"Grass", "Media/Grass.png");
        resourceManager->loadTexture(L"Rock", "Media/Rock.png");
        resourceManager->loadTexture(L"Dirt", "Media/Dirt.png");
        resourceManager->loadTexture(L"plantTexture", "Media/Plant.png");
        resourceManager->loadTexture(L"plantTextureNormal", "Data/PlantNormal.jpg");
        resourceManager->loadTexture(L"lightTexture", "Media/Flare.jpg");
    }
    catch (std::runtime_error e) // Constructors cannot return error messages so use exceptions to catch texture errors (fairly standard approach this)
    {
        LastError = e.what();  // This picks up the error message put in the exception (see Mesh.cpp)
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

    // Create all states that can be used in the scene
    if (!CreateStates(LastError))
    {
        LastError = "Error creating states";
        return false;
    }

    return true;
}

//Function to setup the scene 
bool TerrainGenerationScene::InitScene()
{
    //Setup the GroundModel and the Light in the scene
    GroundModel = new Model(resourceManager->getMesh(L"TerrainMesh"), CVector3(0,0,0));
    Light       = new CLight(resourceManager->getMesh(L"Light"), LightScale, LightColour, LightPosition, 0);

    //Loop through every plant in the vector and create a plant model
    for (int i = 0; i < PlantModels.size(); ++i)
    {
        PlantModels[i] = new Model(resourceManager->getMesh(L"plant"), CVector3(0.0f, -500.0f, 0.0f));
        PlantModels[i]->SetScale(0.1f);
    }

    //Setting up the flags that will be used when creating the ImGui windows
    windowFlags |= ImGuiWindowFlags_NoScrollbar;
    windowFlags |= ImGuiWindowFlags_AlwaysAutoResize;

    //// Set up camera ////
    MainCamera = new Camera();
    MainCamera->SetPosition(CameraPosition);
    MainCamera->SetRotation(CameraRotation);

    return true;
}

//Function to release everything in the scene from memory
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

//Function to render scene from the Camera's perspective
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
    gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler);

    //Only render these models if the terrain has been chosen to be rendered
    if (enableTerrain)
    {
        //set the Pixel, Vertex and Geometry shaders that will be used for the ground Model
        GroundModel->Setup(gWorldTransformVertexShader, gTerrainPixelShader);
        gD3DContext->GSSetShader(gTriangleGeometryShader, nullptr, 0);

        //check if the model needs to be renderedin wireframe mode
        if (enableWireFrame) GroundModel->SetStates(gNoBlendingState, gUseDepthBufferState, gWireframeState);
        else GroundModel->SetStates(gNoBlendingState, gUseDepthBufferState, gCullBackState);

        //Set the resources that the Pixel shader will need to have to work
        GroundModel->SetShaderResources(0, resourceManager->getTexture(L"Grass"));
        GroundModel->SetShaderResources(1, resourceManager->getTexture(L"Rock"));
        GroundModel->SetShaderResources(2, resourceManager->getTexture(L"Dirt"));

        //Render the model
        GroundModel->Render(gPerModelConstantBuffer, gPerModelConstants);

        //Set the Geometry shader to be a nullptr as we do not need it anymore
        gD3DContext->GSSetShader(nullptr, nullptr, 0);

        //Loop through each plant in the scene and update the shaders and the required resources for the shaders.
        //Also update the states for the model and then render the model
        for (int i = 0; i < PlantModels.size(); ++i)
        {
            PlantModels[i]->Setup(gNormalMappingVertexShader, gNormalMappingPixelShader);
            PlantModels[i]->SetShaderResources(0, resourceManager->getTexture(L"plantTexture"), 1, resourceManager->getTexture(L"plantTextureNormal"));
            PlantModels[i]->SetStates(gAlphaBlendingState, gUseDepthBufferState, gCullBackState);
            PlantModels[i]->Render(gPerModelConstantBuffer, gPerModelConstants);
        }

    }

    //Update the Pixel and Vertex shaders that will be used when rendering a light
    gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
    gD3DContext->PSSetShader(gLightModelPixelShader, nullptr, 0);

    // Select the texture and sampler to use in the pixel shader
    ID3D11ShaderResourceView* temp = resourceManager->getTexture(L"lightTexture");
    gD3DContext->PSSetShaderResources(0, 1, &temp); 
    gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler); 

    //Update the states required for the Light Model
    Light->SetLightStates(gAdditiveBlendingState, gDepthReadOnlyState, gCullNoneState);

    // Render all the lights in the array
     gPerModelConstants.objectColour = Light->LightColour; 
     Light->RenderLight(gPerModelConstantBuffer, gPerModelConstants);

}

//Function to render the scene, called every frame
void TerrainGenerationScene::RenderScene(float frameTime)
{
    //------------------------------//
    // Prepare ImGUI for this frame //
    //------------------------------//
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    //-----------------//
    // Common settings //
    //-----------------//
    
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

    //Setup the ImGui elements to be rendered
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

//Updating the scene, called every frame
void TerrainGenerationScene::UpdateScene(float frameTime, HWND HWnd)
{
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

//Function to update the position of every plant in the scene
void TerrainGenerationScene::UpdateFoliagePosition()
{
    //Creation of a random number generation device
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1000);

    //The two ranges to convert the random number to
    uint32_t TerrainRange = 1000;
    uint32_t HeightMapRange = 256;

    //Loop through each plant in the vector
    for (int i = 0; i < PlantModels.size(); ++i)
    {
        //Get the random X and Z positions
        uint32_t randomXPos = dis(gen);
        uint32_t randomZPos = dis(gen);

        //Convert the X and Z positions from the range (0 - 1000) to (0 - 256)
        uint32_t NewXPos = (((randomXPos - 0) * HeightMapRange) / TerrainRange) + 0;
        uint32_t NewZPos = (((randomZPos - 0) * HeightMapRange) / TerrainRange) + 0;

        //Get the height Value from these new X and Z coordinates
        float Heightvalue = HeightMap[NewZPos][NewXPos];

        //Create a position Vector with the X and Z positions and the new height value 
        CVector3 position = { (float)randomXPos, (Heightvalue), (float)randomZPos };

        //Scale the vector by the overall TerrainYScale
        position *= TerrainYScale;

        //Scale the current plant
        PlantModels[i]->SetScale(1);

        //Update the position of the current plant to the new position vector
        PlantModels[i]->SetPosition({position.x, position.y-3, position.z});
    }   
}

//Building the HeightMap
void TerrainGenerationScene::BuildHeightMap(float height)
{
    //Loop through the HeightMap and set each value to the chosen height value
    for (int i = 0; i <= SizeOfTerrain; ++i) {
        for (int j = 0; j <= SizeOfTerrain; ++j) {
            HeightMap[i][j] = height;
        }
    }
}

//Normalisation of the HeightMap
void TerrainGenerationScene::NormaliseHeightMap(float normaliseAmount)
{
    //Loop through the HeightMap and divide each value by the normalisation amount
    for (int i = 0; i <= SizeOfTerrain; ++i) {
        for (int j = 0; j <= SizeOfTerrain; ++j) {
            HeightMap[i][j] /= normaliseAmount;
        }
    }
}

//Function to build the height map with the Perlin Noise Algorithm
void TerrainGenerationScene::BuildPerlinHeightMap(float amplitude, float frequency, bool bOctaves)
{
    //get the scale to make sure that the terrain looks consistent 
    const float scale = (float)resolution / (float)SizeOfTerrain; 

    //Create a PerlinNoise object to get the Perlin Noise values
    CPerlinNoise* pn = new CPerlinNoise(seed);

    for (int z = 0; z <= SizeOfTerrain; ++z) // loop through the z 
    {
        for (int x = 0; x <= SizeOfTerrain; ++x) //loop through the x
        {
            //Get the current X and Z coordinates 
            double ZCoord = z * frequency * scale / 20;
            double XCoord = x * frequency * scale / 20;

            //get the Current Perlin Noise value
            float noiseValue = (float)pn->noise(XCoord, 0.0f, ZCoord) * amplitude;

            //Check if Perlin with Octaves has been selected and add it to the current HeightMap value if it has
            //Otherwise just update the HeightMap value to the new noiseValue
            if (bOctaves)
            {
                HeightMap[z][x] += noiseValue;
            }
            else
            {        
                HeightMap[z][x] = noiseValue;
            }
        }
    }
}

//Perlin Noise with Octaves Function
void TerrainGenerationScene::PerlinNoiseWithOctaves(float Amplitude, float frequency, int octaves)
{
    //loop through the number of octaves
    for (int i = 0; i < octaves; ++i)
    {
        //Update the HeightMap with the Perlin noise function using the current Amplitude and Frequency
        BuildPerlinHeightMap(Amplitude, frequency, true);

        //Update the Amplitude and Frequency variables
        Amplitude *= AmplitudeReduction;
        frequency *= FrequencyMultiplier;
    }
}

//Rigid Noise Function
void TerrainGenerationScene::RigidNoise()
{
    //get the scale to make sure that the terrain looks consistent 
    const float scale = (float)resolution / (float)SizeOfTerrain;

    //Create a PerlinNoise object to get the Perlin Noise values
    CPerlinNoise* pn = new CPerlinNoise(seed);

    for (int z = 0; z <= SizeOfTerrain; ++z) // loop through the z
    {
        for (int x = 0; x < SizeOfTerrain; ++x) //loop through the x
        {
            //Get the current X and Z coordinates 
            double XCoord = x * frequency * scale / 20;
            double ZCoord = z * frequency * scale / 20;

            //Get the absolute value of the perlin noise function and subtract it from 1
            float value = (1.0f - abs((float)pn->noise(XCoord, 0.0f, ZCoord) * (float)Amplitude));

            //Then add the negated value to the current HeighMap value
            HeightMap[z][x] += -value;
        }
    }
}

//Inverse Rigid Noise Function
void TerrainGenerationScene::InverseRigidNoise()
{
    //get the scale to make sure that the terrain looks consistent 
    const float scale = (float)resolution / (float)SizeOfTerrain;

    //Create a PerlinNoise object to get the Perlin Noise values
    CPerlinNoise* pn = new CPerlinNoise(seed);

    for (int z = 0; z <= SizeOfTerrain; ++z) // loop through the z
    {
        for (int x = 0; x <= SizeOfTerrain; ++x) //loop through the x
        {
            //Get the current X and Z coordinates 
             double currentX = x * frequency * scale / 20;
             double currentZ = z * frequency * scale / 20;

             //Get the absolute value of the perlin noise function and subtract it from 1
             float value = (1.0f - abs((float)pn->noise(currentX, 0.0f, currentZ) * (float)Amplitude));

             //Then add the value to the current HeighMap value
             HeightMap[z][x] += value;
        }
    }
}

//Function to call the Diamond Sqaure Algorithm
void TerrainGenerationScene::DiamondSquareMap()
{
    //Create a new DiamondSqaure object with the required information
    DiamondSquare ds(SizeOfTerrain, Spread, SpreadReduction);

    //perform the Diamond Square Algorithm on the HeightMap
    ds.process(HeightMap);
}

//Terracing Function
void TerrainGenerationScene::Terracing(float terracingMultiplier)
{
    for (int x = 0; x <= SizeOfTerrain; ++x) // loop through the x
    {
        for (int z = 0; z <= SizeOfTerrain; ++z) //loop through the z
        {
            //Get the current height value
            float MapValue = HeightMap[x][z];

            //Round this height value
            float roundedValue = round(MapValue);

            //and then set the height to the rounded value divided by the multiplier
            HeightMap[x][z] = roundedValue / terracingMultiplier;
        }
    }
}

//Function to contain all of the ImGui code
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

    //New ImGui window to contain all the buttons and sliders that
    //the user will be able to interact with
    {
        //a new Window showing information of the scene and provides a few toggle buttons
        {
            ImGui::Begin("Information", 0, windowFlags);;
            ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", MainCamera->Position().x,  MainCamera->Position().y,  MainCamera->Position().z);
            ImGui::Text("Camera Rotation: (%.2f, %.2f, %.2f)", MainCamera->Rotation().x,  MainCamera->Rotation().y,  MainCamera->Rotation().z);
            ImGui::Text("");
            if(ImGui::Button("Toggle FPS", ButtonSize)) lockFPS = !lockFPS;
            ImGui::SameLine();
            if (ImGui::Button("Toggle WireFrame", ButtonSize)) enableWireFrame = !enableWireFrame;

            //end of the information window
            ImGui::End();
        }

        //a new window for generating new terrain
        ImGui::Begin("Terrain Generation", 0, windowFlags);
        ImGui::Checkbox("Render Terrain", &enableTerrain);
       
        //If the terrain checkbox is checked, then show the information that changes the terrain
        if (enableTerrain)
        {
            ImGui::Text("");

            //----------------------//
            // Reset Terrain Button //
            //----------------------//
            //goes through every value in the height map and resets it to 1 and
            //then goes through each plant in the scene and updates its position to the new height map
            if (ImGui::Button("Reset Terrain", ButtonSize))
            {
                BuildHeightMap(1);
                GroundModel->ResizeModel(HeightMap, SizeOfTerrainVertices, TerrainMeshMinPt, TerrainMeshMaxPt);
                for (auto i : PlantModels)
                {
                    i->SetPosition(CVector3{i->Position().x, 1, i->Position().z});
                }

            }
            ImGui::SameLine();

            //---------------------//
            // Reset Camera Button //
            //---------------------//
            //move the camera back to its starting position and rotates it accordingly
            if (ImGui::Button("Reset Camera", ButtonSize))
            {
                MainCamera->SetPosition(CameraPosition);
                MainCamera->SetRotation(CameraRotation);
            }

            //----------------------//
            // Reset Terrain Button //
            //----------------------//
            //resets every variable that affects the terrain generation to their starting values
            if (ImGui::Button("Reset Terrain Settings", ButtonSize))
            {
                frequency = 0.125f;
                Amplitude = 200.0f;
                resolution = 500;
                seed = 0;
                TerrainYScale = { 10, 30, 10 };
                terracingMultiplier = 1.1f;

                octaves = 5;
                AmplitudeReduction = 0.33f;
                FrequencyMultiplier = 1.5f;
                Spread = 30.0;
                SpreadReduction = 2.0f;
            }
            ImGui::Text("");

            //-----------------------------------//
            // Terrain Information and Variables //
            //-----------------------------------//
            //Sliders to update the terrain generation variables
            ImGui::SliderFloat("Terrain Frequency", &frequency, 0.0f, 0.25f);
            ImGui::SliderFloat("Terrain amplitude", &Amplitude, 100.0f, 300.0f);
            ImGui::SliderInt("Terrain Resolution", &resolution, 250, 750);
            ImGui::SliderInt("Perlin Noise Seed", &seed, 0, 250);
            ImGui::SliderFloat("Terrain Scale", &TerrainYScale.y, 0.5f, 60.0f);
            ImGui::Text("");

            //------------------------------------------------------//
            // Generate new Terrain with the Perlin Noise Algorithm //
            //------------------------------------------------------//
            //generates new height values with the Perlin Noise Algorithm
            //then normalises the height map and resizes the terrain mesh with these new height values
            //finally updates the positions of the plants in the scene
            if (ImGui::Button("Perlin Noise", ButtonSize))
            {
                BuildPerlinHeightMap(Amplitude, frequency, false);
                NormaliseHeightMap(HeightMapNormaliseAmount);
                GroundModel->ResizeModel(HeightMap, SizeOfTerrainVertices, TerrainMeshMinPt, TerrainMeshMaxPt);
                UpdateFoliagePosition();
            }

            //-----------------------------------------------------//
            // Generate new Terrain with the Rigid Noise Algorithm //
            //-----------------------------------------------------//
            //generates new height values with the Rigid Noise Algorithm
            //then normalises the height map and resizes the terrain mesh with these new height values
            //finally updates the positions of the plants in the scene
            ImGui::SameLine();
            if (ImGui::Button("Rigid Noise", ButtonSize))
            {
                RigidNoise();
                NormaliseHeightMap(HeightMapNormaliseAmount);
                GroundModel->ResizeModel(HeightMap, SizeOfTerrainVertices, TerrainMeshMinPt, TerrainMeshMaxPt);
                UpdateFoliagePosition();
            }

            //-------------------------------------------------------------//
            // Generate new Terrain with the Inverse Rigid Noise Algorithm //
            //-------------------------------------------------------------//
            //generates new height values with the Inverse Rigid Noise Algorithm
            //then normalises the height map and resizes the terrain mesh with these new height values
            //finally updates the positions of the plants in the scene
            if (ImGui::Button("Inverse Rigid Noise", ButtonSize))
            {
                InverseRigidNoise();
                NormaliseHeightMap(HeightMapNormaliseAmount);
                GroundModel->ResizeModel(HeightMap, SizeOfTerrainVertices, TerrainMeshMinPt, TerrainMeshMaxPt);
                UpdateFoliagePosition();
            }
            
            ImGui::Text("");
            ImGui::Separator();
            ImGui::Text("");

            //-----------------------------------//
            // Terrain Information and Variables //
            //-----------------------------------//
            //Sliders to update the terrain generation variables
            ImGui::SliderInt("Number of Octaves", &octaves, 1, 20);
            ImGui::SliderFloat("Amplitude Reduction", &AmplitudeReduction, 0.1f, 0.5f);
            ImGui::SliderFloat("Frequency Multiplier", &FrequencyMultiplier, 1.0f, 2.0f);
            ImGui::SliderFloat("DS Spread", &Spread, 10.0f, 40.0f);
            ImGui::SliderFloat("DS Spread Reduction", &SpreadReduction, 2.0f, 2.5f);
            ImGui::SliderFloat("Terracing multiplier", &terracingMultiplier, 0.950f, 1.25f);
            ImGui::Text("");

            //-----------------------------------------------------------------------//
            // Generate new Terrain with the Perlin Noise Algorithm with octaves     //
            //-----------------------------------------------------------------------//
            //generates new height values with the Perlin Noise algorithm but with octaves
            //then normalises the height map and resizes the terrain mesh with these new height values
            //finally updates the positions of the plants in the scene
            if (ImGui::Button("Perlin with Octaves", ButtonSize))
            {
                //Reset the height map to a flat surface
                BuildHeightMap(1);
                PerlinNoiseWithOctaves(Amplitude, frequency, octaves);
                NormaliseHeightMap(HeightMapNormaliseAmount);
                GroundModel->ResizeModel(HeightMap, SizeOfTerrainVertices, TerrainMeshMinPt, TerrainMeshMaxPt);
                UpdateFoliagePosition();
            }

            //-------------------------------------------------------------//
            // Generate new Terrain with the Diamond Square Algorithm      //
            //-------------------------------------------------------------//
            //generates new height values with the Diamond Square algorithm
            //then resizes the terrain mesh with these new height values
            //finally updates the positions of the plants in the scene
            ImGui::SameLine();
            if (ImGui::Button("Diamond Square", ButtonSize))
            {
                //Reset the height map to a flat surface
                BuildHeightMap(1);
                DiamondSquareMap();
                GroundModel->ResizeModel(HeightMap, SizeOfTerrainVertices, TerrainMeshMinPt, TerrainMeshMaxPt);
                UpdateFoliagePosition();
            }

            //-------------------------------------------------------------//
            // Update the Terrain with Terraces                            //
            //-------------------------------------------------------------//
            //updates the values of the heightMap to generate terraces in the terrain
            //then resizes the terrain mesh with these new height values
            //finally updates the positions of the plants in the scene
            if (ImGui::Button("Terracing", ButtonSize))
            {
                Terracing(terracingMultiplier);
                GroundModel->ResizeModel(HeightMap, SizeOfTerrainVertices, TerrainMeshMinPt, TerrainMeshMaxPt);
                UpdateFoliagePosition();
            }
            ImGui::Text("");
            ImGui::Separator();

            //----------------------------------------------------------------------//
            // Changing the amount of plants that can be spawned per Terrain chunks //
            //----------------------------------------------------------------------//
            //Sliders to update the variables used for creating new plants
            ImGui::SliderInt("Number of plants per chunk", &plantResizeAmount, 1, 20);

            //Checks if the ResizeAmount is equal to the current size of the plant vector
            //if it is not, resize the plant vector to the new size and add new models for every new index.
            //Finally update the position of every plant in the scene
            if (ImGui::Button("Update Size", ButtonSize))
            {
                if (plantResizeAmount != CurrentPlantVectorSize)
                {
                    CurrentPlantVectorSize = plantResizeAmount;
                    PlantModels.resize(plantResizeAmount);
                    for (int i = 0; i < PlantModels.size(); ++i)
                    {
                        PlantModels[i] = new Model(resourceManager->getMesh(L"plant"));
                        PlantModels[i]->SetPosition(CVector3(0.0f, -1500.0f, 0.0f));
                        PlantModels[i]->SetScale(0.1f);
                    }
                    
                }
                UpdateFoliagePosition();    
            }
        }
        //End of the Terrain Generation window
        ImGui::End();
    }

    //New ImGui window to contain the current scene rendered to a 2DTexture//
    {
        ImGui::Begin("Scene", 0, windowFlags);
        ImGui::Image(SceneTextureSRV, ImVec2((float)textureWidth, (float)textureHeight));
        ImGui::End();
    }

    //End the ImGui window
    ImGui::End();
}