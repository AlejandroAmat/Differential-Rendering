// Vertex and fragment shader to draw a textured quad on screen.

cbuffer Uniforms
{
    int x;
    int y;
    int width;
    int height;
    int viewWidth;
    int viewHeight;
    
}

struct AssembledVertex
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct Fragment
{
    float4 color;
};

struct VertexStageOutput
{
    float4 color : COLOR;
    float4          sv_position     : SV_Position;
};

[shader("vertex")]
VertexStageOutput vertexMain(
    AssembledVertex assembledVertex)
{
    VertexStageOutput output;

    float3 position = assembledVertex.position;

    output.color = assembledVertex.color;
    output.sv_position.x = (x + position.x * width) / (float)viewWidth * 2.0f - 1.0f;
    output.sv_position.y = -((y + position.y * height) / (float)viewHeight * 2.0f - 1.0f);
    output.sv_position.z = 0;
    output.sv_position.w = 1.0;
    return output;
}

[shader("fragment")]
float4 fragmentMain(
    float4 color : COLOR) : SV_Target
{
    return color;
}
