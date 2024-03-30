cbuffer Uniforms
{
    float learningRate;
    float beta1;
    float beta2;
    double epsilon;
    RWStructuredBuffer<float> parameters;
    RWStructuredBuffer<float> m; // First moment vector
    RWStructuredBuffer<float> v; // Second moment vector
    uint iteration; // Keep track of the iteration for bias correction
}

[BackwardDifferentiable]
float lossFunction(float x, float y) {
    return (x - 2) * (x - 2) + (y - 3) * (y - 3);
}


[shader("compute")]
[numthreads(1, 1, 1)]
void computeMain(uint3 threadIdx : SV_DispatchThreadID)
{
    var dpX = diffPair(parameters[0], 0.0); // Initialize derivative part to 0.0 because bwd_diff will compute it
    var dpY = diffPair(parameters[1], 0.0); // Same as above

    // Propagate the derivative of the loss (usually 1.0 as the starting point) back to the inputs
    bwd_diff(lossFunction)(dpX, dpY, 1.0);

    // Update first moment (mean) estimate
    m[0] = beta1 * m[0] + (1 - beta1) * dpX.d;
    m[1] = beta1 * m[1] + (1 - beta1) * dpY.d;

    // Update second moment (uncentered variance) estimate
    v[0] = beta2 * v[0] + (1 - beta2) * (dpX.d * dpX.d);
    v[1] = beta2 * v[1] + (1 - beta2) * (dpY.d * dpY.d);

    // Compute bias-corrected first moment estimate
    float mHatX = m[0] / (1 - pow(beta1, iteration));
    float mHatY = m[1] / (1 - pow(beta1, iteration));

    // Compute bias-corrected second moment estimate
    float vHatX = v[0] / (1 - pow(beta2, iteration));
    float vHatY = v[1] / (1 - pow(beta2, iteration));

    // Update parameters
    parameters[0] = parameters[0] - learningRate * mHatX / (sqrt(vHatX) + (float)epsilon);
    parameters[1] = parameters[1] - learningRate * mHatY / (sqrt(vHatY) + (float)epsilon);
}
