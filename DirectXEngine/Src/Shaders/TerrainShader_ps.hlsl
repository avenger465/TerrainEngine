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

    float4 white = float4(0.871f, 0.965f, 0.922f, 1.0f);
    
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

    //if (all(voronoiDiagram >= (white - 0.02f)) && all(voronoiDiagram <= (white + 0.02f)))
    //{
    //    textureColour.rgb = float3(0.0f, 0.0f, 1.0f);
    //}
    //else
    //{
    //    textureColour.rgb = float3(1.0f, 0.0f, 0.0f);
    //}
    
    /////////////////////
    
	// Calculate lighting
    // Normal might have been scaled by model scaling or interpolation so renormalise
    input.worldNormal = normalize(input.worldNormal);

    //colour from the texture 
    float3 diffuseMaterialColour = textureColour.rgb;
    
    //float3 finalColour = diffuseLight * diffuseMaterialColour;
    float3 finalColour = diffuseMaterialColour;

    return float4(finalColour, 1.0f);
}