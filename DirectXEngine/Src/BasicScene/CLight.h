#pragma once
#include "Data/Mesh.h"
#include "Data/Model.h"
#include "project/Common.h"

class CLight
{
//----------------------//
// Construction / Usage	//
//----------------------//
public:
	//Setup the light using the model class 
	CLight(Mesh* Mesh, float Strength, CVector3 Colour, CVector3 Position, float Scale);
	
	//Destructor 
	~CLight();

	//Set the Lights position by using the model class function
	void SetPosition(CVector3 Position);

	//Returns the lights colour
	CVector3 GetLightColour();

	//Set the lights states to be used when rendering 
	void SetLightStates(ID3D11BlendState* blendSate, ID3D11DepthStencilState* depthState, ID3D11RasterizerState* rasterizerState);

	//Call the models render function
	void RenderLight(ID3D11Buffer* buffer, PerModelConstants& ModelConstants);

//-------------//
// Member data //
//-------------//
public:
	Model* LightModel;
	float LightStrength;
	CVector3 LightColour;
};