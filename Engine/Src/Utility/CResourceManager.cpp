#include "CResourceManager.h"

//Attempt to load texture. If load fails use default texture.
//Based on extension, uses slightly different loading function for different image types .dds vs .png/.jpg.
CResourceManager::CResourceManager(ID3D11Device* ldevice, ID3D11DeviceContext* ldeviceContext)
{
	device = ldevice;
	deviceContext = ldeviceContext;
}

void CResourceManager::loadTexture(const wchar_t* uniqueID, const wchar_t* filename)
{
	HRESULT result;

	// Check whether the filename that was given is a valid path
	if (!filename)
	{
		filename = L"../Media/tiles1.jpg";
	}

	// Set the texture to the default one if this filename is not valid
	if (!fileExists(filename))
	{
		// Set the texture to the default on to make it easier
		// to see which texture did not load correctly
		filename = L"../Media/tiles1.jpg";
	}

	// check file extension for correct loading function.
	std::wstring localFileName(filename);
	std::string::size_type index;
	std::wstring extension;

	index = localFileName.rfind('.');

	//Check whether the file does have an extension attached
	if (index != std::string::npos)
	{
		extension = localFileName.substr(index + 1);

	}

	// Load the texture in.
	if (extension == L"dds")
	{
		result = DirectX::CreateDDSTextureFromFile(device, deviceContext, filename, NULL, &texture);// , 0, NULL);
	}

	else
	{
		result = DirectX::CreateWICTextureFromFile(device, deviceContext, filename, NULL, &texture, 0);
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
	// Check whether the filename that was given is a valid path

	// Set the texture to the default one if this filename is not valid
	if (!fileExists(filename))
	{
		// Set the texture to the default on to make it easier
		// to see which texture did not load correctly
		filename = "../Src/Data/DefaultDiffuse.png";
	}
	mesh = new Mesh(filename);
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
}

bool CResourceManager::fileExists(const wchar_t* fname)
{
	std::ifstream infile(fname);
	return infile.good();
}

bool CResourceManager::fileExists(std::string &fname)
{
	std::ifstream infile(fname);
	return infile.good();
}