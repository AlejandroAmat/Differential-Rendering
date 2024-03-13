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

double const M_PI = 3.14159;


int stack = 50;
int sector = 50;
float sectorStep = 2 * M_PI / sector;
float stackStep = M_PI / stack;
float sectorAngle, stackAngle;

// For the purposes of a small example, we will define the vertex data for a
// single triangle directly in the source file. It should be easy to extend
// this example to load data from an external source, if desired.
//
//Vertes struct consistion of a 3 dimension float vector
struct Vertex
{
    float position[3];
    float uv[2];
};

static const int kVertexCount = 15000;
static Vertex kVertexData[kVertexCount];

// The example application will be implemented as a `struct`, so that
// we can scope the resources it allocates without using global variables.
//
struct HelloWorld : public WindowedAppBase
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
gfx::Result loadShaderProgram(
    gfx::IDevice*         device,
    gfx::IShaderProgram**   outProgram)
{
    // We need to obatin a compilation session (`slang::ISession`) that will provide
    // a scope to all the compilation and loading of code we do.
    //
    // Our example application uses the `gfx` graphics API abstraction layer, which already
    // creates a Slang compilation session for us, so we just grab and use it here.
    ComPtr<slang::ISession> slangSession;
    slangSession = device->getSlangSession();

    // We can now start loading code into the slang session.
    //
    // The simplest way to load code is by calling `loadModule` with the name of a Slang
    // module. A call to `loadModule("MyStuff")` will behave more or less as if you
    // wrote:
    //
    //      import MyStuff;
    //
    // In a Slang shader file. The compiler will use its search paths to try to locate
    // `MyModule.slang`, then compile and load that file. If a matching module had
    // already been loaded previously, that would be used directly.
    //
    ComPtr<slang::IBlob> diagnosticsBlob;
    slang::IModule* module = slangSession->loadModule("shaders", diagnosticsBlob.writeRef());
    diagnoseIfNeeded(diagnosticsBlob);
    if(!module)
        return SLANG_FAIL;

    // Loading the `shaders` module will compile and check all the shader code in it,
    // including the shader entry points we want to use. Now that the module is loaded
    // we can look up those entry points by name.
    //
    // Note: If you are using this `loadModule` approach to load your shader code it is
    // important to tag your entry point functions with the `[shader("...")]` attribute
    // (e.g., `[shader("vertex")] void vertexMain(...)`). Without that information there
    // is no umambiguous way for the compiler to know which functions represent entry
    // points when it parses your code via `loadModule()`.
    //
    ComPtr<slang::IEntryPoint> vertexEntryPoint;
    SLANG_RETURN_ON_FAIL(module->findEntryPointByName("vertexMain", vertexEntryPoint.writeRef()));
    //
    ComPtr<slang::IEntryPoint> fragmentEntryPoint;
    SLANG_RETURN_ON_FAIL(module->findEntryPointByName("fragmentMain", fragmentEntryPoint.writeRef()));

    // At this point we have a few different Slang API objects that represent
    // pieces of our code: `module`, `vertexEntryPoint`, and `fragmentEntryPoint`.
    //
    // A single Slang module could contain many different entry points (e.g.,
    // four vertex entry points, three fragment entry points, and two compute
    // shaders), and before we try to generate output code for our target API
    // we need to identify which entry points we plan to use together.
    //
    // Modules and entry points are both examples of *component types* in the
    // Slang API. The API also provides a way to build a *composite* out of
    // other pieces, and that is what we are going to do with our module
    // and entry points.
    //
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
    //
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
ComPtr<gfx::IPipelineState> gPipelineState;
ComPtr<gfx::IBufferResource> gVertexBuffer;

ComPtr<gfx::IResourceView> gTexView;
ComPtr<gfx::ISamplerState> gSampler;



List<uint32_t> mipMapOffset;
    int textureWidth;
    int textureHeight;

// Now that we've covered the function that actually loads and
// compiles our Slang shade code, we can go through the rest
// of the application code without as much commentary.
//


Slang::Result initialize()
{
    // Create a window for our application to render into.
    //
   



    freopen("output.txt", "a", stdout);
    initializeBase("hello-world", 1024, 768);
     gWindow->events.mouseMove = [this](const platform::MouseEventArgs& e) { onMouseMove(e); };
    gWindow->events.mouseUp = [this](const platform::MouseEventArgs& e) { onMouseUp(e); };
    gWindow->events.mouseDown = [this](const platform::MouseEventArgs& e) { onMouseDown(e); };
    gWindow->events.keyDown = [this](const platform::KeyEventArgs& e) { onKeyDown(e); };
    gWindow->events.keyUp = [this](const platform::KeyEventArgs& e) { onKeyUp(e); };
    // We will create objects needed to configur the "input assembler"
    // (IA) stage of the D3D pipeline.
    //
    // First, we create an input layout:
    //
    InputElementDesc inputElements[] = {
        { "POSITION", 0, Format::R32G32B32_FLOAT, offsetof(Vertex, position) },
        { "UV", 0, Format::R32G32B32_FLOAT, offsetof(Vertex, uv) }
    };
    auto inputLayout = gDevice->createInputLayout(
        sizeof(Vertex),
        &inputElements[0],
        2);
    if(!inputLayout) return SLANG_FAIL;

     gfx::ISamplerState::Desc samplerDesc = {};
    samplerDesc.addressU = gfx::TextureAddressingMode::ClampToEdge;
    samplerDesc.addressV = gfx::TextureAddressingMode::ClampToEdge;
    samplerDesc.addressW = gfx::TextureAddressingMode::ClampToEdge;
    gSampler = gDevice->createSamplerState(samplerDesc);
    
    gTexView = createTextureFromFile("ee.jpg", textureWidth, textureHeight);
    //initMipOffsets(textureWidth, textureHeight);
    // Next we allocate a vertex buffer for our pre-initialized
    // vertex data.
    //
    float radius = 0.5;
        float x,y,z,xy;
        static Vertex vertices[2601];
        // Generate vertices
        int p=0;
        for(int i = 0; i <= stack; ++i)
        {
            stackAngle = M_PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
            xy = radius * cosf(stackAngle);             // r * cos(u)
            z = radius * sinf(stackAngle);              // r * sin(u)
            float v = (float)i / stack;
            // add (sectorCount+1) vertices per stack
            // first and last vertices have same position and normal, but different tex coords
            for(int j = 0; j <= sector; ++j)
            {
                sectorAngle = j * sectorStep;           // starting from 0 to 2pi
                float u = (float)j / sector;
                // vertex position (x, y, z)
                x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
                y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)

                vertices[p].position[0] = x;
                vertices[p].position[1]  = y;
                vertices[p].position[2] = z;
                vertices[p].uv[0] = u;
                vertices[p].uv[1] = v;
                p++;

                if(u>1 || v>1) printf ("se pasa po ueon.");
                
            }
         }
        int k = 0;
        ;
        for(int i = 0; i < stack; ++i) {
            for(int j = 0; j < sector; ++j) {
                int first = (i * (sector + 1)) + j;
                int second = first + sector + 1;

                
                // Add indices for the first triangle: first, second, second + 1
                kVertexData[k].position[0]= vertices[first].position[0];
                kVertexData[k].position[1]= vertices[first].position[1];
                kVertexData[k].position[2]= vertices[first].position[2];

                kVertexData[k].uv[0]= vertices[first].uv[0];
                kVertexData[k].uv[1]= vertices[first].uv[1];
                k++;


                kVertexData[k].position[0]= vertices[second].position[0];
                kVertexData[k].position[1]= vertices[second].position[1];
                kVertexData[k].position[2]= vertices[second].position[2];

                kVertexData[k].uv[0]= vertices[second].uv[0];
                kVertexData[k].uv[1]= vertices[second].uv[1];
                k++;


                kVertexData[k].position[0]= vertices[second+1].position[0];
                kVertexData[k].position[1]= vertices[second+1].position[1];
                kVertexData[k].position[2]= vertices[second+1].position[2];

                kVertexData[k].uv[0]= vertices[second+1].uv[0];
                kVertexData[k].uv[1]= vertices[second+1].uv[1];
                k++;

               
                
                kVertexData[k].position[0]= vertices[first].position[0];
                kVertexData[k].position[1]= vertices[first].position[1];
                kVertexData[k].position[2]= vertices[first].position[2];

                kVertexData[k].uv[0]= vertices[first].uv[0];
                kVertexData[k].uv[1]= vertices[first].uv[1];
                k++;
                

                kVertexData[k].position[0]= vertices[second+1].position[0];
                kVertexData[k].position[1]= vertices[second+1].position[1];
                kVertexData[k].position[2]= vertices[second+1].position[2];

                kVertexData[k].uv[0]= vertices[second+1].uv[0];
                kVertexData[k].uv[1]= vertices[second+1].uv[1];
                k++;


                kVertexData[k].position[0]= vertices[first+1].position[0];
                kVertexData[k].position[1]= vertices[first+1].position[1];
                kVertexData[k].position[2]= vertices[first+1].position[2];
                
                kVertexData[k].uv[0]= vertices[first+1].uv[0];
                kVertexData[k].uv[1]= vertices[first+1].uv[1];
                k++;

            }
        }

    IBufferResource::Desc vertexBufferDesc;
    vertexBufferDesc.type = IResource::Type::Buffer;
    vertexBufferDesc.sizeInBytes = kVertexCount * sizeof(Vertex);
    vertexBufferDesc.defaultState = ResourceState::VertexBuffer;
    gVertexBuffer = gDevice->createBufferResource(vertexBufferDesc, &kVertexData[0]);
    if(!gVertexBuffer) return SLANG_FAIL;

    // Now we will use our `loadShaderProgram` function to load
    // the code from `shaders.slang` into the graphics API.
    //
    ComPtr<IShaderProgram> shaderProgram;
    SLANG_RETURN_ON_FAIL(loadShaderProgram(gDevice, shaderProgram.writeRef()));

    // Following the D3D12/Vulkan style of API, we need a pipeline state object
    // (PSO) to encapsulate the configuration of the overall graphics pipeline.
    //
    GraphicsPipelineStateDesc desc;
    desc.inputLayout = inputLayout;
    desc.program = shaderProgram;
    desc.framebufferLayout = gFramebufferLayout;
    //desc.primitiveType = PrimitiveType::Triangle;
    desc.depthStencil.depthFunc = ComparisonFunc::Less;
    desc.depthStencil.depthTestEnable = true;
    auto pipelineState = gDevice->createGraphicsPipelineState(desc);
    if (!pipelineState)
        return SLANG_FAIL;

    gPipelineState = pipelineState;

    return SLANG_OK;
}

void initMipOffsets(int w, int h)
    {
        int layers = 1 + Math::Log2Ceil(Math::Max(w, h));
        uint32_t offset = 0;
        for (int i = 0; i < layers; i++)
        {
            auto lw = Math::Max(1, w >> i);
            auto lh = Math::Max(1, h >> i);
            ;
            mipMapOffset.add(offset);
            offset += lw * lh * 4;
            
        }
       
        mipMapOffset.add(offset);

       
       
    }

    glm::mat4x4 getTransformMatrix()
    {
        float rotX = (rand() / (float)RAND_MAX) * 2.0f;
        float rotY = (rand() / (float)RAND_MAX) * 1.0f;
        glm::mat4x4 matProj = glm::perspectiveRH_ZO(
            glm::radians(60.0f), (float)windowWidth / (float)windowHeight, 0.1f, 1000.0f);
        auto identity = glm::mat4(1.0f);
        auto translate = glm::translate(
            identity,
            glm::vec3(
                -0.6f + 0.2f * (rand() / (float)RAND_MAX),
                -0.6f + 0.2f * (rand() / (float)RAND_MAX),
                -4.0f));
        auto rot = glm::rotate(translate, -glm::pi<float>() * rotX, glm::vec3(1.0f, 0.0f, 0.0f));
        rot = glm::rotate(rot, -glm::pi<float>() * rotY, glm::vec3(0.0f, 1.0f, 0.0f));
        auto transformMatrix = matProj * rot;
        transformMatrix = glm::transpose(transformMatrix);
        return transformMatrix;
       
    }


glm::vec3 cameraPosition = glm::vec3(1.75, 1.25, 5);
glm::quat cameraOrientation = glm::quat(1, glm::vec3(0));

float translationScale = 0.5f;
float rotationScale = 0.025f;

// In order to control camera movement, we will
// use good old WASD
bool wPressed = false;
bool aPressed = false;
bool sPressed = false;
bool dPressed = false;

bool isMouseDown = false;
float lastMouseX = 0.0f;
float lastMouseY = 0.0f;

void setKeyState(platform::KeyCode key, bool state)
{
    switch (key)
    {
    default:
        break;
    case platform::KeyCode::W:
        wPressed = state;
        break;
    case platform::KeyCode::A:
        aPressed = state;
        break;
    case platform::KeyCode::S:
        sPressed = state;
        break;
    case platform::KeyCode::D:
        dPressed = state;
        break;
    }
}
void onKeyDown(platform::KeyEventArgs args) { setKeyState(args.key, true); }
void onKeyUp(platform::KeyEventArgs args) { setKeyState(args.key, false); }

void onMouseDown(platform::MouseEventArgs args)
{
    isMouseDown = true;
    lastMouseX = (float)args.x;
    lastMouseY = (float)args.y;
}

void onMouseMove(platform::MouseEventArgs args)
{
    if (isMouseDown)
    {
        float deltaX = args.x - lastMouseX;
        float deltaY = args.y - lastMouseY;

        cameraOrientation =
            glm::rotate(cameraOrientation, -deltaX * rotationScale, glm::vec3(0, 1, 0));
        cameraOrientation =
            glm::rotate(cameraOrientation, -deltaY * rotationScale, glm::vec3(1, 0, 0));

        cameraOrientation = normalize(cameraOrientation);

        lastMouseX = (float)args.x;
        lastMouseY = (float)args.y;
    }
}
void onMouseUp(platform::MouseEventArgs args)
{
    isMouseDown = false;
}
uint64_t lastTime = 0;
// With the initialization out of the way, we can now turn our attention
// to the per-frame rendering logic. As with the initialization, there is
// nothing really Slang-specific here, so the commentary doesn't need
// to be very detailed.
//
virtual void renderFrame(int frameBufferIndex) override
{       
    if(!lastTime) lastTime = getCurrentTime();
    uint64_t currentTime = getCurrentTime();
    float deltaTime = float(double(currentTime - lastTime) / double(getTimerFrequency()));
    lastTime = currentTime;

    ComPtr<ICommandBuffer> commandBuffer = gTransientHeaps[frameBufferIndex]->createCommandBuffer();
    auto renderEncoder = commandBuffer->encodeRenderCommands(gRenderPass, gFramebuffers[frameBufferIndex]);
     glm::mat4x4 identity = glm::mat4x4(1.0f);
    auto clientRect = getWindow()->getClientRect();
    if (clientRect.height == 0)
        return;
    glm::mat4x4 projection = glm::perspectiveRH_ZO(
        glm::radians(60.0f), float(clientRect.width) / float(clientRect.height),
        0.1f,
        1000.0f);
    glm::mat3x3 cameraOrientationMat(cameraOrientation);
    glm::vec3 forward = -cameraOrientationMat[2];
    glm::vec3 right = cameraOrientationMat[0];

    glm::vec3 movement = glm::vec3(0);
    if(wPressed) movement += forward;
    if(sPressed) movement -= forward;
    if(aPressed) movement -= right;
    if(dPressed) movement += right;

    cameraPosition += deltaTime * translationScale * movement;

    glm::mat4x4 view = identity;
    view *= glm::mat4x4(inverse(cameraOrientation));
    view = glm::translate(view, -cameraPosition);

    glm::mat4x4 viewProjection = projection * view;
    auto deviceInfo = gDevice->getDeviceInfo();
    glm::mat4x4 correctionMatrix;
    memcpy(&correctionMatrix, deviceInfo.identityProjectionMatrix, sizeof(float)*16);
    viewProjection = correctionMatrix * viewProjection;
    // glm uses column-major layout, we need to translate it to row-major.
    viewProjection = glm::transpose(viewProjection);


     auto transformMatrix = viewProjection;
    gfx::Viewport viewport = {};
    viewport.maxZ = 1.0f;
    viewport.extentX = (float)windowWidth;
    viewport.extentY = (float)windowHeight;
    renderEncoder->setViewportAndScissor(viewport);

    // In order to bind shader parameters to the pipeline, we need
    // to know how those parameters were assigned to locations/bindings/registers
    // for the target graphics API.
    //
    // The Slang compiler assigns locations to parameters in a deterministic
    // fashion, so it is possible for a programmer to hard-code locations
    // into their application code that will match up with their shaders.
    //
    // Hard-coding of locations can become intractable as an application needs
    // to support more different target platforms and graphics APIs, as well
    // as more shaders with different specialized variants.
    //
    // Rather than rely on hard-coded locations, our examples will make use of
    // reflection information provided by the Slang compiler (see `programLayout`
    // above), and our example graphics API layer will translate that reflection
    // information into a layout for a "root shader object."
    //
    // The root object will store values/bindings for all of the parameters in
    // the `IShaderProgram` used to create the pipeline state. At a conceptual
    // level we can think of `rootObject` as representing the "global scope" of
    // the shader program that was loaded; it has entries for each global shader
    // parameter that was declared.
    //
    // Readers who are familiar with D3D12 or Vulkan might think of this root
    // layout as being similar in spirit to a "root signature" or "pipeline layout."
    //
    // We start parameter binding by binding the pipeline state in command encoder.
    // This method will return a transient root shader object for us to write our
    // shader parameters into.
    //
    auto rootObject = renderEncoder->bindPipeline(gPipelineState);

    // We will update the model-view-projection matrix that is passed
    // into the shader code via the `Uniforms` buffer on a per-frame
    // basis, even though the data that is loaded does not change
    // per-frame (we always use an identity matrix).
    //
    deviceInfo = gDevice->getDeviceInfo();

    // We know that `rootObject` is a root shader object created
    // from our program, and that it is set up to hold values for
    // all the parameter of that program. In order to actually
    // set values, we need to be able to look up the location
    // of speciic parameter that we want to set.
    //
    // Our example graphics API layer supports this operation
    // with the idea of a *shader cursor* which can be thought
    // of as pointing "into" a particular shader object at
    // some location/offset. This design choice abstracts over
    // the many ways that different platforms and APIs represent
    // the necessary offset information.
    //
    // We construct an initial shader cursor that points at the
    // entire shader program. You can think of this as akin to
    // a diretory path of `/` for the root directory in a file
    // system.
    //
    ShaderCursor rootCursor(rootObject);
    //
    // Next, we use a convenience overload of `operator[]` to
    // navigate from the root cursor down to the parameter we
    // want to set.
    //
    // The operation `rootCursor["Uniforms"]` looks up the
    // offset/location of the global shader parameter `Uniforms`
    // (which is a uniform/constant buffer), and the subsequent
    // `["modelViewProjection"]` step navigates from there down
    // to the member named `modelViewProjection` in that buffer.
    //
    // Once we have formed a cursor that "points" at the
    // model-view projection matrix, we can set its data directly.
    //
    rootCursor["Uniforms"]["modelViewProjection"].setData(
        &transformMatrix, sizeof(float) * 16);
        rootCursor["Uniforms"]["texRef"].setResource(gTexView);
        rootCursor["Uniforms"]["sampler"].setSampler(gSampler);
    //
    // Some readers might be concerned about the performance o
    // the above operations because of the use of strings. For
    // those readers, here are two things to note:
    //
    // * While these `operator[]` steps do need to perform string
    //   comparisons, they do *not* make copies of the strings or
    //   perform any heap allocation.
    //
    // * There are other overloads of `operator[]` that use the
    //   *index* of a parameter/field instead of its name, and those
    //   operations have fixed/constant overhead and perform no
    //   string comparisons. The indices used are independent of
    //   the target platform and graphics API, and can thus be
    //   hard-coded even in cross-platform code.
    //

    // We also need to set up a few pieces of fixed-function pipeline
    // state that are not bound by the pipeline state above.
    //
    renderEncoder->setVertexBuffer(0, gVertexBuffer);
    renderEncoder->setPrimitiveTopology(PrimitiveTopology::TriangleList);

    // Finally, we are ready to issue a draw call for a single triangle.
    //º
    renderEncoder->draw(15000);
    renderEncoder->endEncoding();
    commandBuffer->close();
    gQueue->executeCommandBuffer(commandBuffer);

    // With that, we are done drawing for one frame, and ready for the next.
    //
    gSwapchain->present();
}

};

// This macro instantiates an appropriate main function to
// run the application defined above.
PLATFORM_UI_MAIN(innerMain<HelloWorld>)
