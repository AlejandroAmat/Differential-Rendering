// A compute shader to add gradients to a mip-map texture.

cbuffer Uniforms
{
    uint dstWidth;
    uint dstHeight;
    float learningRate;
    RWTexture2D dstTexture;
    RWTexture2D srcTexture;
    RWTexture2D refTexture; // Reference texture for comparison
    RWStructuredBuffer<float> loss;
    int iter;
}

[shader("compute")]
[numthreads(16, 16, 1)]
void computeMain(uint3 threadIdx : SV_DispatchThreadID)
{
    uint x = threadIdx.x;
    uint y = threadIdx.y;
    if (x >= dstWidth) return;
    if (y >= dstHeight) return;
    var val = srcTexture[uint2(x, y)];

    dstTexture[uint2(x, y)] = float4((dstTexture[uint2(x, y)] - val * learningRate).xyz, 1.0);
    if (iter == 0) {
        var ref = refTexture[uint2(x, y)];
        var diff = dstTexture[uint2(x, y)].xyz - ref.xyz;
        var fdiff = dot(diff, diff);
        loss[((y *dstWidth + x))] = fdiff;

        

        
        
    }
}
