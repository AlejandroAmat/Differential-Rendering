// TensorOpsImpl.slang

// Assume ITensorOps<T> interface is defined somewhere else to include:

#include "ITensor.slang"
#define MAX_TENSOR_RANK 4
// Generic implementation of ITensorOps for a RWStructuredBuffer of any type T
struct Tensor<T> : ITensor<T>
{
    // The data buffer is now also generic, supporting any type T
    RWStructuredBuffer<T> data;
    RWStructuredBuffer<size_t> strides;
    uint dim;

    uint calculateFlatIndex(uint idx[MAX_TENSOR_RANK])
    {
        uint linearIndex = 0;
        for (uint i = 0; i < dim; ++i)
        {
            linearIndex += idx[i] * uint(strides[i]);
        }
        return linearIndex;
    }

    T getElement(uint idx[MAX_TENSOR_RANK])
    {
        uint  idxi = calculateFlatIndex(idx);
        return data[idxi];
    }

    void setElement(uint idx[MAX_TENSOR_RANK], T value)
    {
        uint idxi = calculateFlatIndex(idx);
        data[idxi] = value;
    }
}
