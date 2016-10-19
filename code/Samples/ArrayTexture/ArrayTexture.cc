//------------------------------------------------------------------------------
//  ArrayTexture.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Gfx/Gfx.h"
#include "Dbg/Dbg.h"
#include "Assets/Gfx/ShapeBuilder.h"
#include "glm/gtc/matrix_transform.hpp"
#include "shaders.h"

using namespace Oryol;

class ArrayTextureApp : public App {
public:
    /// on init frame method
    virtual AppState::Code OnInit();
    /// on running frame method
    virtual AppState::Code OnRunning();
    /// on cleanup frame method
    virtual AppState::Code OnCleanup();

    AppState::Code notSupported();
    Shader::VSParams computeShaderParams();

    DrawState drawState;
    int frameIndex = 0;
    glm::mat4 proj;
};
OryolMain(ArrayTextureApp);

//------------------------------------------------------------------------------
AppState::Code
ArrayTextureApp::OnInit() {
    auto gfxSetup = GfxSetup::WindowMSAA4(800, 600, "Array Texture Sample");
    gfxSetup.DefaultClearColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    Gfx::Setup(gfxSetup);
    Dbg::Setup();

    // if array textures are not supported, only show a warning
    if (!Gfx::QueryFeature(GfxFeature::TextureArray)) {
        return App::OnInit();
    }

    // create a 16x16 array texture with 2 slices, one with red, the
    // other with green checker board
    const int numSlices = 2;
    const int width = 16;
    const int height = 16;
    uint32_t data[numSlices][height][width];
    for (int slice = 0, evenOdd = 0; slice < numSlices; slice++, evenOdd++) {
        for (int y = 0; y < height; y++, evenOdd++) {
            for (int x = 0; x < width; x++, evenOdd++) {
                data[slice][y][x] = evenOdd&1 ? 0 : (slice==0 ? 0xFF0000FF : 0xFF00FF00);
            }
        }
    }
    auto texSetup = TextureSetup::FromPixelDataArray(16, 16, 2, 1, PixelFormat::RGBA8);
    texSetup.ImageData.Sizes[0][0] = sizeof(data);
    this->drawState.FSTexture[Textures::Texture] = Gfx::CreateResource(texSetup, data, sizeof(data));

    // build a cube mesh
    ShapeBuilder shapeBuilder;
    shapeBuilder.Layout
        .Add(VertexAttr::Position, VertexFormat::Float3)
        .Add(VertexAttr::TexCoord0, VertexFormat::Float2);
    shapeBuilder.Box(1.0f, 1.0f, 1.0f, 1);
    this->drawState.Mesh[0] = Gfx::CreateResource(shapeBuilder.Build());

    // ...and a pipeline object to complete the DrawState
    Id shd = Gfx::CreateResource(Shader::Setup());
    auto pipSetup = PipelineSetup::FromLayoutAndShader(shapeBuilder.Layout, shd);
    pipSetup.DepthStencilState.DepthWriteEnabled = true;
    pipSetup.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
    pipSetup.RasterizerState.SampleCount = gfxSetup.SampleCount;
    this->drawState.Pipeline = Gfx::CreateResource(pipSetup);

    // setup a projection matrix with the right aspect ratio
    const float fbWidth = (const float) Gfx::DisplayAttrs().FramebufferWidth;
    const float fbHeight = (const float) Gfx::DisplayAttrs().FramebufferHeight;
    this->proj = glm::perspectiveFov(glm::radians(45.0f), fbWidth, fbHeight, 0.01f, 100.0f);

    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
ArrayTextureApp::OnRunning() {

    // if array textures not supported, just display a warning
    if (!Gfx::QueryFeature(GfxFeature::TextureArray)) {
        return this->notSupported();
    }

    auto vsParams = this->computeShaderParams();

    Gfx::BeginPass();
    Gfx::ApplyDrawState(this->drawState);
    Gfx::ApplyUniformBlock(vsParams);
    Gfx::Draw();
    Gfx::EndPass();
    Gfx::CommitFrame();
    this->frameIndex++;
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}


//------------------------------------------------------------------------------
AppState::Code
ArrayTextureApp::OnCleanup() {
    Dbg::Discard();
    Gfx::Discard();
    return App::OnCleanup();
}

//------------------------------------------------------------------------------
Shader::VSParams
ArrayTextureApp::computeShaderParams() {
    Shader::VSParams vsParams;

    float offset = float(this->frameIndex) * 0.001f;
    vsParams.UVOffset = glm::vec4(offset, -offset, -offset, offset);

    const glm::vec3 pos(0.0f, 0.0f, -3.0f);
    float angleX = glm::radians(0.25f * this->frameIndex);
    float angleY = glm::radians(0.2f * this->frameIndex);
    glm::mat4 model = glm::translate(glm::mat4(), pos);
    model = glm::rotate(model, angleX, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, angleY, glm::vec3(0.0f, 1.0f, 0.0f));
    vsParams.ModelViewProj = this->proj * model;

    return vsParams;
}

//------------------------------------------------------------------------------
AppState::Code
ArrayTextureApp::notSupported() {
    #if ORYOL_EMSCRIPTEN
    const char* msg = "This demo needs WebGL2\n";
    #else
    const char* msg = "This demo needs 3D texture support\n";
    #endif
    uint8_t x = (Gfx::DisplayAttrs().FramebufferWidth/16 - strlen(msg))/2;
    uint8_t y = Gfx::DisplayAttrs().FramebufferHeight/16/2;
    Gfx::BeginPass(PassState(glm::vec4(0.5f, 0.0f, 0.0f, 1.0f)));
    Dbg::SetTextScale(glm::vec2(2.0f, 2.0f));
    Dbg::CursorPos(x, y);
    Dbg::Print(msg);
    Dbg::DrawTextBuffer();
    Gfx::EndPass();
    Gfx::CommitFrame();
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

