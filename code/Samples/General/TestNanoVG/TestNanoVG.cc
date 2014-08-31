//------------------------------------------------------------------------------
//  NanoVG.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/App.h"
#include "IO/IOFacade.h"
#include "IO/Core/IOQueue.h"
#include "HTTP/HTTPFileSystem.h"
#include "Render/RenderFacade.h"
#include "Input/InputFacade.h"
#include "Time/Clock.h"
#include "NanoVG/NVGFacade.h"
#include "NanoVG/NanoVG.h"
#include "demo.h"

using namespace Oryol;
using namespace Oryol::IO;
using namespace Oryol::HTTP;
using namespace Oryol::Core;
using namespace Oryol::Render;
using namespace Oryol::Time;
using namespace Oryol::Input;
using namespace Oryol::NanoVG;

// derived application class
class NanoVGApp : public App {
public:
    virtual AppState::Code OnInit();
    virtual AppState::Code OnRunning();
    virtual AppState::Code OnCleanup();
    
private:
    void loadAssets();

    IOFacade* io = nullptr;
    RenderFacade* render = nullptr;
    InputFacade* input = nullptr;
    NVGFacade* nvg = nullptr;
    NVGcontext* ctx = nullptr;
    DemoData data;
    IOQueue ioQueue;
};
OryolMain(NanoVGApp);

//------------------------------------------------------------------------------
AppState::Code
NanoVGApp::OnRunning() {
    
    this->ioQueue.Update();

    const DisplayAttrs& attrs = this->render->GetDisplayAttrs();
    const int32 w = attrs.FramebufferWidth;
    const int32 h = attrs.FramebufferHeight;
    if (this->render->BeginFrame()) {
        this->render->ApplyDefaultRenderTarget();
        this->render->Clear(Channel::All, glm::vec4(0.3f), 1.0f, 0);

        const Mouse& mouse = this->input->Mouse();
        const int32 mouseX = mouse.Position().x;
        const int32 mouseY = mouse.Position().y;
        const int32 blowup = this->input->Keyboard().KeyPressed(Key::Space) ? 1 : 0;
        const float64 time = Clock::Now().Since(0).AsSeconds();
        
        this->nvg->BeginFrame(this->ctx);
        renderDemo(this->ctx, mouseX, mouseY, w, h, time, blowup, &this->data);
        this->nvg->EndFrame(this->ctx);

        this->render->EndFrame();
    }
    
    // continue running or quit?
    return render->QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
NanoVGApp::OnInit() {
    this->io = IOFacade::CreateSingle();
    this->io->RegisterFileSystem("http", Creator<HTTPFileSystem, FileSystem>());
    this->io->SetAssign("res:", "http://localhost:8000/");

    this->render = RenderFacade::CreateSingle(RenderSetup::Windowed(1024, 600, "Oryol NanoVG Sample", PixelFormat::RGB8, PixelFormat::D24S8));
    this->input  = InputFacade::CreateSingle();
    this->nvg    = NVGFacade::CreateSingle();
    this->ctx    = this->nvg->CreateContext(NVG_STENCIL_STROKES | NVG_ANTIALIAS);
    
    // start loading assets asynchronously
    StringBuilder str;
    for (int i = 0; i < 12; i++) {
        str.Format(128, "res:image%d.jpg", i+1);
        this->ioQueue.Add(str.GetString(), 0, [this, i](const Ptr<Stream>& stream) {
            this->data.images[i] = this->nvg->CreateImage(this->ctx, stream, 0);
        });
    }
    this->ioQueue.Add("res:entypo.ttf", 0, [this](const Ptr<Stream>& stream) {
        this->data.fontIcons = this->nvg->CreateFont(this->ctx, "icons", stream);
    });
    this->ioQueue.Add("res:Roboto-Regular.ttf", 0, [this](const Ptr<Stream>& stream) {
        this->data.fontNormal = this->nvg->CreateFont(this->ctx, "sans", stream);
    });
    this->ioQueue.Add("res:Roboto-Bold.ttf", 0, [this](const Ptr<Stream>& stream) {
        this->data.fontBold = this->nvg->CreateFont(this->ctx, "sans-bold", stream);
    });
    
    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
NanoVGApp::OnCleanup() {
    // cleanup everything
    freeDemoData(this->ctx, &this->data);
    this->nvg->DeleteContext(this->ctx);
    this->ctx = nullptr;
    this->nvg = nullptr;
    this->input = nullptr;
    this->render = nullptr;
    NVGFacade::DestroySingle();
    InputFacade::DestroySingle();
    RenderFacade::DestroySingle();
    return App::OnCleanup();
}
