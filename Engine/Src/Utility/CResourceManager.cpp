#include "CResourceManager.h"

CResourceManager::CResourceManager()
{
}

void CResourceManager::loadTexture(const wchar_t* uniqueID, std::string filename)
{
	HRESULT result;

	// Set the texture to the default one if this filename is not valid
	if (!doesFileExist(filename))
	{
		// Set the texture to the default on to make it easier
		// to see which texture did not load correctly
		filename = "../Media/tiles1.jpg";
	}

	std::string dds = ".dds"; // So check the filename extension (case insensitive)
	if (filename.size() >= 4 &&
		std::equal(dds.rbegin(), dds.rend(), filename.rbegin(), [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); }))
	{
		result = DirectX::CreateDDSTextureFromFile(gD3DDevice, CA2CT(filename.c_str()), NULL, &texture);
	}
	else
	{
		result = DirectX::CreateWICTextureFromFile(gD3DDevice, gD3DContext, CA2CT(filename.c_str()), NULL, &texture);
	}

	//Check whether DirectX had an error when creating the texture
	if (FAILED(result))
	{
		MessageBox(NULL, L"Texture loading error", L"ERROR", MB_OK);
	}

	//If no error found then add the texture to the TextureMap paired with the unique ID Created
	else
	{
		textureMap.insert(std::make_pair(const_cast<wchar_t*>(uniqueID), texture));
	}
}

void CResourceManager::loadMesh(const wchar_t* uniqueID, std::string &filename)
{
	// Set the texture to the default one if this filename is not valid
	if (!doesFileExist(filename))
	{
		// Set the texture to the default on to make it easier
		// to see which texture did not load correctly
		filename = "../Src/Data/DefaultDiffuse.png";
	}
	mesh = new Mesh(filename);
	meshMap.insert(std::make_pair(const_cast<wchar_t*>(uniqueID), mesh));
}

void CResourceManager::loadGrid(const wchar_t* uniqueID, CVector3 minPt, CVector3 maxPt, int subDivX, int subDivZ, float** heightMap, bool normals, bool uvs)
{
	mesh = new Mesh(minPt, maxPt, subDivX, subDivZ, heightMap, normals, uvs);
	meshMap.insert(std::make_pair(const_cast<wchar_t*>(uniqueID), mesh));
}

// Release resource.
CResourceManager::~CResourceManager()
{
	if (texture)
	{
		texture->Release();
		texture = 0;
	}
	if (mesh)
	{
		mesh->~Mesh();
		mesh = 0;
	}
}

// Return texture as a shader resource.
ID3D11ShaderResourceView* CResourceManager::getTexture(const wchar_t* uid)
{
	if (textureMap.find(const_cast<wchar_t*>(uid)) != textureMap.end())
	{
		// texture exists
		texture = textureMap.at(const_cast<wchar_t*>(uid));
		return texture;
	}
	else
	{
		return textureMap.at(L"default");
	}
}

Mesh* CResourceManager::getMesh(const wchar_t* uid)
{
	if (meshMap.find(const_cast<wchar_t*>(uid)) != meshMap.end())
	{
		// texture exists
		mesh = meshMap.at(const_cast<wchar_t*>(uid));
		return mesh;
	}
	return nullptr;
}

bool CResourceManager::doesFileExist(std::string &fname)
{
	std::ifstream infile(fname);
	return infile.good();
}