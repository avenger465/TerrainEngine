//--------------------------------------------------------------------------------------
// Light Model Vertex Shader
//--------------------------------------------------------------------------------------
// Transform model into world space only - used for when geometry shader will do the 
// final transform into 2D

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Vertex shader gets vertices from the mesh one at a time. It transforms their positions
// from model space into world space only
BasicVertex main(BasicVertex modelVertex)
{
    BasicVertex output;

	// Input position is x,y,z only. add a 4th element 1 for a point (0 for a vector) - recall lectures
    float4 modelPosition = float4(modelVertex.position, 1);

	// Multiply by the world matrix passed from C++ to transform the model vertex position into world space. 
    output.position = mul(gWorldMatrix, modelPosition).xyz;

	// Same process to transfrom model normals into world space
    float4 modelNormal = float4(modelVertex.normal, 0);
    output.normal = mul(gWorldMatrix, modelNormal).xyz;
    output.normal = normalize(output.normal);

	// Pass texture coordinates (UVs) on to the later shaders unchanged
    output.uv = modelVertex.uv;

    return output; // Ouput data sent down the pipeline (to the pixel shader)
}