#include "Common.hlsli"

Texture2D texture0 : register(t0);
Texture2D texture1 : register(t1);
Texture2D texture2 : register(t2);
SamplerState sampler0 : register(s0);

float4 main(LightingPixelShaderInput input) : SV_TARGET
{
    float4 textureColour;
    float4 grassColour;
    float4 rockColour;
    float4 dirtColour;
    float blendAmount;
    
    grassColour = texture0.Sample(sampler0, input.uv);
    rockColour = texture1.Sample(sampler0, input.uv);
    dirtColour = texture2.Sample(sampler0, input.uv);
    
    float slope = 1- input.normal.y;
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

    ////---------////
	//// Light 1 ////
    ////---------////
	// Direction and distance from pixel to light
    float3 light1Direction = normalize(gLight1Position - input.worldPosition);
    float3 light1Dist = length(gLight1Position - input.worldPosition);
    float3 diffuseLight1 = gLight1Colour * max(dot(input.worldNormal, light1Direction), 0) / light1Dist;

    ////---------////
	//// Light 2 ////
    ////---------////
    float3 light2Direction = normalize(gLight2Position - input.worldPosition);
    float3 light2Dist = length(gLight2Position - input.worldPosition);
    float3 diffuseLight2 = gLight2Colour * max(dot(input.worldNormal, light2Direction), 0) / light2Dist;

    //colour from the texture 
    float3 diffuseMaterialColour = textureColour.rgb;
    
	// Sum the effect of the lights - add the ambient at this stage rather than for each light (or we will get too much ambient)
    float3 diffuseLight = gAmbientColour + diffuseLight1 + diffuseLight2;
    
    float3 finalColour = diffuseLight * diffuseMaterialColour;
    
    
    if (!gEnableLights)
    {
        finalColour = diffuseMaterialColour;
    }
    return float4(finalColour, 1.0f);
}