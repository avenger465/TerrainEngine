#pragma once
#include "tepch.h"
#include "Direct3DSetup.h"

#include "Utility/Input.h"
#include "Utility/Timer.h"
#include "project/Common.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "BasicScene/BaseScene.h"

class System
{
//----------------------//
// Construction / Usage	//
//----------------------//

public:
	//Constructor to initialise everything in the system and scene
	System(BaseScene* scene, HINSTANCE hInstance, int nCmdShow,int screenWidth, int screenHeight, bool VSYNC, bool FULL_SCREEN);
	
	//Class deconstructor 
	~System();

	//Function that is called every frame to run the scene
	void run();
	
	//Function to deal with the messages that pop up during the running of the program
	static LRESULT CALLBACK WndProc(HWND , UINT , WPARAM , LPARAM);

	//Updates the style of the ImGui widgets 
	void SetupIMGUIiStyle(float alpha);

//-------------//
// Member data //
//-------------//

private:
	//Initialise the window that will appear on the screen
	BOOL InitWindow(HINSTANCE, int);

	HWND HWnd;
	BaseScene* Scene;
	Timer gTimer;
	std::string LastError;

	//Width and Height of the window viewport
	int viewportWidth;
	int viewportHeight;
};

