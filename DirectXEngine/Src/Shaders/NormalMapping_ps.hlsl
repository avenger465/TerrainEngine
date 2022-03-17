#include "Common.hlsli"

Texture2D DiffuseSpecularMap : register(t0);
SamplerState TexSampler : register(s0);

Texture2D NormalMap : register(t1);

float4 main(NormalMappingPixelShaderInput input) : SV_TARGET
{
    float3 modelNormal = normalize(input.modelNormal);
    float3 modelTangent = normalize(input.modelTangent);

    float3 modelBiTangent = cross(modelNormal, modelTangent);
    float3x3 invTangentMatrix = float3x3(modelTangent, modelBiTangent, modelNormal);

    float3 textureNormal = (2.0f * NormalMap.Sample(TexSampler, input.uv).rgb - 1.0f) * 1000;

    float3 worldNormal = normalize(mul((float3x3) gWorldMatrix, mul(textureNormal, invTangentMatrix)));

    ////////////////////////
	// Calculate lighting //

    // Direction from pixel to camera
    float3 cameraDirection = normalize(gCameraPosition - input.worldPosition);

	// Direction and distance from pixel to light
    float3 lightDirection = normalize(gLightPosition - input.worldPosition);
    float3 lightDist = length(gLightPosition - input.worldPosition);
    
    // Equations from lighting lecture
    float3 diffuseLight = gAmbientColour + (gLightColour * max(dot(worldNormal, lightDirection), 0) / lightDist);

    float4 textureColour = DiffuseSpecularMap.Sample(TexSampler, input.uv);
    float3 diffuseMaterialColour = textureColour.rgb;

    float3 finalColour = diffuseLight * diffuseMaterialColour;

    if (textureColour.a < 0.1f)
    {
        discard;
    }

    return float4(finalColour, 1.0f);
}