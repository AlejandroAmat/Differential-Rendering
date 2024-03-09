#include "examples/example-base/example-base.h"
#include "gfx-util/shader-cursor.h"
#include "slang-com-ptr.h"
#include "slang-gfx.h"
#include "source/core/slang-basic.h"
#include "tools/platform/vector-math.h"
#include "tools/platform/window.h"
#include <slang.h>

//gfx is the abstraction API library used to generalize API targets such as DIRECT,Vulkan,CUDA...
using namespace gfx;
using namespace Slang;

//Vertes struct consistion of a 3 dimension float vector
struct Vertex
{
    float position[3];
};

//Data passing to vertexshader. In this case a 4 point plane (square)
static const int kVertexCount = 4;
static const Vertex kVertexData[kVertexCount] = {
    {{0, 0, 0}},
    {{0, 1, 0}},
    {{1, 0, 0}},
    {{1, 1, 0}},
};

struct AutoDiffTexture : public WindowedAppBase
{

    List<uint32_t> mipMapOffset;
    int textureWidth;
    int textureHeight;

    //slang diagnostics to detect issues in program
    void diagnoseIfNeeded(slang::IBlob* diagnosticsBlob)
    {
        if (diagnosticsBlob != nullptr)
        {
            printf("%s", (const char*)diagnosticsBlob->getBufferPointer());
        }
    }

    //function to load rendering shaders into main program. After loading the shader program it will be stored in outProgram
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

    //This method is the same as the previous one but loads a compute shader instead so there is only an entry point (i.e compute)
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


    //We will be defining in this section the important structures needed for the program:
    // PipelineState
    //...............................................................................................................
    // These objects encapsulate the entire state of the graphics pipeline needed for rendering or compute operations.
    // Each state includes configurations like the shader programs to use, input layouts, rasterization settings, etc.
    ComPtr<gfx::IPipelineState> gRefPipelineState;
    ComPtr<gfx::IPipelineState> gIterPipelineState;
    ComPtr<gfx::IPipelineState> gReconstructPipelineState;
    ComPtr<gfx::IPipelineState> gConvertPipelineState;
    ComPtr<gfx::IPipelineState> gBuildMipPipelineState;
    ComPtr<gfx::IPipelineState> gLearnMipPipelineState;
    ComPtr<gfx::IPipelineState> gDrawQuadPipelineState;

    //ItextureResource
    //...................................................................................................................
    // Representation of a texture stored in memory.This data can represent anything from color maps (diffuse maps), height
    // maps, normal maps, to arbitrary data for compute shaders.

    //IresourceView
    //...................................................................................................................
    // Defines how a pipeline stage (such as vertex shader, pixel shader, compute shader, etc.) accesses a resource:::
    //
    //Shader Resource View (SRV): Allows a shader to read from a resource. Commonly used for sampling textures.
    //RenderTargetView (RTV): Allows a resource to be used as an output render target for drawing operations.
    //DepthStencilView (DSV): Specialized for depth and stencil testing, allowing a texture to be used as a depth-stencil buffer.
    //Unordered Access View (UAV): Provides read-write access to a resource from a shader, allowing for more complex operations like compute shaders updating a texture.

    //IBufferResource
    //...................................................................................................................
    // Buffers can store a wide array of data types, including vertices, indices, constants (uniforms), and arbitrary data for compute shaders.

    //IFramebuffer
    //...................................................................................................................
    // Collection of memory buffers that can be used as the destination for rendering output.

    //ISamplerState
    //...................................................................................................................
    // Describes how texture data should be sampled and filtered when accessed by shaders

    ComPtr<gfx::ITextureResource> gLearningTexture;
    ComPtr<gfx::IResourceView> gLearningTextureSRV;
    List<ComPtr<gfx::IResourceView>> gLearningTextureUAVs;

    ComPtr<gfx::ITextureResource> gDiffTexture;
    ComPtr<gfx::IResourceView> gDiffTextureSRV;
    List<ComPtr<gfx::IResourceView>> gDiffTextureUAVs;

    ComPtr<gfx::IBufferResource> gVertexBuffer;
    ComPtr<gfx::IResourceView> gTexView;
    ComPtr<gfx::ISamplerState> gSampler;
    ComPtr<gfx::IFramebuffer> gRefFrameBuffer;
    ComPtr<gfx::IFramebuffer> gIterFrameBuffer;

    ComPtr<gfx::ITextureResource> gDepthTexture;
    ComPtr<gfx::IResourceView> gDepthTextureView;

    ComPtr<gfx::IResourceView> gIterImageDSV;

    ComPtr<gfx::ITextureResource> gIterImage;
    ComPtr<gfx::IResourceView> gIterImageSRV;
    ComPtr<gfx::IResourceView> gIterImageRTV;

    ComPtr<gfx::ITextureResource> gRefImage;
    ComPtr<gfx::IResourceView> gRefImageSRV;
    ComPtr<gfx::IResourceView> gRefImageRTV;

    ComPtr<gfx::IBufferResource> gAccumulateBuffer;
    ComPtr<gfx::IBufferResource> gReconstructBuffer;
    ComPtr<gfx::IResourceView> gAccumulateBufferView;
    ComPtr<gfx::IResourceView> gReconstructBufferView;

    ClearValue kClearValue;   // Color value used to restore the learnt Texture UAV
    bool resetLearntTexture = false;

    //method that creates a RenderTargetTexture with all the possible states and corresponding size and mip map levels
    ComPtr<gfx::ITextureResource> createRenderTargetTexture(gfx::Format format, int w, int h, int levels)
    {
        gfx::ITextureResource::Desc textureDesc = {};
        textureDesc.allowedStates.add(ResourceState::ShaderResource);
        textureDesc.allowedStates.add(ResourceState::UnorderedAccess);
        textureDesc.allowedStates.add(ResourceState::RenderTarget);
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
        desc.rasterizer.cullMode = gfx::CullMode::None;
        desc.framebufferLayout = gFramebufferLayout;
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
    


    
    Slang::Result initialize()
    {
        initializeBase("autodiff-texture", 1024, 768);
        srand(20421);

        freopen("output.txt", "a", stdout);
        
        //reset learning. Sets learnt texture to clearvalue.
        gWindow->events.keyPress = [this](platform::KeyEventArgs& e)
        {
            if (e.keyChar == 'R' || e.keyChar == 'r')
                resetLearntTexture = true;
        };

        //defines values for clear components.
        kClearValue.color.floatValues[0] = 0.3f;
        kClearValue.color.floatValues[1] = 0.5f;
        kClearValue.color.floatValues[2] = 0.7f;
        kClearValue.color.floatValues[3] = 1.0f;

        auto clientRect = gWindow->getClientRect();
        windowWidth = clientRect.width;
        windowHeight = clientRect.height;

        //define the input layout so the rendering pipelines now how to expect vertices position.
        InputElementDesc inputElements[] = {
            {"POSITION", 0, Format::R32G32B32_FLOAT, offsetof(Vertex, position)}};
        auto inputLayout = gDevice->createInputLayout(sizeof(Vertex), &inputElements[0], 1);
        if (!inputLayout)
            return SLANG_FAIL;

        //Buffer to store the vertices ofthe plane--gVertexBuffer
        IBufferResource::Desc vertexBufferDesc;
        vertexBufferDesc.type = IResource::Type::Buffer;
        vertexBufferDesc.sizeInBytes = kVertexCount * sizeof(Vertex);
        vertexBufferDesc.defaultState = ResourceState::VertexBuffer;
        gVertexBuffer = gDevice->createBufferResource(vertexBufferDesc, &kVertexData[0]);
        if (!gVertexBuffer)
            return SLANG_FAIL;


        //load all shaderPrograms and link them to a pipeline.
        //......................................................................................................
        //gRefPipeline::To render Reference Image. Vertex with random modelviewmatrix
        //gIterPipeline:: Render Loss Image & computes forward&Backward Gradients. Vertex with random modelviewmatrix
        //gDrawQuadPipeline:: Render learntTexture and the other two (ref & Iter Textures)
        //
        //
        //
        //gLearnMipPipelineState:: updates LearnTexture with DiffTexture (Gradients for all MiPMap levels) for all Mip Map Level
        //
        {
            ComPtr<IShaderProgram> shaderProgram;
            SLANG_RETURN_ON_FAIL(
                loadRenderProgram(gDevice, "train", "fragmentMain", shaderProgram.writeRef()));
            gRefPipelineState = createRenderPipelineState(inputLayout, shaderProgram);
        }
        {
            ComPtr<IShaderProgram> shaderProgram;
            SLANG_RETURN_ON_FAIL(
                loadRenderProgram(gDevice, "train", "diffFragmentMain", shaderProgram.writeRef()));
            gIterPipelineState = createRenderPipelineState(inputLayout, shaderProgram);
        }
        {
            ComPtr<IShaderProgram> shaderProgram;
            SLANG_RETURN_ON_FAIL(
                loadRenderProgram(gDevice, "draw-quad", "fragmentMain", shaderProgram.writeRef()));
            gDrawQuadPipelineState = createRenderPipelineState(inputLayout, shaderProgram);
        }
        {
            ComPtr<IShaderProgram> shaderProgram;
            SLANG_RETURN_ON_FAIL(
                loadComputeProgram(gDevice, "reconstruct", shaderProgram.writeRef()));
            gReconstructPipelineState = createComputePipelineState(shaderProgram);
        }
        {
            ComPtr<IShaderProgram> shaderProgram;
            SLANG_RETURN_ON_FAIL(loadComputeProgram(gDevice, "convert", shaderProgram.writeRef()));
            gConvertPipelineState = createComputePipelineState(shaderProgram);
        }
        {
            ComPtr<IShaderProgram> shaderProgram;
            SLANG_RETURN_ON_FAIL(loadComputeProgram(gDevice, "buildmip", shaderProgram.writeRef()));
            gBuildMipPipelineState = createComputePipelineState(shaderProgram);
        }
        {
            ComPtr<IShaderProgram> shaderProgram;
            SLANG_RETURN_ON_FAIL(loadComputeProgram(gDevice, "learnmip", shaderProgram.writeRef()));
            gLearnMipPipelineState = createComputePipelineState(shaderProgram);
        }

        //create the Reference Texture View and create MipMapOffsets (Check function)
        gTexView = createTextureFromFile("op.jpg", textureWidth, textureHeight);
        initMipOffsets(textureWidth, textureHeight);


        //create Accumulate and Reconstruct Buffers. The Accumulate buffer is
        //in charge of storing Gradients in backprogation of gIterPipeline (linked to train.slang).
        //It gets refreshed for every frame. 
        //Reconstruction Buffer::
        gfx::IBufferResource::Desc bufferDesc = {};
        bufferDesc.allowedStates.add(ResourceState::ShaderResource);
        bufferDesc.allowedStates.add(ResourceState::UnorderedAccess);
        bufferDesc.allowedStates.add(ResourceState::General);
        bufferDesc.sizeInBytes = mipMapOffset.getLast() * sizeof(uint32_t);
        bufferDesc.type = IResource::Type::Buffer;
        gAccumulateBuffer = gDevice->createBufferResource(bufferDesc);
        gReconstructBuffer = gDevice->createBufferResource(bufferDesc);

        //Create Unordered Acces View of Buffers in order to update them in different pipelines
        gAccumulateBufferView = createUAV(gAccumulateBuffer);
        gReconstructBufferView = createUAV(gReconstructBuffer);

        //create Learning and DiffTexture. Learning TExture is the texture that will be updated 
        //every frame and thaat we try to estimate. DiffTExture is an auxiliary Texture that stores 
        //The differential gradients through propagation in MIPS.
        int mipCount = 1 + Math::Log2Ceil(Math::Max(textureWidth, textureHeight));
        gLearningTexture = createRenderTargetTexture(
            Format::R32G32B32A32_FLOAT,
            textureWidth,
            textureHeight,
            mipCount);
        //SRV in order to access resource from Shaders
        gLearningTextureSRV = createSRV(gLearningTexture);
        //Create UAV for every MIP level in order to write and update texture for all mip levels through 
        //propagation pipelines
        for (int i = 0; i < mipCount; i++)
            gLearningTextureUAVs.add(createUAV(gLearningTexture, i));

        //idem
        gDiffTexture = createRenderTargetTexture(
            Format::R32G32B32A32_FLOAT,
            textureWidth,
            textureHeight,
            mipCount);
        gDiffTextureSRV = createSRV(gDiffTexture);
        for (int i = 0; i < mipCount; i++)
            gDiffTextureUAVs.add(createUAV(gDiffTexture, i));


        gfx::ISamplerState::Desc samplerDesc = {};
        //samplerDesc.maxLOD = 0.0f;
        gSampler = gDevice->createSamplerState(samplerDesc);

        gDepthTexture = createDepthTexture();
        gDepthTextureView = createDSV(gDepthTexture);

        //Create Render Targets and RTV (Render Target) ans SRV (Shader Resource) of Reference and Iteration Textures + create the
        //Frame buffers for their render encoders.
        gRefImage = createRenderTargetTexture(Format::R8G8B8A8_UNORM, windowWidth, windowHeight, 1);
        gRefImageRTV = createRTV(gRefImage, Format::R8G8B8A8_UNORM);
        gRefImageSRV = createSRV(gRefImage);

        gIterImage = createRenderTargetTexture(Format::R8G8B8A8_UNORM, windowWidth, windowHeight, 1);
        gIterImageRTV = createRTV(gIterImage, Format::R8G8B8A8_UNORM);
        gIterImageSRV = createSRV(gIterImage);

        gRefFrameBuffer = createRenderTargetFramebuffer(gRefImageRTV);
        gIterFrameBuffer = createRenderTargetFramebuffer(gIterImageRTV);

        //Set Textures to initial State and clear learning and diff.

        {
            ComPtr<ICommandBuffer> commandBuffer = gTransientHeaps[0]->createCommandBuffer();
            auto encoder = commandBuffer->encodeResourceCommands();
            encoder->textureBarrier(gLearningTexture, ResourceState::RenderTarget, ResourceState::UnorderedAccess);
            encoder->textureBarrier(gDiffTexture, ResourceState::RenderTarget, ResourceState::UnorderedAccess);
            encoder->textureBarrier(gRefImage, ResourceState::RenderTarget, ResourceState::ShaderResource);
            encoder->textureBarrier(gIterImage, ResourceState::RenderTarget, ResourceState::ShaderResource);
            for (int i = 0; i < gLearningTextureUAVs.getCount(); i++)
            {
                ClearValue clearValue = {};
                encoder->clearResourceView(gLearningTextureUAVs[i], &clearValue, ClearResourceViewFlags::None);
                encoder->clearResourceView(gDiffTextureUAVs[i], &clearValue, ClearResourceViewFlags::None);
            }
            encoder->textureBarrier(gLearningTexture, ResourceState::UnorderedAccess, ResourceState::ShaderResource);

            encoder->endEncoding();
            commandBuffer->close();
            gQueue->executeCommandBuffer(commandBuffer);
        }

        return SLANG_OK;
    }

    //Sets the offsets for the MipMaps List. The Mip levels is defined by the log2 of thre maximum (w,h)
    //i.e wxh = 512x512 ::: levels = 9 + 1 (512x512 txt). Level0 represents the 512x512 texture.
    //for each level the texture is divided by 4 (2x2) and the offset is stored :: lw x lh x sizeof(int)
    void initMipOffsets(int w, int h)
    {
        int layers = 1 + Math::Log2Ceil(Math::Max(w, h));
        uint32_t offset = 0;
        for (int i = 0; i < layers; i++)
        {
            auto lw = Math::Max(1, w >> i);
            auto lh = Math::Max(1, h >> i);
            printf("%i \n",  offset);
            mipMapOffset.add(offset);
            offset += lw * lh * 4;
            
        }
        printf("%i \n",  offset);
        mipMapOffset.add(offset);

        printf("%i \n",  sizeof(offset));
       
    }

    //returns the trandofrm matrix that will be used in vertex shader for both reference 
    //object and loss. 
    glm::mat4x4 getTransformMatrix()
    {
        float rotX = (rand() / (float)RAND_MAX) * 0.3f;
        float rotY = (rand() / (float)RAND_MAX) * 0.2f;
        glm::mat4x4 matProj = glm::perspectiveRH_ZO(
            glm::radians(60.0f), (float)windowWidth / (float)windowHeight, 0.1f, 1000.0f);
        auto identity = glm::mat4(1.0f);
        auto translate = glm::translate(
            identity,
            glm::vec3(
                -0.6f + 0.2f * (rand() / (float)RAND_MAX),
                -0.6f + 0.2f * (rand() / (float)RAND_MAX),
                -1.0f));
        auto rot = glm::rotate(translate, -glm::pi<float>() * rotX, glm::vec3(1.0f, 0.0f, 0.0f));
        rot = glm::rotate(rot, -glm::pi<float>() * rotY, glm::vec3(0.0f, 1.0f, 0.0f));
        auto transformMatrix = matProj * rot;
        transformMatrix = glm::transpose(transformMatrix);
        return transformMatrix;
       
    }


    //Defines function for rendering Image with template. Creates render encoder with FrameBuffer with the according
    //Index (passed in render frame). Sets viewport, vertexBuffer and Topology and draws.
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

    //Method to render the reference Texture. Sets RefImage to RenderTarget 
    //(this will write to RTV & the gRefFrameBuffer passed in renderImage)
    //Note that in order to modify RecourceStates an ecnoder has to be defined.
    //Then a call to render Image is performed with 
    //referenceFrameBuffer and the frameIndex. Apart from the methiod defined 
    //functionality, the Uniforms are passed and the encoder is binded to the pipeline.
    // Note gTextView is passed as reference Texture. This pipeline uses
    //the train.slang fragmentMain shader that simply samples the reference texture.
    void renderReferenceImage(int transientHeapIndex, glm::mat4x4 transformMatrix)
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
                auto rootObject = encoder->bindPipeline(gRefPipelineState);
                ShaderCursor rootCursor(rootObject);
                rootCursor["Uniforms"]["modelViewProjection"].setData(
                    &transformMatrix, sizeof(float) * 16);
                rootCursor["Uniforms"]["bwdTexture"]["texture"].setResource(gTexView);
                rootCursor["Uniforms"]["sampler"].setSampler(gSampler);
                rootCursor["Uniforms"]["mipOffset"].setData(mipMapOffset.getBuffer(), sizeof(uint32_t) * mipMapOffset.getCount());
                rootCursor["Uniforms"]["texRef"].setResource(gTexView);
                rootCursor["Uniforms"]["bwdTexture"]["accumulateBuffer"].setResource(gAccumulateBufferView);
            });
    }

    //Render Frame method performed for each frame. the frameBufferIndex contains

    virtual void renderFrame(int frameBufferIndex) override
    {
        static uint32_t frameCount = 0;
        frameCount++;
        auto transformMatrix = getTransformMatrix();
        renderReferenceImage(frameBufferIndex, transformMatrix);
        
        
        // Barriers.
        {
            ComPtr<ICommandBuffer> commandBuffer =
                gTransientHeaps[frameBufferIndex]->createCommandBuffer();
            auto resEncoder = commandBuffer->encodeResourceCommands();
            ClearValue clearValue = {};
            resEncoder->bufferBarrier(gAccumulateBuffer, ResourceState::Undefined, ResourceState::UnorderedAccess);
            resEncoder->bufferBarrier(gReconstructBuffer, ResourceState::Undefined, ResourceState::UnorderedAccess);
            resEncoder->textureBarrier(gRefImage, ResourceState::Present, ResourceState::ShaderResource);
            resEncoder->textureBarrier(gIterImage, ResourceState::ShaderResource, ResourceState::RenderTarget);
            resEncoder->clearResourceView(gAccumulateBufferView, &clearValue, ClearResourceViewFlags::None);
            resEncoder->clearResourceView(gReconstructBufferView, &clearValue, ClearResourceViewFlags::None);
            if (resetLearntTexture)
            {
                resEncoder->textureBarrier(gLearningTexture, ResourceState::ShaderResource, ResourceState::UnorderedAccess);
                for (Index i =0; i <gLearningTextureUAVs.getCount(); i++)
                    resEncoder->clearResourceView(gLearningTextureUAVs[i], &clearValue, ClearResourceViewFlags::None);
                resEncoder->textureBarrier(gLearningTexture, ResourceState::UnorderedAccess, ResourceState::ShaderResource);
                resetLearntTexture = false;
            }
            resEncoder->endEncoding();
            commandBuffer->close();
            gQueue->executeCommandBuffer(commandBuffer);
        }

        // Render image using backward propagate shader to obtain texture-space gradients.
        renderImage(
            frameBufferIndex,
            gIterFrameBuffer,
            [&](IRenderCommandEncoder* encoder)
            {
                auto rootObject = encoder->bindPipeline(gIterPipelineState);
                ShaderCursor rootCursor(rootObject);
                
                rootCursor["Uniforms"]["modelViewProjection"].setData(
                    &transformMatrix, sizeof(float) * 16);
                rootCursor["Uniforms"]["bwdTexture"]["texture"].setResource(gLearningTextureSRV);
                rootCursor["Uniforms"]["sampler"].setSampler(gSampler);
                rootCursor["Uniforms"]["mipOffset"].setData(mipMapOffset.getBuffer(), sizeof(uint32_t) * mipMapOffset.getCount());
                rootCursor["Uniforms"]["texRef"].setResource(gRefImageSRV);
                rootCursor["Uniforms"]["bwdTexture"]["accumulateBuffer"].setResource(gAccumulateBufferView);
                rootCursor["Uniforms"]["bwdTexture"]["minLOD"].setData(5.0);
                
            });

        // Propagete gradients through mip map layers from top (lowest res) to bottom (highest res).
        {
            ComPtr<ICommandBuffer> commandBuffer =
                gTransientHeaps[frameBufferIndex]->createCommandBuffer();
            auto encoder = commandBuffer->encodeComputeCommands();
            encoder->textureBarrier(gLearningTexture, ResourceState::ShaderResource, ResourceState::UnorderedAccess);
            auto rootObject = encoder->bindPipeline(gReconstructPipelineState);
            for (int i = (int)mipMapOffset.getCount() - 2; i >= 0; i--)
            {
                ShaderCursor rootCursor(rootObject);
                rootCursor["Uniforms"]["mipOffset"].setData(mipMapOffset.getBuffer(), sizeof(uint32_t) * mipMapOffset.getCount());
                rootCursor["Uniforms"]["dstLayer"].setData(i);
                rootCursor["Uniforms"]["layerCount"].setData(mipMapOffset.getCount() - 1);
                rootCursor["Uniforms"]["width"].setData(textureWidth);
                rootCursor["Uniforms"]["height"].setData(textureHeight);
                rootCursor["Uniforms"]["accumulateBuffer"].setResource(gAccumulateBufferView);
                rootCursor["Uniforms"]["dstBuffer"].setResource(gReconstructBufferView);
                encoder->dispatchCompute(
                    ((textureWidth >> i) + 15) / 16, ((textureHeight >> i) + 15) / 16, 1);
                encoder->bufferBarrier(gReconstructBuffer, ResourceState::UnorderedAccess, ResourceState::UnorderedAccess);
            }

            // Convert bottom layer mip from buffer to texture.
            rootObject = encoder->bindPipeline(gConvertPipelineState);
            ShaderCursor rootCursor(rootObject);
            rootCursor["Uniforms"]["mipOffset"].setData(mipMapOffset.getBuffer(), sizeof(uint32_t) * mipMapOffset.getCount());
            rootCursor["Uniforms"]["dstLayer"].setData(0);
            rootCursor["Uniforms"]["width"].setData(textureWidth);
            rootCursor["Uniforms"]["height"].setData(textureHeight);
            rootCursor["Uniforms"]["srcBuffer"].setResource(gReconstructBufferView);
            rootCursor["Uniforms"]["dstTexture"].setResource(gDiffTextureUAVs[0]);
            encoder->dispatchCompute(
                (textureWidth + 15) / 16, (textureHeight + 15) / 16, 1);
            encoder->textureBarrier(gDiffTexture, ResourceState::UnorderedAccess, ResourceState::UnorderedAccess);

            // Build higher level mip map layers.
            rootObject = encoder->bindPipeline(gBuildMipPipelineState);
            for (int i = 1; i < (int)mipMapOffset.getCount() - 1; i++)
            {
                ShaderCursor rootCursor(rootObject);
                rootCursor["Uniforms"]["dstWidth"].setData(textureWidth >> i);
                rootCursor["Uniforms"]["dstHeight"].setData(textureHeight >> i);
                rootCursor["Uniforms"]["srcTexture"].setResource(gDiffTextureUAVs[i-1]);
                rootCursor["Uniforms"]["dstTexture"].setResource(gDiffTextureUAVs[i]);
                encoder->dispatchCompute(
                    ((textureWidth >> i) + 15) / 16, ((textureHeight >> i) + 15) / 16, 1);
                encoder->textureBarrier(gDiffTexture, ResourceState::UnorderedAccess, ResourceState::UnorderedAccess);
            }

            // Accumulate gradients to learnt texture.
            rootObject = encoder->bindPipeline(gLearnMipPipelineState);
            for (int i = 0; i < (int)mipMapOffset.getCount() - 1; i++)
            {
                ShaderCursor rootCursor(rootObject);
                rootCursor["Uniforms"]["dstWidth"].setData(textureWidth >> i);
                rootCursor["Uniforms"]["dstHeight"].setData(textureHeight >> i);
                rootCursor["Uniforms"]["learningRate"].setData(0.1f);
                rootCursor["Uniforms"]["srcTexture"].setResource(gDiffTextureUAVs[i]);
                rootCursor["Uniforms"]["dstTexture"].setResource(gLearningTextureUAVs[i]);
                encoder->dispatchCompute(
                    ((textureWidth >> i) + 15) / 16, ((textureHeight >> i) + 15) / 16, 1);
            }
            encoder->textureBarrier(gLearningTexture, ResourceState::UnorderedAccess, ResourceState::ShaderResource);
            encoder->textureBarrier(gIterImage, ResourceState::Present, ResourceState::ShaderResource);

            encoder->endEncoding();
            commandBuffer->close();
            gQueue->executeCommandBuffer(commandBuffer);
        }
        
        // Draw currently learnt texture.
        {
            ComPtr<ICommandBuffer> commandBuffer =
                gTransientHeaps[frameBufferIndex]->createCommandBuffer();
            auto renderEncoder = commandBuffer->encodeRenderCommands(gRenderPass, gFramebuffers[frameBufferIndex]);
            drawTexturedQuad(renderEncoder, 0, 0, textureWidth, textureHeight, gLearningTextureSRV);
            int refImageWidth = windowWidth - textureWidth - 10;
            int refImageHeight = refImageWidth * windowHeight / windowWidth;

            drawTexturedQuad(renderEncoder, textureWidth + 10, 0, refImageWidth, refImageHeight, gRefImageSRV);
            drawTexturedQuad(renderEncoder, textureWidth + 10, refImageHeight + 10, refImageWidth, refImageHeight, gIterImageSRV);
            renderEncoder->endEncoding();
            commandBuffer->close();
            gQueue->executeCommandBuffer(commandBuffer);
        }

        gSwapchain->present();
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
        renderEncoder->setVertexBuffer(0, gVertexBuffer);
        renderEncoder->setPrimitiveTopology(PrimitiveTopology::TriangleStrip);
        renderEncoder->draw(4);
    }

};

PLATFORM_UI_MAIN(innerMain<AutoDiffTexture>)
