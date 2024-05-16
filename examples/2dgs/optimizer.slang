struct Blob {
    float2 pos;
    float2 scale;
    float3 color;
    float rotation;
};


int2 resolution = (1024, 1024);
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
    RWStructuredBuffer<float> blobs;
    int blob_count;
    RWStructuredBuffer<float> result;
    RWStructuredBuffer<int> grad_blobs;
    float learningRate;
    float beta1;
    float beta2;
    double epsilon;
    RWStructuredBuffer<float> m; // First moment vector
    RWStructuredBuffer<float> v; // Second moment vector
    uint iteration;              // Keep track of the iteration for bias correction

}




[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint3 threadIdx: SV_DispatchThreadID)
{

    int idx = threadIdx.x;
    float learningRate = 0.0035;
    if (idx < 4096) {

        // Assuming beta1, beta2, learningRate, epsilon are defined
// Assuming m and v are appropriately sized and initialized

for (int i = 0; i < 8; i++) {
    // Scale gradient and retrieve from buffer
    float dp = float(grad_blobs[idx * 8 + i]) / float(65536);

    // Update first moment (momentum) estimate
    m[idx * 8 + i] = beta1 * m[idx * 8 + i] + (1 - beta1) * dp;

    // Update second moment (uncentered variance) estimate
    v[idx * 8 + i] = beta2 * v[idx * 8 + i] + (1 - beta2) * (dp * dp);

    // Compute bias-corrected first moment estimate
    float mHat = m[idx * 8 + i] / (1 - pow(beta1, iteration));

    // Compute bias-corrected second moment estimate
    float vHat = v[idx * 8 + i] / (1 - pow(beta2, iteration));

    // Update parameters based on calculated moments
    float updateValue = learningRate * mHat / (sqrt(vHat) + epsilon);
    // Assuming blobs[idx].params[i] is the corresponding parameter
    blobs[idx*8+i] -= updateValue;

    // Reset gradient buffer for the next iteration
    grad_blobs[idx * 8 + i] = 0;
}

// Note: Ensure 'blobs',


    }
    

}