#include "Common.hlsli"


NormalMappingPixelShaderInput main(TangentVertex modelVertex)
{
    NormalMappingPixelShaderInput output;

    float4 modelPosition = float4(modelVertex.position, 1);

    float4 worldPosition = mul(gWorldMatrix, modelPosition);
    float4 viewPosition = mul(gViewMatrix, worldPosition);
    output.projectedPosition = mul(gProjectionMatrix, viewPosition);

    output.worldPosition = worldPosition.xyz;

    output.modelNormal = modelVertex.normal;
    output.modelTangent = modelVertex.tangent;

    output.uv = modelVertex.uv;

    return output;
}