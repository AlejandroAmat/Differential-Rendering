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
    RWStructuredBuffer<float> result;
    RWStructuredBuffer<int> grad_blobs;

}




[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint3 threadIdx: SV_DispatchThreadID)
{

    int idx = threadIdx.x;
    float learningrate = 0.7;
    if (idx < 4096) {
        // do

         blobs[idx].pos.x -= learningrate * float(float(grad_blobs[idx * 2]) / float(65536));

         blobs[idx].pos.y -= learningrate * float(float(grad_blobs[idx * 2 + 1]) / float(65536));
         grad_blobs[idx*2] = 0;
         grad_blobs[idx*2+1] = 0;

    }
    

}