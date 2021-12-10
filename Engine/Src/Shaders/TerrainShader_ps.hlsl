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

    
    float slope = 1.0f - input.normal.y;
    if (slope < 0.2) //check if the slope value is less than 0.2 (flat)
    {
        blendAmount = slope / 0.2f; //calculate the blend amount
        textureColour = lerp(grassColour, dirtColour, blendAmount);
    }
    else if ((slope < 0.7f) && (slope >= 0.2f)) //check if the slo[e value is between 0.2 and 0.7 (average)
    {
        blendAmount = (slope - 0.2f) * (1.0f / (0.7f - 0.2f));

        textureColour = lerp(dirtColour, rockColour, blendAmount);
    }
    else
    {
        textureColour = rockColour;
    }
    
    // Normal might have been scaled by model scaling or interpolation so renormalise
    input.worldNormal = normalize(input.worldNormal);

	///////////////////////
	// Calculate lighting
    
    // Direction from pixel to camera
    float3 cameraDirection = normalize(gCameraPosition - input.worldPosition);

	//// Light 1 ////

	// Direction and distance from pixel to light
    float3 light1Direction = normalize(gLight1Position - input.worldPosition);
    float3 light1Dist = length(gLight1Position - input.worldPosition);
    
    // Equations from lighting lecture
    float3 diffuseLight1 = gLight1Colour * max(dot(input.worldNormal, light1Direction), 0) / light1Dist;
    float3 halfway = normalize(light1Direction + cameraDirection);
    float3 specularLight1 = diffuseLight1 * pow(max(dot(input.worldNormal, halfway), 0), gSpecularPower); // Multiplying by diffuseLight instead of light colour - my own personal preference


	//// Light 2 ////

    float3 light2Direction = normalize(gLight2Position - input.worldPosition);
    float3 light2Dist = length(gLight2Position - input.worldPosition);
    float3 diffuseLight2 = gLight2Colour * max(dot(input.worldNormal, light2Direction), 0) / light2Dist;
    halfway = normalize(light2Direction + cameraDirection);
    float3 specularLight2 = diffuseLight2 * pow(max(dot(input.worldNormal, halfway), 0), gSpecularPower);

    float3 diffuseMaterialColour = textureColour.rgb;
    float specularMaterialColour = textureColour.a;
	// Sum the effect of the lights - add the ambient at this stage rather than for each light (or we will get too much ambient)
    float3 diffuseLight = gAmbientColour + diffuseLight1 + diffuseLight2;
    float3 specularLight = specularLight1 + specularLight2;
    
    float3 finalColour = diffuseLight * diffuseMaterialColour + specularLight * specularMaterialColour;
    
	return float4(finalColour, 1.0f);
}