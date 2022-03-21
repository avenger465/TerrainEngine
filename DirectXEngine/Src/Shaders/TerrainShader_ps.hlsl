#include "Common.hlsli"

Texture2D texture0 : register(t0);
Texture2D texture1 : register(t1);
Texture2D texture2 : register(t2);
SamplerState sampler0 : register(s0);

float4 main(LightingPixelShaderInput input) : SV_TARGET
{
    //Sample the Textures
    float4 textureColour;
    float4 grassColour = texture0.Sample(sampler0, input.uv);
    float4 rockColour = texture1.Sample(sampler0, input.uv);
    float4 dirtColour = texture2.Sample(sampler0, input.uv);
    float blendAmount;
    
    //Get the slope of the pixel
    float slope = 1 - input.normal.y;
    //if the slope is less than 0.2f then blend between grass and dirt
    if (slope < 0.2)
    {
        blendAmount = slope / 0.2f;
        textureColour = lerp(grassColour, dirtColour, blendAmount);
    }
    //if the slope is between 0.2 and 0.7 then blend between dirt and rock
    else if ((slope < 0.7f) && (slope >= 0.2f))
    {
        blendAmount = (slope - 0.2f) * (1.0f / (0.7f - 0.2f));

        textureColour = lerp(dirtColour, rockColour, blendAmount);
    }
    //otherwise set the texture to rock
    else
    {
        textureColour = rockColour;
    }
    
    /////////////////////
    
	// Calculate lighting
    // Normal might have been scaled by model scaling or interpolation so renormalise
    input.worldNormal = normalize(input.worldNormal);

    float3 cameraDirection = normalize(gCameraPosition - input.worldPosition);

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