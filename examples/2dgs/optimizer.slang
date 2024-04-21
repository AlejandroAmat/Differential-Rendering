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
    RWStructuredBuffer<Blob> blobs;
    int blob_count;
    RWStructuredBuffer <float> result;
    RWStructuredBuffer <int> grad_blobs;

}




[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint3 threadIdx: SV_DispatchThreadID)
{

    int idx = threadIdx.x;
    float learningrate = -0.01f;
    if (idx < 4096) {
        // do
        blobs[idx].pos.x += learningrate * float(grad_blobs[idx * 8]/float(65536));
        grad_blobs[idx * 8] = 0;
        blobs[idx].pos.y += learningrate * float(grad_blobs[idx * 8 + 1] / float(65536));
        grad_blobs[idx * 8 + 1] = 0;
        blobs[idx].scale.x += learningrate * float(grad_blobs[idx * 8 + 2] / float(65536));
        grad_blobs[idx * 8 + 2] = 0;
        blobs[idx].scale.y += learningrate * float(grad_blobs[idx * 8 + 3] / float(65536));
        grad_blobs[idx * 8 + 3] = 0;
        blobs[idx].color.x -= 0.01 * float(grad_blobs[idx * 8 + 4] / float(65536));
        grad_blobs[idx * 8 + 4] = 0;
        blobs[idx].color.y -= 0.01 * float(grad_blobs[idx * 8 + 5] / float(65536));
        grad_blobs[idx * 8 + 5] = 0;
        blobs[idx].color.z -= 0.01 * float(grad_blobs[idx * 8 + 6] / float(65536));
        grad_blobs[idx * 8 + 6] = 0;
        blobs[idx].rotation += learningrate * float(grad_blobs[idx * 8 + 7] / float(65536));
        grad_blobs[idx * 8 + 7] = 0;
    }
    

}