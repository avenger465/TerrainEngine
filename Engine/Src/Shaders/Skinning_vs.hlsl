//--------------------------------------------------------------------------------------
// Skinning Vertex Shader
//--------------------------------------------------------------------------------------
// Same as per-pixel lighting vertex shader, but each vertex is influenced by the world matrices of four nearby bones
// rather than just having a single world matrix

#include "Common.hlsli" // Shaders can also use include files - note the extension


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Vertex shader gets vertices from the mesh one at a time. It transforms their positions
// from 3D into 2D (see lectures) and passes that position down the pipeline so pixels can
// be rendered. For skinning the transformation includes blending the world matrices of 4 nearby bonees.
LightingPixelShaderInput main(SkinningVertex modelVertex)
{
    LightingPixelShaderInput output; // This is the data the pixel shader requires from this vertex shader

    // Add 4th element to vertex position and normal (1 for positions, 0 for vectors)
    float4 modelPosition = float4(modelVertex.position, 1); 
    float4 modelNormal   = float4(modelVertex.normal,   0);

    float4 worldPosition;
	worldPosition  = mul( gBoneMatrices[modelVertex.bones[0]], modelPosition ) * modelVertex.weights[0];
                     //*** MISSING - The above line multiplies the vertex position by the first bone's matrix, using the weight of the first bone
	                 //***           Now you need to do the same for each bone, adding all the results together to get a weighted average for this vertex (as discussed in lecture)
	
    float4 worldNormal;
    worldNormal = mul(gBoneMatrices[modelVertex.bones[0]], modelNormal) * modelVertex.weights[0];
     //*** ` - Do the same process as above to calculate the world normal using the influencing bones

    // Use the view matrix to transform the final vertex position from world space into view space (camera's point of view)
    // and then use the projection matrix to transform the vertex to 2D projection space (project onto the 2D screen)
    float4 viewPosition      = mul(gViewMatrix,       worldPosition);
    output.projectedPosition = mul(gProjectionMatrix, viewPosition);

    // Pass world position and normal to pixel shader for lighting
    output.worldPosition = worldPosition.xyz;
    output.worldNormal   = worldNormal.xyz;

    // Pass texture coordinates (UVs) on to the pixel shader, the vertex shader doesn't need them
    output.uv = modelVertex.uv;

    return output; // Ouput data sent down the pipeline (to the pixel shader)
}
