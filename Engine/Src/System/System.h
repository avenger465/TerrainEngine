#pragma once
#include "tepch.h"
#include "project/Scene.h"
#include "Direct3DSetup.h"

#include "Utility/Input.h"
#include "Utility/Timer.h"
#include "project/Common.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

class System
{
public:
	System(HINSTANCE hInstance, int nCmdShow,int screenWidth, int screenHeight, bool VSYNC, bool FULL_SCREEN);
	~System();

	void run();

	static LRESULT CALLBACK WndProc(HWND , UINT , WPARAM , LPARAM);

private:

	BOOL InitWindow(HINSTANCE, int);
	HINSTANCE HInst;
	HWND HWnd;
	Timer gTimer;
	std::string LastError;
	int viewportWidth;
	int viewportHeight;
};

