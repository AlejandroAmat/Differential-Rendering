// shaders.slang

//
// This file provides a simple vertex and fragment shader that can be compiled
// using Slang. This code should also be valid as HLSL, and thus it does not
// use any of the new language features supported by Slang.
//

static const float kSkipDistance = 0.1;

struct Blob  {
    float2 pos;
    float2 scale;
    float3 color;
    float rotation;
};


float2x2 inverse(float2x2 M)
{
    float2x2 inv;
    float invdet = 1.0f / determinant(M);
    inv[0][0] = M[1][1] * invdet;
    inv[1][1] = M[0][0] * invdet;
    inv[0][1] = -M[0][1] * invdet;
    inv[1][0] = -M[1][0] * invdet;
    return inv;
}


float2x2 rotation_matrix(float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return float2x2(c, -s, s, c);
}


float2x2 scale_matrix(float2 scale)
{
    return float2x2(scale.x, 0, 0, scale.y);
}


float gaussian(float2 x, float2x2 sigma)
{
    float2x2 sigma_inv = inverse(sigma);
    return exp(-0.5 * dot(x, mul(sigma_inv, x)));
}


float3 eval(no_diff float2 pos, Blob blob) {
    float2x2 R = rotation_matrix(blob.rotation);
    float2x2 S = scale_matrix(max(blob.scale, 0.001));
    float2x2 sigma = mul(mul(R, S), mul(transpose(S), transpose(R)));
    return gaussian(pos - blob.pos, sigma) * blob.color;
}

// Uniform data to be passed from application -> shader.
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
    StructuredBuffer<Blob> blobs;
    int blob_count;
    RWStructuredBuffer<float> result;
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



float3 gs2d(float2 pos) {
    float3 color = float3(0);
    for (uint i = 0; i < blob_count; ++i) {
        if (kSkipDistance > 0 && distance(pos, blobs[i].pos) > kSkipDistance)
             continue;
        color += eval(pos, blobs[i]);
    }
    return color;
}


[BackwardDifferentiable]
float3 loss(no_diff float2 screenPos, float3 screenColor)
{
    float3 refColor = (no_diff texRef.Load(int3(int2(screenPos), 0))).xyz;
    float3 rs = screenColor - refColor;
    rs *= rs;
    return rs;
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
    CoarseVertex coarseVertex: CoarseVertex,
    float4 pos: SV_Position) : SV_Target
{
    return float4(gs2d(pos.xy),1);
}


[shader("fragment")]
float4 fragmentDiffMain(
    CoarseVertex coarseVertex : CoarseVertex,
    float4 pos : SV_Position) : SV_Target
{

     DifferentialPair<float3> screenColor = diffPair(gs2d(pos.xy));
     bwd_diff(loss)(pos.xy, screenColor, 1);
     // write in pixel values the screenColor.d//
     
     // We'll use fast fixed point atomics instead of floats.

     result[3 * (uint(pos.x) + uint(pos.y) * 1024)] = screenColor.d.x;
     result[3 * (uint(pos.x) + uint(pos.y) * 1024) + 1] = screenColor.d.y;
     result[3 * (uint(pos.x) + uint(pos.y) * 1024) + 2] = screenColor.d.z;

     return float4(loss(pos.xy, (gs2d(pos.xy))), 1);
    
}
