// library-test.slang

// A test to try out the basics of using libraries that can be linked (by downstream tools).
// (This is in contrast to Slang modules and Slangs own linkage mechanisms)

// This didn't work for lib_6_2 when compiling via DXC (!). Even though it's stated elsewhere the feature is available from 6.1

//TEST:COMPILE: tests/library/library.slang -profile lib_6_3 -target dxil -o tests/library/library.dxil
//TEST:COMPILE: tests/library/library-test.slang -entry computeMain -profile cs_6_3 -target dxil -r tests/library/library.dxil -o tests/library/library-test.dxil 

// Test the produced kernel.

//TEST:COMPARE_COMPUTE_EX:-slang -compute -profile cs_6_3 -dx12 -use-dxil -shaderobj -Xslang... -r tests/library/library.dxil -X.

extern int foo(int a);

//TEST_INPUT:ubuffer(data=[0 0 0 0 ], stride=4):out,name outputBuffer
RWStructuredBuffer<int> outputBuffer;

[shader("compute")]
[numthreads(4, 1, 1)]
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    int index = (int)dispatchThreadID.x;
       
    outputBuffer[index] = foo(index);
}
