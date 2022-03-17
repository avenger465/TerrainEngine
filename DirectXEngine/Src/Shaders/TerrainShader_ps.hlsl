#include "Common.hlsli"

Texture2D texture0 : register(t0);
Texture2D texture1 : register(t1);
Texture2D texture2 : register(t2);
Texture2D texture3 : register(t3);
SamplerState sampler0 : register(s0);

float4 main(LightingPixelShaderInput input) : SV_TARGET
{
    float4 textureColour;
    float4 grassColour = texture0.Sample(sampler0, input.uv);
    float4 rockColour = texture1.Sample(sampler0, input.uv);
    float4 dirtColour = texture2.Sample(sampler0, input.uv);
    float4 voronoiDiagram = texture3.Sample(sampler0, input.uv);
    float blendAmount;
    
    float slope = 1 - input.normal.y;
    if (slope < 0.2)
    {
        blendAmount = slope / 0.2f;
        textureColour = lerp(grassColour, dirtColour, blendAmount);
    }
    else if ((slope < 0.7f) && (slope >= 0.2f))
    {
        blendAmount = (slope - 0.2f) * (1.0f / (0.7f - 0.2f));

        textureColour = lerp(dirtColour, rockColour, blendAmount);
    }
    else
    {
        textureColour = rockColour;
    }
    
    /////////////////////
    
	// Calculate lighting
    // Normal might have been scaled by model scaling or interpolation so renormalise
    input.worldNormal = normalize(input.worldNormal);

    float3 cameraDirection = normalize(gCameraPosition - input.worldPosition);

    	//// Light 1 ////

	// Direction and distance from pixel to light
    float3 lightDirection = normalize(gLightPosition - input.worldPosition);
    float3 lightDist = length(gLightPosition - input.worldPosition);
    
    // Equations from lighting lecture
    float3 diffuseLight = gAmbientColour + (gLightColour * max(dot(input.worldNormal, lightDirection), 0) / lightDist);
    //colour from the texture 

    float3 diffuseMaterialColour = textureColour.rgb;
    
    //float3 finalColour = diffuseLight * diffuseMaterialColour;
    float3 finalColour = diffuseLight *  diffuseMaterialColour;

    return float4(finalColour, textureColour.a);
}