//--------------------------------------------------------------------------------------
// Entry point for the application
// Window creation code
//--------------------------------------------------------------------------------------

#include "tepch.h"
#include "System/System.h"
#include "Scene/TerrainGenerationScene.h"

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

    TerrainGenerationScene* Scene = new TerrainGenerationScene();
    System* system;

    system = new System(Scene, hInstance, nCmdShow, 1268, 960, false, false);

    system->run();

    delete system;

    system = 0;

    return 0;
}