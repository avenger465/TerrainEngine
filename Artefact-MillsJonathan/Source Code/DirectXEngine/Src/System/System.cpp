#include "System.h"

//Constructor to initialise everything in the system and scene
System::System(BaseScene* scene, HINSTANCE hInstance, int nCmdShow, int screenWidth, int screenHeight, bool VSYNC, bool FULL_SCREEN)
{
    //Set the width and height of the viewport (Window)
    viewportWidth = screenWidth;
    viewportHeight = screenHeight;

    // Create a window to display the scene
    if (!InitWindow(hInstance, nCmdShow)) { exit(0); }

    // Prepare the user input functions
    InitInput();

    // Initialise Direct3D
    if (!InitDirect3D(viewportWidth, viewportHeight, HWnd, LastError))
    {
        MessageBoxA(HWnd, LastError.c_str(), NULL, MB_OK);
        exit(0);
    }
       
    //Update the Systems Scene pointer to the current scene 
    //Allowing for the system to load the scene
    Scene = scene;

    //------------------//
    // Initialise ImGui //
    //------------------//

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; //Enable docking of the viewports 
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; //Enable each window to be a seperate viewport

    // Setup ImGui style
    SetupIMGUIiStyle(0.5f);

    // Setup Platform/Renderer bindings
    ImGui_ImplWin32_Init(HWnd);
    ImGui_ImplDX11_Init(gD3DDevice, gD3DContext);

    // Initialise scene
    // If scene cannot be initialised then release the resources from system memory
    if (!Scene->InitGeometry(LastError) || !Scene->InitScene())
    {
        MessageBoxA(HWnd, LastError.c_str(), NULL, MB_OK);
        Scene->ReleaseResources();
        ShutdownDirect3D();
        exit(0);
    }
}

//Class deconstructor 
System::~System()
{
    //Check if Scene exists and then delete the current scene
    if (Scene)
    {
        delete Scene;
        Scene = 0;
    }
}

//Function that is called every frame to run the scene
void System::run()
{
    //Use a Timer to get the frameTime
    gTimer.Start();


    // Main message loop
    MSG msg = {};
    while (msg.message != WM_QUIT) // As long as window is open
    {
        // Check for and deal with any window messages (input, window resizing, minimizing, etc.).
        // The actual message processing happens in the function WndProc below
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            // Deal with messages
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        else // When no windows messages left to process then render & update our scene
        {
            // Update the scene by the amount of time since the last frame
            float frameTime = gTimer.GetLapTime();
            Scene->UpdateScene(frameTime, HWnd);

            // Render the scene
            Scene->RenderScene(frameTime);


            if (KeyHit(Key_Escape))
            {
                DestroyWindow(HWnd); // This will close the window and ultimately exit this loop
            }
        }
    }

    //IMGUI
    //*******************************
    // Shutdown ImGui
    //*******************************

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    //*******************************

    // Release everything from memory before quitting
    Scene->ReleaseResources();
    ShutdownDirect3D();
}

//Functions to handle user input in the window
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT System::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) // IMGUI this line passes user input to ImGUI
        return true;

    switch (message)
    {
    case WM_PAINT: // A necessary message to ensure the window content is displayed
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY: // Another necessary message to deal with the window being closed
        PostQuitMessage(0);
        break;

    // The WM_KEYXXXX messages report keyboard input to our window.
    // This application has added some simple functions (not DirectX) to process these messages (all in Input.cpp/h)
    // so you don't need to change this code. Instead simply use KeyHit, KeyHeld etc.
    case WM_KEYDOWN:
        KeyDownEvent(static_cast<KeyCode>(wParam));
        break;

    case WM_KEYUP:
        KeyUpEvent(static_cast<KeyCode>(wParam));
        break;


    // The following WM_XXXX messages report mouse movement and button presses
    // Use KeyHit to get mouse buttons, GetMouseX, GetMouseY for its position
    case WM_MOUSEMOVE:
    {
        MouseMoveEvent(LOWORD(lParam), HIWORD(lParam));
        break;
    }
    case WM_LBUTTONDOWN:
    {
        KeyDownEvent(Mouse_LButton);
        break;
    }
    case WM_LBUTTONUP:
    {
        KeyUpEvent(Mouse_LButton);
        break;
    }
    case WM_RBUTTONDOWN:
    {
        KeyDownEvent(Mouse_RButton);
        break;
    }
    case WM_RBUTTONUP:
    {
        KeyUpEvent(Mouse_RButton);
        break;
    }
    case WM_MBUTTONDOWN:
    {
        KeyDownEvent(Mouse_MButton);
        break;
    }
    case WM_MBUTTONUP:
    {
        KeyUpEvent(Mouse_MButton);
        break;
    }


    // Any messages we don't handle are passed back to Windows default handling
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

//Initialise the window that will appear on the screen
BOOL System::InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // Get a stock icon to show on the taskbar for this program.
    SHSTOCKICONINFO stockIcon{};
    stockIcon.cbSize = sizeof(stockIcon);
    if (SHGetStockIconInfo(SIID_APPLICATION, SHGSI_ICON, &stockIcon) != S_OK) // Returns false on failure
    {
        return false;
    }

    // Register window class. Defines various UI features of the window for our application.
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;    // Which function deals with windows messages
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0; SIID_APPLICATION;
    wcex.hInstance = hInstance;
    wcex.hIcon = stockIcon.hIcon; // Which icon to use for the window
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW); // What cursor appears over the window
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"Window";
    wcex.hIconSm = stockIcon.hIcon;
    if (!RegisterClassEx(&wcex)) // Returns false on failure
    {
        return false;
    }


    // Select the type of window to show our application in
    DWORD windowStyle = WS_OVERLAPPEDWINDOW; // Standard window

    // Calculate overall dimensions for the window. We will render to the *inside* of the window. But the
    // overall winder will be larger because it includes the borders, title bar etc. This code calculates
    // the overall size of the window given our choice of viewport size.
    RECT rc = { 0, 0, viewportWidth, viewportHeight };
    AdjustWindowRect(&rc, windowStyle, FALSE);

    // Create window, the second parameter is the text that appears in the title bar
    HWnd = CreateWindow(L"Window", L"Procedural Terrain Generation", windowStyle,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
    if (!HWnd)
    {
        return false;
    }

    ShowWindow(HWnd, nCmdShow);
    UpdateWindow(HWnd);

    return TRUE;
}

//Update the Colour Scheme (Style) of ImGui to a black Background and Golden highlights
void System::SetupIMGUIiStyle(float alpha)
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.FrameRounding = 15.0f;
    style.Colors[ImGuiCol_Text]               = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]       = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);

    style.Colors[ImGuiCol_WindowBg]           = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
    style.Colors[ImGuiCol_PopupBg]            = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);

    style.Colors[ImGuiCol_Border]             = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
    style.Colors[ImGuiCol_BorderShadow]       = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);

    style.Colors[ImGuiCol_FrameBg]            = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
    style.Colors[ImGuiCol_FrameBgHovered]     = ImVec4(1.0f, 1.0f, 1.0f, 0.4f);
    style.Colors[ImGuiCol_FrameBgActive]      = ImVec4(1.0f, 1.0f, 1.0f, 0.4f);

    style.Colors[ImGuiCol_TitleBg]            = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]   = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
    style.Colors[ImGuiCol_TitleBgActive]      = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);

    style.Colors[ImGuiCol_MenuBarBg]          = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);

    style.Colors[ImGuiCol_CheckMark]          = ImVec4(1.0f, 0.8f, 0.4f, 1.0f);
    style.Colors[ImGuiCol_DockingPreview]     = ImVec4(1.0f, 0.8f, 0.4f, 0.8f);

    style.Colors[ImGuiCol_SliderGrab]         = ImVec4(1.0f, 0.8f, 0.4f, 0.75f);
    style.Colors[ImGuiCol_SliderGrabActive]   = ImVec4(1.0f, 0.8f, 0.4f, 1.00f);

    style.Colors[ImGuiCol_Button]             = ImVec4(1.0f, 0.8f, 0.4f, 0.60f);
    style.Colors[ImGuiCol_ButtonHovered]      = ImVec4(1.0f, 0.8f, 0.4f, 0.85f);
    style.Colors[ImGuiCol_ButtonActive]       =  ImVec4(1.0f, 0.8f, 0.4f, 1.0f);

    style.Colors[ImGuiCol_ResizeGrip]         = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
    style.Colors[ImGuiCol_ResizeGripHovered]  = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive]   = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);

    style.Colors[ImGuiCol_Tab]                = ImVec4(1.0f, 0.8f, 0.4f, 0.05f);
    style.Colors[ImGuiCol_TabHovered]         = ImVec4(1.0f, 0.8f, 0.4f, 0.60f);
    style.Colors[ImGuiCol_TabActive]          = ImVec4(1.0f, 0.8f, 0.4f, .85f);
    style.Colors[ImGuiCol_TabUnfocused]       = ImVec4(1.0f, 0.8f, 0.4f, 0.20f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(1.0f, 0.8f, 0.4f, 0.60f);

    //Go through each 
    for (int i = 0; i <= ImGuiCol_COUNT; i++)
    {
        ImVec4& col = style.Colors[i];
        float H, S, V;
        ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);

        if (S < 0.1f)
        {
            V = 1.0f - V;
        }
        ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
        if (col.w < 1.00f)
        {
            col.w *= alpha;
        }
    }

}