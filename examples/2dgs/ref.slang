// shaders.slang

//
// This file provides a simple vertex and fragment shader that can be compiled
// using Slang. This code should also be valid as HLSL, and thus it does not
// use any of the new language features supported by Slang.
//


cbuffer Uniforms
{
    float4x4 modelViewProjection;
    SamplerState sampler;
    Texture2D texRef;
    int x;
    int y;
    int width;
    int height;
    int viewWidth;
    int viewHeight;
  
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
};

// Output of the fragment shader
struct Fragment
{
    float4 color;
};



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

    output.coarseVertex.uv = position.xy;

    output.sv_position.x = ((x + position.x * width) / (float)viewWidth) * 2.0f - 1.0f;
    output.sv_position.y = -((y + position.y * height) / (float)viewHeight * 2.0f - 1.0f);
    output.sv_position.z = 0;
    output.sv_position.w = 1.0;

    return output;
}

// Fragment Shader

[shader("fragment")]
float4 fragmentMain(
    CoarseVertex coarseVertex: CoarseVertex) : SV_Target
{
    return float4(texRef.Sample(sampler, coarseVertex.uv).xyz +float3(0.05), 1.0);
}


