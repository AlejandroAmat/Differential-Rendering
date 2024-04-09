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

using namespace gfx;
using namespace Slang;




//Vertes struct consistion of a 3 dimension float vector
struct Vertex
{
    float position[3];
    float color[4];
};
int frame=0;

const int BLOB_COUNT = 1024*4; // Example blob count
float blobs[BLOB_COUNT*8];
//Data passing to vertexshader. In this case a 4 point plane (square)
static const int kVertexCount = 4;
static Vertex kVertexData[kVertexCount]={
    {{0, 0, 0},{0,0,0,1}},
    {{0, 1, 0},{0,0,0,1}},
    {{1, 0, 0},{0,0,0,1}},
    {{1, 1, 0},{0,0,0,1}},
};

static const int kVertexCountQuad = 4;
static Vertex kVertexDataQuad[kVertexCountQuad]={
    {{0, 0, 0},{0,0,0,1}},
    {{0, 1, 0},{0,0,0,1}},
    {{1, 0, 0},{0,0,0,1}},
    {{1, 1, 0},{0,0,0,1}},
};



// The example application will be implemented as a `struct`, so that
// we can scope the resources it allocates without using global variables.
//
struct DiffPose : public WindowedAppBase
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

// The main task an application cares about is compiling shader code
// from souce (if needed) and loading it through the chosen graphics API.
//
// In addition, an application may want to receive reflection information
// about the program, which is what a `slang::ProgramLayout` provides.
//
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
//
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

// The above function shows the core of what is required to use the
// Slang API as a simple compiler (e.g., a drop-in replacement for
// fxc or dxc).
//
// The rest of this file implements an extremely simple rendering application
// that will execute the vertex/fragment shaders loaded with the function
// we have just defined.
//

// We will define global variables for the various platform and
// graphics API objects that our application needs:
//
// As a reminder, *none* of these are Slang API objects. All
// of them come from the utility library we are using to simplify
// building an example program.
//
glm::mat4x4 transformMatrix;

ComPtr<gfx::IPipelineState> gPipelineState;
ComPtr<gfx::IPipelineState> gGaussianState;
ComPtr<gfx::IPipelineState> gOptimizerState;
ComPtr<gfx::IPipelineState> gPipelineIterState;
ComPtr<gfx::IPipelineState> gDrawQuadPipelineState;
ComPtr<gfx::IBufferResource> gVertexBuffer;
ComPtr<gfx::IBufferResource> gVertexBufferQuad;


ComPtr<gfx::ISamplerState> gSampler;

ComPtr<gfx::IResourceView> gTexView;
ComPtr<gfx::IFramebuffer> gRefFrameBuffer;
ComPtr<gfx::IFramebuffer> gResultFrameBuffer;

ComPtr<gfx::IBufferResource> BlobsBuffer;
ComPtr<gfx::IResourceView> BlobsBufferView;
ComPtr<gfx::IBufferResource> BlobsGradientBuffer;
ComPtr<gfx::IResourceView> BlobsGradientBufferView;
ComPtr<gfx::IBufferResource> ResultBuffer;
ComPtr<gfx::IResourceView> ResultResource;

ComPtr<gfx::ITextureResource> gRefImage;
ComPtr<gfx::IResourceView> gRefImageSRV;
ComPtr<gfx::IResourceView> gRefImageRTV;

ComPtr<gfx::ITextureResource> gResultImage;
ComPtr<gfx::IResourceView> gResultImageSRV;
ComPtr<gfx::IResourceView> gResultImageRTV;

ComPtr<gfx::ITextureResource> gDepthTexture;
ComPtr<gfx::IResourceView> gDepthTextureView;

ComPtr<gfx::IResourceView> gIterImageDSV;


ClearValue kClearValue; 

ComPtr<gfx::ITextureResource> createRenderTargetTexture(gfx::Format format, int w, int h, int levels)
    {
        gfx::ITextureResource::Desc textureDesc = {};
        textureDesc.allowedStates.add(ResourceState::ShaderResource);
        textureDesc.allowedStates.add(ResourceState::UnorderedAccess);
        textureDesc.allowedStates.add(ResourceState::RenderTarget);
        textureDesc.allowedStates.add(ResourceState::CopySource);
        textureDesc.defaultState = ResourceState::RenderTarget;
        textureDesc.format = format;
        textureDesc.numMipLevels = levels;
        textureDesc.type = gfx::IResource::Type::Texture2D;
        textureDesc.size.width = w;
        textureDesc.size.height = h;
        textureDesc.size.depth = 1;
        textureDesc.optimalClearValue = &kClearValue;
        return gDevice->createTextureResource(textureDesc, nullptr);
    }

    // Method that creates DepthTexture
    ComPtr<gfx::ITextureResource> createDepthTexture()
    {
        gfx::ITextureResource::Desc textureDesc = {};
        textureDesc.allowedStates.add(ResourceState::DepthWrite);
        textureDesc.defaultState = ResourceState::DepthWrite;
        textureDesc.format = gfx::Format::D32_FLOAT;
        textureDesc.numMipLevels = 1;
        textureDesc.type = gfx::IResource::Type::Texture2D;
        textureDesc.size.width = windowWidth;
        textureDesc.size.height = windowHeight;
        textureDesc.size.depth = 1;
        ClearValue clearValue = {};
        textureDesc.optimalClearValue = &clearValue;
        return gDevice->createTextureResource(textureDesc, nullptr);
    }

    // Method that creates the Rendering Frame Buffer from a Texture Resource View. In this context, from the RTV view of the Texture Resources. It uses
    //the layout of the global gFramebufferLayout set by the gfx API
    ComPtr<gfx::IFramebuffer> createRenderTargetFramebuffer(IResourceView* tex)
    {
        IFramebuffer::Desc desc = {};
        desc.layout = gFramebufferLayout.get();
        desc.renderTargetCount = 1;
        desc.renderTargetViews = &tex;
        desc.depthStencilView = gDepthTextureView;
        return gDevice->createFramebuffer(desc);
    }

    //create each of the 4 texture Views from texture Resource. 

    ComPtr<gfx::IResourceView> createRTV(ITextureResource* tex, Format f)
    {
        IResourceView::Desc rtvDesc = {};
        rtvDesc.type = IResourceView::Type::RenderTarget;
        rtvDesc.subresourceRange.mipLevelCount = 1;
        rtvDesc.format = f;
        rtvDesc.renderTarget.shape = gfx::IResource::Type::Texture2D;
        return gDevice->createTextureView(tex, rtvDesc);
    }
    ComPtr<gfx::IResourceView> createDSV(ITextureResource* tex)
    {
        IResourceView::Desc dsvDesc = {};
        dsvDesc.type = IResourceView::Type::DepthStencil;
        dsvDesc.subresourceRange.mipLevelCount = 1;
        dsvDesc.format = Format::D32_FLOAT;
        dsvDesc.renderTarget.shape = gfx::IResource::Type::Texture2D;
        return gDevice->createTextureView(tex, dsvDesc);
    }
    ComPtr<gfx::IResourceView> createSRV(ITextureResource* tex)
    {
        IResourceView::Desc rtvDesc = {};
        rtvDesc.type = IResourceView::Type::ShaderResource;
        return gDevice->createTextureView(tex, rtvDesc);
    }
    ComPtr<gfx::IResourceView> createUAV(IBufferResource* buffer)
    {
        IResourceView::Desc desc = {};
        desc.type = IResourceView::Type::UnorderedAccess;
        desc.bufferElementSize = 0;
        return gDevice->createBufferView(buffer, nullptr, desc);
    }
    ComPtr<gfx::IResourceView> createUAV(ITextureResource* texture, int level)
    {
        IResourceView::Desc desc = {};
        desc.type = IResourceView::Type::UnorderedAccess;
        desc.subresourceRange.layerCount = 1;
        desc.subresourceRange.mipLevel = level;
        desc.subresourceRange.baseArrayLayer = 0;
        return gDevice->createTextureView(texture,desc);
    }



    //Create Pipelines for Rendering/Compute stages

    ComPtr<gfx::IPipelineState> createRenderPipelineState(
        IInputLayout* inputLayout, IShaderProgram* program)
    {
        GraphicsPipelineStateDesc desc;
        desc.inputLayout = inputLayout;
        desc.program = program;
        desc.framebufferLayout = gFramebufferLayout;
        desc.primitiveType = PrimitiveType::Triangle;
       // desc.depthStencil.depthFunc = ComparisonFunc::LessEqual;
        //desc.depthStencil.depthTestEnable = true;
       // desc.rasterizer.cullMode = gfx::CullMode::Front;
        auto pipelineState = gDevice->createGraphicsPipelineState(desc);
        return pipelineState;
    }

     ComPtr<gfx::IPipelineState> createRenderPipelineStateQuad(
        IInputLayout* inputLayout, IShaderProgram* program)
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

    
ComPtr<gfx::IPipelineState> createComputePipelineState(IShaderProgram* program)
    {
        ComputePipelineStateDesc desc = {};
        desc.program = program;
        auto pipelineState = gDevice->createComputePipelineState(desc);
        return pipelineState;
    }



List<uint32_t> mipMapOffset;
    int textureWidth;
    int textureHeight;

// Now that we've covered the function that actually loads and
// compiles our Slang shade code, we can go through the rest
// of the application code without as much commentary.
//


Slang::Result initialize()
{
   
    freopen("output.txt", "a", stdout);
    initializeBase("PoseDiff", 1024, 1024);
  
    // We will create objects needed to configur the "input assembler"
    // (IA) stage of the D3D pipeline.
    //
    // First, we create an input layout:
    //
    InputElementDesc inputElements[] = {
        { "POSITION", 0, Format::R32G32B32_FLOAT, offsetof(Vertex, position) },
        { "COLOR", 0, Format::R32G32B32_FLOAT, offsetof(Vertex, color) }
    };
    auto inputLayout = gDevice->createInputLayout(
        sizeof(Vertex),
        &inputElements[0],
        2);
    if(!inputLayout) return SLANG_FAIL;

    kClearValue.color.floatValues[0] = 0.3f;
    kClearValue.color.floatValues[1] = 0.5f;
    kClearValue.color.floatValues[2] = 0.7f;
    kClearValue.color.floatValues[3] = 1.0f;

    gfx::ISamplerState::Desc samplerDesc = {};
    gSampler = gDevice->createSamplerState(samplerDesc);
    
 
    //initMipOffsets(textureWidth, textureHeight);
    // Next we allocate a vertex buffer for our pre-initialized
    // vertex data.
    //
    
    IBufferResource::Desc vertexBufferDesc;
    vertexBufferDesc.type = IResource::Type::Buffer;
    vertexBufferDesc.sizeInBytes = kVertexCount * sizeof(Vertex);
    vertexBufferDesc.defaultState = ResourceState::VertexBuffer;
    gVertexBuffer = gDevice->createBufferResource(vertexBufferDesc, &kVertexData[0]);
    if(!gVertexBuffer) return SLANG_FAIL;

    IBufferResource::Desc vertexBufferDescQuad;
    vertexBufferDescQuad.type = IResource::Type::Buffer;
    vertexBufferDescQuad.sizeInBytes = kVertexCountQuad * sizeof(Vertex);
    vertexBufferDescQuad.defaultState = ResourceState::VertexBuffer;
    gVertexBufferQuad = gDevice->createBufferResource(vertexBufferDescQuad, &kVertexDataQuad[0]);
    if (!gVertexBufferQuad)
        return SLANG_FAIL;

    initBlobs();
    gfx::IBufferResource::Desc bufferDesc = {};
    bufferDesc.allowedStates.add(ResourceState::ShaderResource);
    bufferDesc.allowedStates.add(ResourceState::UnorderedAccess);
    bufferDesc.allowedStates.add(ResourceState::General);
    bufferDesc.sizeInBytes = BLOB_COUNT * sizeof (float)*8; 
    bufferDesc.type = IResource::Type::Buffer;
    BlobsBuffer = gDevice->createBufferResource(bufferDesc, &blobs);
    BlobsBufferView = createUAV(BlobsBuffer);

    gfx::IBufferResource::Desc bufferDescb = {};
    bufferDescb.allowedStates.add(ResourceState::ShaderResource);
    bufferDescb.allowedStates.add(ResourceState::UnorderedAccess);
    bufferDescb.allowedStates.add(ResourceState::General);
    bufferDescb.sizeInBytes = BLOB_COUNT * sizeof (float)*8; 
    bufferDescb.type = IResource::Type::Buffer ;
    BlobsGradientBuffer = gDevice->createBufferResource(bufferDescb);
    BlobsGradientBufferView = createUAV(BlobsGradientBuffer);

    gfx::IBufferResource::Desc bufferDescr = {};
    bufferDescr.allowedStates.add(ResourceState::ShaderResource);
    bufferDescr.allowedStates.add(ResourceState::UnorderedAccess);
    bufferDescr.allowedStates.add(ResourceState::General);
    bufferDescr.sizeInBytes = windowHeight*windowWidth*sizeof(float)*3;
    bufferDescr.type = IResource::Type::Buffer;
    ResultBuffer = gDevice->createBufferResource(bufferDescr);
    ResultResource = createUAV(ResultBuffer);
  ;
        //Create Unordered Acces View of Buffers in order to update them in different pipelines
   
    
    

    // Now we will use our `loadShaderProgram` function to load
    // the code from `shaders.slang` into the graphics API.
    //
    
    //RandomBlobs

    
    

     
    // Following the D3D12/Vulkan style of API, we need a pipeline state object
    // (PSO) to encapsulate the configuration of the overall graphics pipeline.
    //
    

        {
            ComPtr<IShaderProgram> shaderProgram;
            SLANG_RETURN_ON_FAIL(
                loadRenderProgram(gDevice, "ref", "fragmentMain", shaderProgram.writeRef()));
            gPipelineState = createRenderPipelineState(inputLayout, shaderProgram);
        }

        {
            ComPtr<IShaderProgram> shaderProgram;
            SLANG_RETURN_ON_FAIL(
                loadRenderProgram(gDevice, "shaders", "fragmentDiffMain", shaderProgram.writeRef()));
            gPipelineIterState = createRenderPipelineState(inputLayout, shaderProgram);
        }

        {
            ComPtr<IShaderProgram> shaderProgram;
            SLANG_RETURN_ON_FAIL(
                loadComputeProgram(gDevice, "backward", shaderProgram.writeRef()));
            gGaussianState = createComputePipelineState(shaderProgram);
        }

        {
            ComPtr<IShaderProgram> shaderProgram;
            SLANG_RETURN_ON_FAIL(
                loadComputeProgram(gDevice, "optimizer", shaderProgram.writeRef()));
            gOptimizerState = createComputePipelineState(shaderProgram);
        }

        {
            ComPtr<IShaderProgram> shaderProgram;
            SLANG_RETURN_ON_FAIL(
                loadRenderProgram(gDevice, "draw-quad", "fragmentMain", shaderProgram.writeRef()));
            gDrawQuadPipelineState = createRenderPipelineStateQuad(inputLayout, shaderProgram);
        }

    gTexView = createTextureFromFile("jardin.jpeg", textureWidth, textureHeight);
    gDepthTexture = createDepthTexture();
    gDepthTextureView = createDSV(gDepthTexture);
    gRefImage = createRenderTargetTexture(Format::R8G8B8A8_UNORM, windowWidth, windowHeight, 1);
    gRefImageRTV = createRTV(gRefImage, Format::R8G8B8A8_UNORM);
    gRefImageSRV = createSRV(gRefImage);

    gResultImage = createRenderTargetTexture(Format::R8G8B8A8_UNORM, windowWidth, windowHeight, 1);
    gResultImageRTV = createRTV(gResultImage, Format::R8G8B8A8_UNORM);
    gResultImageSRV = createSRV(gResultImage);

    gRefFrameBuffer = createRenderTargetFramebuffer(gRefImageRTV);
    gResultFrameBuffer = createRenderTargetFramebuffer(gResultImageRTV);

    {
            ComPtr<ICommandBuffer> commandBuffer = gTransientHeaps[0]->createCommandBuffer();
            auto encoder = commandBuffer->encodeResourceCommands();
            encoder->bufferBarrier(BlobsBuffer, ResourceState::Undefined, ResourceState::UnorderedAccess);
            encoder->bufferBarrier(BlobsGradientBuffer, ResourceState::Undefined, ResourceState::UnorderedAccess);
            encoder->bufferBarrier(ResultBuffer, ResourceState::Undefined, ResourceState::UnorderedAccess);
            encoder->textureBarrier(gRefImage, ResourceState::RenderTarget, ResourceState::ShaderResource);
            encoder->textureBarrier(gResultImage, ResourceState::RenderTarget, ResourceState::ShaderResource);
            encoder->endEncoding();
            commandBuffer->close();
            gQueue->executeCommandBuffer(commandBuffer);
     }


    return SLANG_OK;


}
float sigmoid(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}
void initBlobs(){
    
    for (int i = 0; i < BLOB_COUNT*8; i+=8) {
        blobs[i] = static_cast<float>(rand()) / RAND_MAX;
        blobs[i + 1] = static_cast<float>(rand()) / RAND_MAX;
    
        // Scale - directly assigning 0.005f since exp(log(0.005f)) is 0.005f
        blobs[i + 2] = 0.25f;
        blobs[i + 3] = 0.5f;
        
        // Rotation - random between 0 and 2*pi
        blobs[i + 7] = static_cast<float>(rand()) / RAND_MAX * (2 * 3.1415926);
        
        // Color - applying sigmoid to the random values between 0.25 and 0.75
        blobs[i + 4] = sigmoid(static_cast<float>(rand()) / RAND_MAX * 0.5f + 0.25f);
        blobs[i + 5] = sigmoid(static_cast<float>(rand()) / RAND_MAX * 0.5f + 0.25f);
        blobs[i + 6] = sigmoid(static_cast<float>(rand()) / RAND_MAX * 0.5f + 0.25f);
    }
    }



// With the initialization out of the way, we can now turn our attention
// to the per-frame rendering logic. As with the initialization, there is
// nothing really Slang-specific here, so the commentary doesn't need
// to be very detailed.
//


template <typename SetupPipelineFunc>
    void renderImage(
        int transientHeapIndex, IFramebuffer* fb, const SetupPipelineFunc& setupPipeline)
    {
        ComPtr<ICommandBuffer> commandBuffer =
            gTransientHeaps[transientHeapIndex]->createCommandBuffer();
        auto renderEncoder = commandBuffer->encodeRenderCommands(gRenderPass, fb);

        gfx::Viewport viewport = {};
        viewport.maxZ = 1.0f;
        viewport.extentX = (float)windowWidth;
        viewport.extentY = (float)windowHeight;
        renderEncoder->setViewportAndScissor(viewport);

        setupPipeline(renderEncoder);

        renderEncoder->setVertexBuffer(0, gVertexBuffer);
        renderEncoder->setPrimitiveTopology(PrimitiveTopology::TriangleStrip);

        
        renderEncoder->draw(4);
        renderEncoder->endEncoding();
        commandBuffer->close();
        gQueue->executeCommandBuffer(commandBuffer);
    }


    void renderReferenceImage(int transientHeapIndex)
    {
        {
            ComPtr<ICommandBuffer> commandBuffer = gTransientHeaps[transientHeapIndex]->createCommandBuffer();
            auto encoder = commandBuffer->encodeResourceCommands();
            encoder->textureBarrier(gRefImage, ResourceState::ShaderResource, ResourceState::RenderTarget);
            encoder->endEncoding();
            commandBuffer->close();
            gQueue->executeCommandBuffer(commandBuffer);
        }

        renderImage(
            transientHeapIndex,
            gRefFrameBuffer,
            [&](IRenderCommandEncoder* encoder)
            {
                auto rootObject = encoder->bindPipeline(gPipelineState);
                ShaderCursor rootCursor(rootObject);
                rootCursor["Uniforms"]["modelViewProjection"].setData(
                    &transformMatrix, sizeof(float) * 16);
                rootCursor["Uniforms"]["sampler"].setSampler(gSampler);
                rootCursor["Uniforms"]["texRef"].setResource(gTexView);
                rootCursor["Uniforms"]["x"].setData(0);
                rootCursor["Uniforms"]["y"].setData(0);
                rootCursor["Uniforms"]["width"].setData(windowWidth);
                rootCursor["Uniforms"]["height"].setData(windowHeight);
                rootCursor["Uniforms"]["viewWidth"].setData(windowWidth);
                rootCursor["Uniforms"]["viewHeight"].setData(windowHeight);
              //  rootCursor["Uniforms"]["blobs"].setResource(BlobsBufferView);
                //rootCursor["Uniforms"]["blob_count"].setData(BLOB_COUNT);
                
            });
    }

    void renderResultImage(int transientHeapIndex)
    {
        {
            ComPtr<ICommandBuffer> commandBuffer = gTransientHeaps[transientHeapIndex]->createCommandBuffer();
            auto encoder = commandBuffer->encodeResourceCommands();
            encoder->textureBarrier(gResultImage, ResourceState::ShaderResource, ResourceState::RenderTarget);
            encoder->textureBarrier(gRefImage, ResourceState::RenderTarget, ResourceState::ShaderResource);
            encoder->endEncoding();
            commandBuffer->close();
            gQueue->executeCommandBuffer(commandBuffer);
        }
        
        renderImage(
            transientHeapIndex,
            gResultFrameBuffer,
            [&](IRenderCommandEncoder* encoder)
            {
                auto rootObject = encoder->bindPipeline(gPipelineIterState);
                ShaderCursor rootCursor(rootObject);
                rootCursor["Uniforms"]["modelViewProjection"].setData(
                    &transformMatrix, sizeof(float) * 16);
                rootCursor["Uniforms"]["sampler"].setSampler(gSampler);
                rootCursor["Uniforms"]["texRef"].setResource(gTexView);
                 rootCursor["Uniforms"]["x"].setData(0);
                rootCursor["Uniforms"]["y"].setData(0);
                rootCursor["Uniforms"]["width"].setData(windowWidth);
                rootCursor["Uniforms"]["height"].setData(windowHeight);
                rootCursor["Uniforms"]["viewWidth"].setData(windowWidth);
                rootCursor["Uniforms"]["viewHeight"].setData(windowHeight);
                rootCursor["Uniforms"]["blobs"].setResource(BlobsBufferView);
                rootCursor["Uniforms"]["blob_count"].setData(BLOB_COUNT);
                rootCursor["Uniforms"]["result"].setResource(ResultResource);
               
                
            });
    }

    void drawTexturedQuad(IRenderCommandEncoder* renderEncoder, int x, int y, int w, int h, IResourceView* srv)
    {   
        gfx::Viewport viewport = {};
        viewport.maxZ = 1.0f;
        viewport.extentX = (float)windowWidth;
        viewport.extentY = (float)windowHeight;
        renderEncoder->setViewportAndScissor(viewport);

        auto root = renderEncoder->bindPipeline(gDrawQuadPipelineState);
        ShaderCursor rootCursor(root);
        rootCursor["Uniforms"]["x"].setData(x);
        rootCursor["Uniforms"]["y"].setData(y);
        rootCursor["Uniforms"]["width"].setData(w);
        rootCursor["Uniforms"]["height"].setData(h);
        rootCursor["Uniforms"]["viewWidth"].setData(windowWidth);
        rootCursor["Uniforms"]["viewHeight"].setData(windowHeight);
        rootCursor["Uniforms"]["texture"].setResource(srv);
        rootCursor["Uniforms"]["sampler"].setSampler(gSampler);
        renderEncoder->setVertexBuffer(0, gVertexBufferQuad);
        renderEncoder->setPrimitiveTopology(PrimitiveTopology::TriangleStrip);
        renderEncoder->draw(4);
    }


virtual void renderFrame(int frameBufferIndex) override
{       


    renderReferenceImage(frameBufferIndex);
    // With that, we are done drawing for one frame, and ready for the next.
    
    renderResultImage(frameBufferIndex);

    

     {
            ComPtr<ICommandBuffer> commandBuffer = gTransientHeaps[frameBufferIndex]->createCommandBuffer();
            auto encoder = commandBuffer->encodeResourceCommands();
            encoder->textureBarrier(gResultImage, ResourceState::RenderTarget, ResourceState::ShaderResource);
            encoder->endEncoding();
            commandBuffer->close();
            gQueue->executeCommandBuffer(commandBuffer);
     }
        
           
           {
                ComPtr<ICommandBuffer> commandBuffer = gTransientHeaps[frameBufferIndex]->createCommandBuffer();
                auto encoder = commandBuffer->encodeComputeCommands();
            
                auto rootObject = encoder->bindPipeline(gGaussianState);
                ShaderCursor rootCursor(rootObject);
                rootCursor["Uniforms"]["modelViewProjection"].setData(
                    &transformMatrix, sizeof(float) * 16);
                rootCursor["Uniforms"]["sampler"].setSampler(gSampler);
                rootCursor["Uniforms"]["texRef"].setResource(gTexView);
                 rootCursor["Uniforms"]["x"].setData(0);
                rootCursor["Uniforms"]["y"].setData(0);
                rootCursor["Uniforms"]["width"].setData(windowWidth);
                rootCursor["Uniforms"]["height"].setData(windowHeight);
                rootCursor["Uniforms"]["viewWidth"].setData(windowWidth);
                rootCursor["Uniforms"]["viewHeight"].setData(windowHeight);
                rootCursor["Uniforms"]["blob_count"].setData(BLOB_COUNT);
                rootCursor["Uniforms"]["result"].setResource(ResultResource);
                rootCursor["Uniforms"]["grad_blobs"].setResource(BlobsGradientBufferView);
                rootCursor["Uniforms"]["blobs"].setResource(BlobsBufferView);
                
            
            encoder->dispatchCompute(
                    windowWidth/16 + 1, windowHeight/16 + 1, 1);
            encoder->endEncoding();
            commandBuffer->close();
            gQueue->executeCommandBuffer(commandBuffer);
                
            }

    
    {
                ComPtr<ICommandBuffer> commandBuffer = gTransientHeaps[frameBufferIndex]->createCommandBuffer();
                auto encoder = commandBuffer->encodeComputeCommands();
            
                auto rootObject = encoder->bindPipeline(gOptimizerState);
                ShaderCursor rootCursor(rootObject);
                rootCursor["Uniforms"]["modelViewProjection"].setData(
                    &transformMatrix, sizeof(float) * 16);
                rootCursor["Uniforms"]["sampler"].setSampler(gSampler);
                rootCursor["Uniforms"]["texRef"].setResource(gTexView);
                 rootCursor["Uniforms"]["x"].setData(0);
                rootCursor["Uniforms"]["y"].setData(0);
                rootCursor["Uniforms"]["width"].setData(windowWidth);
                rootCursor["Uniforms"]["height"].setData(windowHeight);
                rootCursor["Uniforms"]["viewWidth"].setData(windowWidth);
                rootCursor["Uniforms"]["viewHeight"].setData(windowHeight);
                rootCursor["Uniforms"]["blobs"].setResource(BlobsBufferView);
                rootCursor["Uniforms"]["blob_count"].setData(BLOB_COUNT);
                rootCursor["Uniforms"]["result"].setResource(ResultResource);
                rootCursor["Uniforms"]["grad_blobs"].setResource(BlobsGradientBufferView);
                
            
            encoder->dispatchCompute(
                    BLOB_COUNT/256 + 1, 1, 1);
            encoder->endEncoding();
            commandBuffer->close();
            gQueue->executeCommandBuffer(commandBuffer);
                
            }
            
    







    {
        ComPtr<ICommandBuffer> commandBuffer =
            gTransientHeaps[frameBufferIndex]->createCommandBuffer();
        auto renderEncoder = commandBuffer->encodeRenderCommands(gRenderPass, gFramebuffers[frameBufferIndex]);
        
        int refImageWidth = windowWidth/2;
        int refImageHeight = refImageWidth * windowHeight / windowWidth;
        
        drawTexturedQuad(renderEncoder, 0, 0, refImageWidth, refImageHeight, gRefImageSRV);
        drawTexturedQuad(renderEncoder,refImageWidth, 0, refImageWidth, refImageHeight, gResultImageSRV);
        renderEncoder->endEncoding();
        commandBuffer->close();
        gQueue->executeCommandBuffer(commandBuffer);
    }

    

    gSwapchain->present();

   frame++;
   if(frame == 1 || frame == 40){
    gQueue->waitOnHost();
    std::vector<float> params;
    const size_t bufferSize = BlobsBuffer->getDesc()->sizeInBytes;
    ComPtr<ISlangBlob> blob;

    // Read the buffer contents into a blob
    gDevice->readBufferResource(BlobsBuffer, 0, bufferSize, blob.writeRef());
    if (!blob)
        printf("BAD");

    // Cast the blob data to the type stored in the buffer, in this case, let's assume it's float
    const float* parameters = (const float*)blob->getBufferPointer();
    size_t paramCount = bufferSize / sizeof(int);
    String var[2]= {"x", "y\n"};
    // Copy the loss values into the output vector
    params.assign(parameters, parameters + paramCount);
    printf("%d", paramCount);
     for(int k=0; k<paramCount; k=k+8){
                printf("%d posx  %f \n", k,float(params[k]));
                printf("%d posy  %f \n", k, float(params[k+1]));
                printf("%d scalex %f \n", k, float(params[k+2]));
                printf("%d scaley %f \n", k, float(params[k+3]));
                printf("%d colorr %f \n", k, float(params[k+4]));
                printf("%d colorg %f \n", k, float(params[k+5]));
                printf("%d colorb %f \n", k, float(params[k+6]));
                printf("%d rotation %f \n", k, float(params[k+7]));
    }

}   
   
}
};

// This macro instantiates an appropriate main function to
// run the application defined above.
PLATFORM_UI_MAIN(innerMain<DiffPose>)
