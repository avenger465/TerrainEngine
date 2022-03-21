//--------------------------------------------------------------------------------------
// Entry point for the application
// Window creation code
//--------------------------------------------------------------------------------------

#include "System/System.h"
#include "TerrainGenerationScene/TerrainGenerationScene.h"

//--------------------------------------------------------------------------------------
// The entry function for a Windows application is called wWinMain
//--------------------------------------------------------------------------------------
int APIENTRY wWinMain(_In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPWSTR    lpCmdLine,
    _In_     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    //Create a new TerrainScene object 
    TerrainGenerationScene* TerrainScene = new TerrainGenerationScene();

    //Create a new System object with the scene to load and the Height and Width of the window
    System* system = new System(TerrainScene, hInstance, nCmdShow, 1268, 960, false, false);

    //Run the System
    system->run();

    //At the end release the system object from memory
    delete system;
    system = 0;
    return 0;
}