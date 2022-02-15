
#include "Data/Mesh.h"
#include "Data/Model.h"
#include "project/Common.h"

#pragma once

class CLight
{
public:
	CLight(Mesh* Mesh, float Strength, CVector3 Colour, CVector3 Position, float Scale);
	~CLight();

public:
	Model* LightModel;
	float LightStrength;
	CVector3 LightColour;

	void SetPosition(CVector3 Position);
	CVector3 GetLightColour();
	void SetLightStates(ID3D11BlendState* blendSate, ID3D11DepthStencilState* depthState, ID3D11RasterizerState* rasterizerState);
	void RenderLight(ID3D11Buffer* buffer, PerModelConstants& ModelConstants);
};

