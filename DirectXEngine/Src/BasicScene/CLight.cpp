#include "CLight.h"

//Setup the light using the model class 
CLight::CLight(Mesh* Mesh, float Strength, CVector3 Colour, CVector3 Position, float Scale)
{

	LightModel = new Model(Mesh);
	LightStrength = Strength;
	LightColour = Colour;
	LightModel->SetPosition(Position);
	LightModel->SetScale(Scale);

}

//Destructor 
CLight::~CLight()
{}

//Set the Lights position by using the model class function
void CLight::SetPosition(CVector3 Position)
{
	LightModel->SetPosition(Position);
}

//Returns the lights colour
CVector3 CLight::GetLightColour()
{
	return LightColour;
}

//Set the lights states to be used when rendering 
void CLight::SetLightStates(ID3D11BlendState* blendSate, ID3D11DepthStencilState* depthState, ID3D11RasterizerState* rasterizerState)
{
	gD3DContext->OMSetBlendState(blendSate, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(depthState, 0);
	gD3DContext->RSSetState(rasterizerState);
}

//Call the models render function
void CLight::RenderLight(ID3D11Buffer* buffer, PerModelConstants& ModelConstants)
{
	LightModel->Render(buffer, ModelConstants);
}