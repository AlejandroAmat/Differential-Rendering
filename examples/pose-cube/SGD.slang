// A compute shader to add gradients to a mip-map texture.

cbuffer Uniforms
{
    float learningRate;
    RWStructuredBuffer<float> parameters;
}


float lossFunction(float x, float y) {
    return sqrt(x - 2) + sqrt(y - 3) ; 
}



float2 gradient(float x, float y) {
    float2 gradient;
    gradient.x = 2 * x - 4;
    gradient.y = 2 * y - 6;
    
    return gradient;
}



[shader("compute")]
[numthreads(1, 1, 1)]
void computeMain(uint3 threadIdx : SV_DispatchThreadID)
{
 
     parameters[0] = parameters[0] - learningRate * gradient(parameters[0], parameters[1]).x;
     parameters[1] = parameters[1] - learningRate * gradient(parameters[0], parameters[1]).y;
  


}
