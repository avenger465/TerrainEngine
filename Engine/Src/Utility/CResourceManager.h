#pragma once
#include "tepch.h"
#include "GraphicsHelpers.h"
#include "project/Mesh.h"
#include <WICTextureLoader.h>
#include <DDSTextureLoader.h>

class CResourceManager
{
public:
	CResourceManager();
	~CResourceManager();

	void loadTexture(const wchar_t* uniqueID, std::string filename);
	void loadMesh(const wchar_t* uniqueID, std::string &filename);
	void CResourceManager::loadGrid(const wchar_t* uniqueID, CVector3 minPt, CVector3 maxPt, int subDivX, int subDivZ, std::array<std::array<float, resolution>, resolution> &heightMap, bool normals, bool uvs);


	ID3D11ShaderResourceView* getTexture(const wchar_t* uid);
	Mesh* getMesh(const wchar_t* uid);

private:
	bool doesFileExist(std::string &fileName);

	ID3D11ShaderResourceView* texture;
	Mesh* mesh;

	std::map<wchar_t*, ID3D11ShaderResourceView*> textureMap;
	std::map<wchar_t*, Mesh*> meshMap;
};