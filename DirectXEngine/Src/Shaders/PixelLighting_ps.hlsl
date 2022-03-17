//--------------------------------------------------------------------------------------
// Per-Pixel Lighting Pixel Shader
//--------------------------------------------------------------------------------------
// Pixel shader receives position and normal from the vertex shader and uses them to calculate
// lighting per pixel. Also samples a samples a diffuse + specular texture map and combines with light colour.

#include "Common.hlsli" // Shaders can also use include files - note the extension


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// Here we allow the shader access to a texture that has been loaded from the C++ side and stored in GPU memory.
// Note that textures are often called maps (because texture mapping describes wrapping a texture round a mesh).
// Get used to people using the word "texture" and "map" interchangably.
Texture2D    DiffuseSpecularMap : register(t0); // Textures here can contain a diffuse map (main colour) in their rgb channels and a specular map (shininess) in the a channel
SamplerState TexSampler         : register(s0); // A sampler is a filter for a texture like bilinear, trilinear or anisotropic - this is the sampler used for the texture above


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------
    
// Pixel shader entry point - each shader has a "main" function
// This shader just samples a diffuse texture map
float4 main(LightingPixelShaderInput input) : SV_Target
{
    // Normal might have been scaled by model scaling or interpolation so renormalise
    input.worldNormal = normalize(input.worldNormal); 

	///////////////////////
	// Calculate lighting
    
    // Direction from pixel to camera
    float3 cameraDirection = normalize(gCameraPosition - input.worldPosition);

	//// Light 1 ////

	// Direction and distance from pixel to light
	float3 lightDirection = normalize(gLightPosition - input.worldPosition);
    float3 lightDist = length(gLightPosition - input.worldPosition);
    
    // Equations from lighting lecture
    float3 diffuseLight = gAmbientColour + (gLightColour * max(dot(input.worldNormal, lightDirection), 0) / lightDist);

	////////////////////
	// Combine lighting and textures

    // Sample diffuse material and specular material colour for this pixel from a texture using a given sampler that you set up in the C++ code
    float4 textureColour = DiffuseSpecularMap.Sample(TexSampler, input.uv);
    float3 diffuseMaterialColour = textureColour.rgb; // Diffuse material colour in texture RGB (base colour of model)
    float specularMaterialColour = textureColour.a;   // Specular material colour in texture A (shininess of the surface)
    
    // Combine lighting with texture colours
    //float3 finalColour = diffuseLight * diffuseMaterialColour + specularLight * specularMaterialColour;
    float3 finalColour = diffuseMaterialColour;

    return float4(finalColour, 1.0f); // Always use 1.0f for output alpha - no alpha blending in this lab
}