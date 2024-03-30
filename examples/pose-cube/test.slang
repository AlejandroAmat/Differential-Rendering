// A compute shader to add gradients to a mip-map texture.
#include "Tensor.slang"
cbuffer Uniforms
{
    int dim;
    RWStructuredBuffer<float> tensor;
    RWStructuredBuffer<float> result;
    RWStructuredBuffer<size_t> strides;
    RWStructuredBuffer<size_t> shape;
}


uint calculateFlatIndex(uint idx[MAX_TENSOR_RANK])
{
    uint linearIndex = 0;
    for (uint i = 0; i < dim; ++i)
    {
        linearIndex += idx[i] * uint(strides[i]);
    }
    return linearIndex;
}

[shader("compute")]
[numthreads(16, 16, 1)]
void computeMain(uint3 DTID: SV_DispatchThreadID)
{
    uint row = DTID.y;
    uint col = DTID.x;

    // Ensure we're within the bounds of the matrix
    if (row >= shape[0] || col >= shape[1]) return;

    float sum = 0.0;
    for (uint k = 0; k < shape[0]; ++k)
    {

        sum += tensor[calculateFlatIndex( { row, k, 0, 0 })] * tensor[calculateFlatIndex({ k, col, 0, 0 })];
    }

    // Store the computed value in the result matrix

    result[calculateFlatIndex( { row, col, 0, 0 })] = sum;
}
