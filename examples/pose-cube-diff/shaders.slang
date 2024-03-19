// shaders.slang

//
// This file provides a simple vertex and fragment shader that can be compiled
// using Slang. This code should also be valid as HLSL, and thus it does not
// use any of the new language features supported by Slang.
//

// Uniform data to be passed from application -> shader.
cbuffer Uniforms
{
    float4x4 modelViewProjection;
    SamplerState sampler;
}

// Per-vertex attributes to be assembled from bound vertex buffers.
struct AssembledVertex
{
    float3 position : POSITION;
    float4 color : COLOR; 
};

// Output of the vertex shader, and input to the fragment shader.
struct CoarseVertex
{
    float2 uv;
    float4 color;
};

// Output of the fragment shader
struct Fragment
{
    float4 color;
};

// Vertex  Shader
static float4 sample(Texture2D t, SamplerState s, float2 uv)
{
    return t.Sample(s, uv);
}
struct VertexStageOutput
{
    CoarseVertex    coarseVertex    : CoarseVertex;
    float4          sv_position     : SV_Position;
};

[shader("vertex")]
VertexStageOutput vertexMain(
    AssembledVertex assembledVertex)
{
    VertexStageOutput output;

    float3 position = assembledVertex.position;
    float4 color = assembledVertex.color;

    output.coarseVertex.color = color;
    output.sv_position = mul(modelViewProjection, float4(position, 1.0));

    return output;
}

// Fragment Shader

[shader("fragment")]
float4 fragmentMain(
    CoarseVertex coarseVertex : CoarseVertex) : SV_Target
{
    return coarseVertex.color;
   
}
