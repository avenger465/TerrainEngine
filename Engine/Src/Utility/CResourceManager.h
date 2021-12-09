#pragma once
#include "tepch.h"
#include "GraphicsHelpers.h"
#include "project/Mesh.h"

#include <WICTextureLoader.h>
#include <DDSTextureLoader.h>

class CResourceManager
{
public:
	CResourceManager(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
	~CResourceManager();

	void loadTexture(const wchar_t* uniqueID, const wchar_t* filename);
	void loadMesh(const wchar_t* uniqueID, std::string &filename);
	ID3D11ShaderResourceView* getTexture(const wchar_t* uid);
	Mesh* getMesh(const wchar_t* uid);

private:
	bool fileExists(const wchar_t* fileName);
	bool fileExists(std::string &fileName);

	ID3D11ShaderResourceView* texture;
	Mesh* mesh;


	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;

	std::map<wchar_t*, ID3D11ShaderResourceView*> textureMap;
	std::map<wchar_t*, Mesh*> meshMap;
	ID3D11Texture2D* pTexture;
};

