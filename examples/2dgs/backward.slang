#define NV_SHADER_EXTN_SLOT u0 



static const float kSkipDistance = 0.1;
struct Blob : IDifferentiable {
    float2 pos;
    float2 scale;
    float3 color;
    float rotation;
};

[BackwardDifferentiable]
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

[BackwardDifferentiable]
float2x2 rotation_matrix(float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return float2x2(c, -s, s, c);
}

[BackwardDifferentiable]
float2x2 scale_matrix(float2 scale)
{
    return float2x2(scale.x, 0, 0, scale.y);
}

[BackwardDifferentiable]
float gaussian(float2 x, float2x2 sigma)
{
    float2x2 sigma_inv = inverse(sigma);
    return exp(-0.5 * dot(x, mul(sigma_inv, x)));
}

[BackwardDifferentiable]
float3 sigmoid(float3 x) {
    return 1.0 / (1.0 + exp(-x));
}

[BackwardDifferentiable]
float3 eval(no_diff float2 pos, Blob blob) {
    float3 color = sigmoid(blob.color); // Apply sigmoid to color
    float2 scale = exp(blob.scale);
    float2x2 R = rotation_matrix(blob.rotation);
    float2x2 S = scale_matrix(max(scale, 0.001));
    float2x2 sigma = mul(mul(R, S), mul(transpose(S), transpose(R)));
    return gaussian(pos - blob.pos, sigma) * color;
    
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
    int blob_count;
    RWStructuredBuffer<float> result;
    RWStructuredBuffer<int> grad_blobs;
    RWStructuredBuffer<int> grad;
    RWStructuredBuffer<Blob> blobs;

}

void write_grad(uint offset, float value)
{

    //float sum = WaveActiveSum(value);
    //if (WaveIsFirstLane())
       InterlockedAdd(grad_blobs[offset], int(value*65336));
    

   // InterlockedAdd(grad_blobs[0], int(sum));

    
}


[shader("compute")]
[numthreads(16, 16, 1)]
void computeMain(uint3 threadIdx: SV_DispatchThreadID)
{
    uint x = threadIdx.x;
    uint y = threadIdx.y;
    if (x >= width) return;
    if (y >= height) return;
    
    int2 resolution = (width, height);
    no_diff float2 pos = float2(threadIdx.x,threadIdx.y) / resolution;
    float d_image_r = result[(y * width + x) * 3];
    float d_image_g = result[(y * width + x) * 3 + 1];
    float d_image_b = result[(y * width + x) * 3 + 2];

    float3 d_image = (d_image_r, d_image_g, d_image_g);

    int k = 0;
    for (uint i = 0; i < blob_count-1; ++i) {
        var d_blob = diffPair(blobs[i]);
       
        if (kSkipDistance > 0 && distance(pos, blobs[i].pos) >0.1)
            continue;
      
        bwd_diff(eval)(pos, d_blob,d_image);
     
        write_grad(i * 2, d_blob.d.pos.x);
        write_grad(i * 2 + 1, d_blob.d.pos.y);


        
        //else result[i * 3] = 33;
       
        
      
    }
    

}