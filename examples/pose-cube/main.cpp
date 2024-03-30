// main.cpp

// This file implements an extremely simple example of loading and
// executing a Slang shader program. This is primarily an example
// of how to use Slang as a "drop-in" replacement for an existing
// HLSL compiler like the `D3DCompile` API. More advanced usage
// of advanced Slang language and API features is left to the
// next example.
//
// The comments in the file will attempt to explain concepts as
// they are introduced.
//
// Of course, in order to use the Slang API, we need to include
// its header. We have set up the build options for this project
// so that it is as simple as:
//
#include <slang.h>
//
// Other build setups are possible, and Slang doesn't assume that
// its include directory must be added to your global include
// path.

// For the purposes of keeping the demo code as simple as possible,
// while still retaining some level of portability, our examples
// make use of a small platform and graphics API abstraction layer,
// which is included in the Slang source distribution under the
// `tools/` directory.
//
// Applications can of course use Slang without ever touching this
// abstraction layer, so we will not focus on it when explaining
// examples, except in places where best practices for interacting
// with Slang may depend on an application/engine making certain
// design choices in their abstraction layer.
//
#include "slang-gfx.h"
#include "gfx-util/shader-cursor.h"
#include "tools/platform/window.h"
#include "slang-com-ptr.h"
#include "source/core/slang-basic.h"
#include "examples/example-base/example-base.h"
#include "tools/platform/vector-math.h"
#include "Tensor.h"

using namespace gfx;
using namespace Slang;

struct Vertex
{
    float position[3];
    float color[4];
};
static const int kVertexCountQuad = 4;
static Vertex kVertexDataQuad[kVertexCountQuad]={
    {{0, 0, 0},{0,0,0,1}},
    {{0, 1, 0},{0,0,0,1}},
    {{1, 0, 0},{0,0,0,1}},
    {{1, 1, 0},{0,0,0,1}},
};


float initialParameters[2] = {0, 0}; // Example initial data
float initialInertia[2] = {0, 0};
Tensor myTensor2;
Tensor myTensor;

int dimensions =2;

struct Gradient : public WindowedAppBase
{

// Many Slang API functions return detailed diagnostic information
// (error messages, warnings, etc.) as a "blob" of data, or return
// a null blob pointer instead if there were no issues.
//
// For convenience, we define a subroutine that will dump the information
// in a diagnostic blob if one is produced, and skip it otherwise.
//
void diagnoseIfNeeded(slang::IBlob* diagnosticsBlob)
{
    if( diagnosticsBlob != nullptr )
    {
        printf("%s", (const char*) diagnosticsBlob->getBufferPointer());
    }
}
gfx::Result loadRenderProgram(
        gfx::IDevice* device, const char* fileName, const char* fragmentShader, gfx::IShaderProgram** outProgram)
        {
            //create slang session
            ComPtr<slang::ISession> slangSession;
            slangSession = device->getSlangSession();
            int a =3;
            ComPtr<slang::IBlob> diagnosticsBlob;
            
            // Loading the `shaders` module will compile and check all the shader code in it,
            // including the shader entry points we want to use. Now that the module is loaded
            // we can look up those entry points by name.
            slang::IModule* module = slangSession->loadModule(fileName, diagnosticsBlob.writeRef());
            diagnoseIfNeeded(diagnosticsBlob);
            if (!module)
                return SLANG_FAIL;

            //slang works using multi-entry point shader compilation as it works with unnifed shader 
            //programming so the entry points have to be created and found. In this case we are loading
            // the vertex and the specified fragment entry point.
            ComPtr<slang::IEntryPoint> vertexEntryPoint;
            SLANG_RETURN_ON_FAIL(
                module->findEntryPointByName("vertexMain", vertexEntryPoint.writeRef()));
            ComPtr<slang::IEntryPoint> fragmentEntryPoint;
            SLANG_RETURN_ON_FAIL(
                module->findEntryPointByName(fragmentShader, fragmentEntryPoint.writeRef()));

            //Cretae component type and the module is added together with the entry points.  
            //Modules and entry points are both examples of *component types* in the
            // Slang API. The API also provides a way to build a *composite* out of
            // other pieces, and that is what we are going to do with our module
            // and entry points.

            Slang::List<slang::IComponentType*> componentTypes;
            componentTypes.add(module);

            // Later on when we go to extract compiled kernel code for our vertex
            // and fragment shaders, we will need to make use of their order within
            // the composition, so we will record the relative ordering of the entry
            // points here as we add them.

            int entryPointCount = 0;
            int vertexEntryPointIndex = entryPointCount++;
            componentTypes.add(vertexEntryPoint);

            int fragmentEntryPointIndex = entryPointCount++;
            componentTypes.add(fragmentEntryPoint);

            // Actually creating the composite component type is a single operation
            // on the Slang session, but the operation could potentially fail if
            // something about the composite was invalid (e.g., you are trying to
            // combine multiple copies of the same module), so we need to deal
            // with the possibility of diagnostic output.

            ComPtr<slang::IComponentType> linkedProgram;
            SlangResult result = slangSession->createCompositeComponentType(
                componentTypes.getBuffer(),
                componentTypes.getCount(),
                linkedProgram.writeRef(),
                diagnosticsBlob.writeRef());
            diagnoseIfNeeded(diagnosticsBlob);
            SLANG_RETURN_ON_FAIL(result);
            
            // Once we've described the particular composition of entry points
            // that we want to compile, we defer to the graphics API layer
            // to extract compiled kernel code and load it into the API-specific
            // program representation.
            //
            gfx::IShaderProgram::Desc programDesc = {};
            programDesc.slangGlobalScope = linkedProgram;
            SLANG_RETURN_ON_FAIL(device->createProgram(programDesc, outProgram));

            return SLANG_OK;
        }
gfx::Result loadComputeProgram(
        gfx::IDevice* device, const char* fileName, gfx::IShaderProgram** outProgram)
    {
        ComPtr<slang::ISession> slangSession;
        slangSession = device->getSlangSession();

        ComPtr<slang::IBlob> diagnosticsBlob;
        slang::IModule* module = slangSession->loadModule(fileName, diagnosticsBlob.writeRef());
        diagnoseIfNeeded(diagnosticsBlob);
        if (!module)
            return SLANG_FAIL;

        Slang::List<slang::IComponentType*> componentTypes;
        componentTypes.add(module);
        ComPtr<slang::IEntryPoint> computeEntryPoint;
        SLANG_RETURN_ON_FAIL(
            module->findEntryPointByName("computeMain", computeEntryPoint.writeRef()));
        componentTypes.add(computeEntryPoint);

        ComPtr<slang::IComponentType> linkedProgram;
        SlangResult result = slangSession->createCompositeComponentType(
            componentTypes.getBuffer(),
            componentTypes.getCount(),
            linkedProgram.writeRef(),
            diagnosticsBlob.writeRef());
        diagnoseIfNeeded(diagnosticsBlob);
        SLANG_RETURN_ON_FAIL(result);

        gfx::IShaderProgram::Desc programDesc = {};
        programDesc.slangGlobalScope = linkedProgram;
        SLANG_RETURN_ON_FAIL(device->createProgram(programDesc, outProgram));

        return SLANG_OK;
    }

ComPtr<gfx::IPipelineState> gLossPipeline;
ComPtr<gfx::IPipelineState> gDrawQuadPipelineState;

ComPtr<gfx::IBufferResource> gVertexBufferQuad;
ComPtr<gfx::IBufferResource> gParametersBuffer;
ComPtr<gfx::IResourceView> gParametersBufferView;

ComPtr<gfx::IBufferResource> gMtBuffer;
ComPtr<gfx::IBufferResource> gVtBuffer;
ComPtr<gfx::IResourceView> gMtBufferView;
ComPtr<gfx::IResourceView> gVtBufferView;


ComPtr<gfx::IBufferResource> TensorBuffer;
ComPtr<gfx::IResourceView> TensorResource;
ComPtr<gfx::IBufferResource> StrideBuffer;
ComPtr<gfx::IResourceView> StrideResource;
ComPtr<gfx::IBufferResource> ShapeBuffer;
ComPtr<gfx::IResourceView> ShapeResource;

ComPtr<gfx::IBufferResource> TensorABuffer;
ComPtr<gfx::IResourceView> TensorAResource;


int frames =0;

ComPtr<gfx::IPipelineState> createComputePipelineState(IShaderProgram* program)
    {
        ComputePipelineStateDesc desc = {};
        desc.program = program;
        auto pipelineState = gDevice->createComputePipelineState(desc);
        return pipelineState;
    }
ComPtr<gfx::IPipelineState> createRenderPipelineStateQuad( IInputLayout* inputLayout, IShaderProgram* program)
    {
        GraphicsPipelineStateDesc desc;
        desc.inputLayout = inputLayout;
        desc.program = program;
        desc.framebufferLayout = gFramebufferLayout;
        desc.depthStencil.depthFunc = ComparisonFunc::LessEqual;
        desc.depthStencil.depthTestEnable = true;
        auto pipelineState = gDevice->createGraphicsPipelineState(desc);
        return pipelineState;
    }
ComPtr<gfx::IResourceView> createUAV(IBufferResource* buffer)
    {
        IResourceView::Desc desc = {};
        desc.type = IResourceView::Type::UnorderedAccess;
        desc.bufferElementSize = 0;
        return gDevice->createBufferView(buffer, nullptr, desc);
    }



Slang::Result initialize()
{
    
    
   
    freopen("output.txt", "a", stdout);
    initializeBase("PoseDiff", 1024, 768);
    

   
   // if(value>2)
    // Get and print values from the tensor.
    std::vector<size_t> shape = { 10, 10 };
    size_t dtypeSize = sizeof(float);
    Tensor myTensorloc (shape
        , dtypeSize);
    myTensor = std::move(myTensorloc);
     // Initialize all elements of myTensorloc to 0
    for (size_t i = 0; i < shape[0]; ++i) {
        for (size_t j = 0; j < shape[1]; ++j) {
            for (size_t k = 0; k < shape[2]; ++k) {
                myTensor.setValue<float>({i, j, k}, 5.0f);
            }
        }
    }   

    auto strides = myTensor.getStrides();
    
    
       // Use std::move to transfer ownership of tensors

    InputElementDesc inputElements[] = {
        { "POSITION", 0, Format::R32G32B32_FLOAT, offsetof(Vertex, position) },
        { "COLOR", 0, Format::R32G32B32_FLOAT, offsetof(Vertex, color) }
    };

    gfx::IBufferResource::Desc bufferDescLoss = {};
    bufferDescLoss.allowedStates.add(ResourceState::UnorderedAccess);
    bufferDescLoss.allowedStates.add(ResourceState::CopySource);
    bufferDescLoss.sizeInBytes = sizeof(float)*dimensions;
    bufferDescLoss.type = IResource::Type::Buffer;
    gParametersBuffer = gDevice->createBufferResource(bufferDescLoss, &initialParameters);
    gParametersBufferView = createUAV(gParametersBuffer);

    gfx::IBufferResource::Desc bufferDescM = {};
    bufferDescM.allowedStates.add(ResourceState::UnorderedAccess);
    bufferDescM.allowedStates.add(ResourceState::CopySource);
    bufferDescM.sizeInBytes = sizeof(float)*dimensions;
    bufferDescM.type = IResource::Type::Buffer;
    gMtBuffer = gDevice->createBufferResource(bufferDescM,&initialInertia);
    gMtBufferView = createUAV(gMtBuffer);

    gfx::IBufferResource::Desc bufferDescV = {};
    bufferDescV.allowedStates.add(ResourceState::UnorderedAccess);
    bufferDescV.allowedStates.add(ResourceState::CopySource);
    bufferDescV.sizeInBytes = sizeof(float)*dimensions;
    bufferDescV.type = IResource::Type::Buffer;
    gVtBuffer = gDevice->createBufferResource(bufferDescV,&initialInertia);
    gVtBufferView = createUAV(gVtBuffer);

    gfx::IBufferResource::Desc TensorResourceDes = {};
    TensorResourceDes.allowedStates.add(ResourceState::UnorderedAccess);
    TensorResourceDes.allowedStates.add(ResourceState::CopySource);
    TensorResourceDes.sizeInBytes =   std::accumulate(shape.begin(), shape.end(), 1,
                                   std::multiplies<size_t>()) * dtypeSize;
    TensorResourceDes.type = IResource::Type::Buffer;
    TensorBuffer = gDevice->createBufferResource(TensorResourceDes,myTensor.getDataPointer());
    TensorResource = createUAV(TensorBuffer);

    
    TensorABuffer = gDevice->createBufferResource(TensorResourceDes);
    TensorAResource = createUAV(TensorABuffer);

    gfx::IBufferResource::Desc StrideDesc = {};
    StrideDesc.allowedStates.add(ResourceState::UnorderedAccess);
    StrideDesc.allowedStates.add(ResourceState::CopySource);
    StrideDesc.sizeInBytes = sizeof(size_t)*shape.size();
    StrideDesc.type = IResource::Type::Buffer;
    StrideBuffer = gDevice->createBufferResource(StrideDesc, strides.data());
    StrideResource = createUAV(StrideBuffer);


    gfx::IBufferResource::Desc ShapeDesc = {};
        ShapeDesc.allowedStates.add(ResourceState::UnorderedAccess);
        ShapeDesc.allowedStates.add(ResourceState::CopySource);
        ShapeDesc.sizeInBytes = sizeof(size_t)*shape.size();
        ShapeDesc.type = IResource::Type::Buffer;
        ShapeBuffer = gDevice->createBufferResource(ShapeDesc, shape.data());
        ShapeResource = createUAV(ShapeBuffer);

    {       
        ComPtr<IShaderProgram> shaderProgram;
        SLANG_RETURN_ON_FAIL(loadComputeProgram(gDevice, "test", shaderProgram.writeRef()));
        gLossPipeline = createComputePipelineState(shaderProgram);
    }
  
}


virtual void renderFrame(int frameBufferIndex) override
{
    
    /*{
        ComPtr<ICommandBuffer> commandBuffer = gTransientHeaps[frameBufferIndex]->createCommandBuffer();
        auto resEncoder = commandBuffer->encodeResourceCommands();
        resEncoder->bufferBarrier(gParametersBuffer, ResourceState::Undefined, ResourceState::UnorderedAccess);
        resEncoder->bufferBarrier(gMtBuffer, ResourceState::Undefined, ResourceState::UnorderedAccess);
        resEncoder->bufferBarrier(gVtBuffer, ResourceState::Undefined, ResourceState::UnorderedAccess);
        resEncoder->endEncoding();
        commandBuffer->close();
        gQueue->executeCommandBuffer(commandBuffer);
    }
    {
    ComPtr<ICommandBuffer> commandBuffer = gTransientHeaps[frameBufferIndex]->createCommandBuffer();
            auto encoder = commandBuffer->encodeComputeCommands();
            auto rootObject = encoder->bindPipeline(gLossPipeline);
            ShaderCursor rootCursor(rootObject);
            rootCursor["Uniforms"]["learningRate"].setData(0.1f);
            rootCursor["Uniforms"]["beta1"].setData(0.9f);
            rootCursor["Uniforms"]["beta2"].setData(0.999f);
            rootCursor["Uniforms"]["epsilon"].setData(1e-8);
            rootCursor["Uniforms"]["parameters"].setResource(gParametersBufferView);
            rootCursor["Uniforms"]["m"].setResource(gMtBufferView);
            rootCursor["Uniforms"]["v"].setResource(gVtBufferView);
            rootCursor["Uniforms"]["iteration"].setData(frames+1);
            encoder->dispatchCompute(
                    1, 1, 1);
            encoder->endEncoding();
            commandBuffer->close();
            gQueue->executeCommandBuffer(commandBuffer);
                
    }
    */
    {
        ComPtr<ICommandBuffer> commandBuffer = gTransientHeaps[frameBufferIndex]->createCommandBuffer();
        auto resEncoder = commandBuffer->encodeResourceCommands();
        resEncoder->bufferBarrier(TensorBuffer, ResourceState::Undefined, ResourceState::UnorderedAccess);
        resEncoder->bufferBarrier(StrideBuffer, ResourceState::Undefined, ResourceState::UnorderedAccess);
        resEncoder->bufferBarrier(ShapeBuffer, ResourceState::Undefined, ResourceState::UnorderedAccess);
        resEncoder->bufferBarrier(TensorABuffer, ResourceState::Undefined, ResourceState::UnorderedAccess);
        resEncoder->endEncoding();
        commandBuffer->close();
        gQueue->executeCommandBuffer(commandBuffer);
    }
    {
    ComPtr<ICommandBuffer> commandBuffer = gTransientHeaps[frameBufferIndex]->createCommandBuffer();
            auto encoder = commandBuffer->encodeComputeCommands();
            auto rootObject = encoder->bindPipeline(gLossPipeline);
            ShaderCursor rootCursor(rootObject);
            rootCursor["Uniforms"]["dim"].setData(3);
            rootCursor["Uniforms"]["tensor"].setResource(TensorResource);
            rootCursor["Uniforms"]["strides"].setResource(StrideResource);
            rootCursor["Uniforms"]["shape"].setResource(ShapeResource);
            rootCursor["Uniforms"]["result"].setResource(TensorAResource);
            encoder->dispatchCompute(
                    1, 1, 1);
            encoder->endEncoding();
            commandBuffer->close();
            gQueue->executeCommandBuffer(commandBuffer);
                
    }
    

    
    gSwapchain->present();
    

    gQueue->waitOnHost();
    std::vector<float> params;
    const size_t bufferSize = TensorABuffer->getDesc()->sizeInBytes;
    ComPtr<ISlangBlob> blob;

    // Read the buffer contents into a blob
    gDevice->readBufferResource(TensorABuffer, 0, bufferSize, blob.writeRef());
    if (!blob)
        printf("BAD");

    // Cast the blob data to the type stored in the buffer, in this case, let's assume it's float
    const float* parameters = (const float*)blob->getBufferPointer();
    size_t paramCount = bufferSize / sizeof(float);
    String var[2]= {"x", "y\n"};
    // Copy the loss values into the output vector
    params.assign(parameters, parameters + paramCount);
     for(int k=0; k<paramCount; k++){
                printf(" %f:: %d \n", params[k], frames);
    }

    //printf("\n");
frames++;

    
}



};



// This macro instantiates an appropriate main function to
// run the application defined above.
PLATFORM_UI_MAIN(innerMain<Gradient>)
