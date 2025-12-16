// Copyright (c) 2023 Kirill Gavrilov

#ifdef _WIN32
#include <windows.h>
#endif

#include "OcctGlTools.h"

#include <Aspect_NeutralWindow.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <OpenGl_GlCore20.hxx>
#include <OpenGl_FrameBuffer.hxx>
#include <OpenGl_View.hxx>
#include <OpenGl_Window.hxx>

//! OpenGL FBO subclass for wrapping FBO created by Qt using GL_RGBA8
//! texture format instead of GL_SRGB8_ALPHA8.
//! This FBO is set to OpenGl_Context::SetDefaultFrameBuffer() as a final target.
//! Subclass calls OpenGl_Context::SetFrameBufferSRGB() with sRGB=false flag,
//! which asks OCCT to disable GL_FRAMEBUFFER_SRGB and apply sRGB gamma correction manually.
class OcctQtFrameBuffer : public OpenGl_FrameBuffer
{
  DEFINE_STANDARD_RTTI_INLINE(OcctQtFrameBuffer, OpenGl_FrameBuffer)
public:
  //! Empty constructor.
  OcctQtFrameBuffer() {}

  //! Make this FBO active in context.
  virtual void BindBuffer(const Handle(OpenGl_Context)& theGlCtx) override
  {
    OpenGl_FrameBuffer::BindBuffer(theGlCtx);
    theGlCtx->SetFrameBufferSRGB(true, false);
  }

  //! Make this FBO as drawing target in context.
  virtual void BindDrawBuffer(const Handle(OpenGl_Context)& theGlCtx) override
  {
    OpenGl_FrameBuffer::BindDrawBuffer(theGlCtx);
    theGlCtx->SetFrameBufferSRGB(true, false);
  }

  //! Make this FBO as reading source in context.
  virtual void BindReadBuffer(const Handle(OpenGl_Context)& theGlCtx) override
  {
    OpenGl_FrameBuffer::BindReadBuffer(theGlCtx);
  }
};

// ================================================================
// Function : GetGlContext
// ================================================================
Handle(OpenGl_Context) OcctGlTools::GetGlContext(const Handle(V3d_View)& theView)
{
  Handle(OpenGl_View) aGlView = Handle(OpenGl_View)::DownCast(theView->View());
  return aGlView->GlWindow()->GetGlContext();
}

// ================================================================
// Function : GetGlNativeWindow
// ================================================================
Aspect_Drawable OcctGlTools::GetGlNativeWindow(Aspect_Drawable theNativeWin)
{
  Aspect_Drawable aNativeWin = (Aspect_Drawable)theNativeWin;
#ifdef _WIN32
  HDC  aWglDevCtx = wglGetCurrentDC();
  HWND aWglWin = WindowFromDC(aWglDevCtx);
  aNativeWin = (Aspect_Drawable)aWglWin;
#endif
  return aNativeWin;
}

// ================================================================
// Function : InitializeGlWindow
// ================================================================
bool OcctGlTools::InitializeGlWindow(const Handle(V3d_View)& theView,
                                     const Aspect_Drawable theNativeWin,
                                     const Graphic3d_Vec2i& theSize)
{
  const Aspect_Drawable aNativeWin = GetGlNativeWindow(theNativeWin);

  Handle(OpenGl_GraphicDriver) aDriver = Handle(OpenGl_GraphicDriver)::DownCast(theView->Viewer()->Driver());
  Handle(OpenGl_Context) aGlCtx = new OpenGl_Context();
  if (!aGlCtx->Init(!aDriver->Options().contextCompatible))
  {
    Message::SendFail() << "Error: OpenGl_Context is unable to wrap OpenGL context";
    return false;
  }

  Handle(Aspect_NeutralWindow) aWindow = Handle(Aspect_NeutralWindow)::DownCast(theView->Window());
  if (!aWindow.IsNull())
  {
    aWindow->SetNativeHandle(aNativeWin);
    aWindow->SetSize(theSize.x(), theSize.y());
    theView->SetWindow(aWindow, aGlCtx->RenderingContext());
  }
  else
  {
    aWindow = new Aspect_NeutralWindow();
    aWindow->SetVirtual(true);
    aWindow->SetNativeHandle(aNativeWin);
    aWindow->SetSize(theSize.x(), theSize.y());
    theView->SetWindow(aWindow, aGlCtx->RenderingContext());
  }
  return true;
}

// ================================================================
// Function : InitializeGlFbo
// ================================================================
bool OcctGlTools::InitializeGlFbo(const Handle(V3d_View)& theView)
{
  Handle(OpenGl_Context)     aGlCtx = OcctGlTools::GetGlContext(theView);
  Handle(OpenGl_FrameBuffer) aDefaultFbo = aGlCtx->DefaultFrameBuffer();
  if (aDefaultFbo.IsNull())
  {
    aDefaultFbo = new OcctQtFrameBuffer();
    aGlCtx->SetDefaultFrameBuffer(aDefaultFbo);
  }
  if (!aDefaultFbo->InitWrapper(aGlCtx))
  {
    aDefaultFbo.Nullify();
    Message::DefaultMessenger()->Send("Default FBO wrapper creation failed", Message_Fail);
    return false;
  }

  Graphic3d_Vec2i aViewSizeOld;
  const Graphic3d_Vec2i        aViewSizeNew = aDefaultFbo->GetVPSize();
  Handle(Aspect_NeutralWindow) aWindow = Handle(Aspect_NeutralWindow)::DownCast(theView->Window());
  aWindow->Size(aViewSizeOld.x(), aViewSizeOld.y());
  if (aViewSizeNew != aViewSizeOld)
  {
    aWindow->SetSize(aViewSizeNew.x(), aViewSizeNew.y());
    theView->MustBeResized();
    theView->Invalidate();
#if (OCC_VERSION_HEX >= 0x070700)
    for (const Handle(V3d_View)& aSubviewIter : theView->Subviews())
    {
      aSubviewIter->MustBeResized();
      aSubviewIter->Invalidate();
      aDefaultFbo->SetupViewport(aGlCtx);
    }
#endif
  }
  return true;
}

// ================================================================
// Function : ResetGlStateBeforeOcct
// ================================================================
void OcctGlTools::ResetGlStateBeforeOcct(const Handle(V3d_View)& theView)
{
  Handle(OpenGl_Context) aGlCtx = GetGlContext(theView);
  if (aGlCtx.IsNull())
    return;

  if (aGlCtx->core20fwd != nullptr)
  {
    // shouldn't be a problem in most cases, but make sure to unbind active GLSL program
    aGlCtx->core20fwd->glUseProgram(0);
  }

  // Qt leaves GL_BLEND enabled after drawing semitransparent elements,
  // but OCCT doesn't reset its state before drawing opaque objects.
  // Disable also texture bindings left by Qt.
  aGlCtx->core11fwd->glBindTexture(GL_TEXTURE_2D, 0);
  aGlCtx->core11fwd->glDisable(GL_BLEND);
  if (aGlCtx->core11ffp != nullptr)
  {
    aGlCtx->core11fwd->glDisable(GL_ALPHA_TEST);
    aGlCtx->core11fwd->glDisable(GL_TEXTURE_2D);
  }
}

// ================================================================
// Function : ResetGlStateAfterOcct
// ================================================================
void OcctGlTools::ResetGlStateAfterOcct(const Handle(V3d_View)& theView)
{
  Handle(OpenGl_Context) aGlCtx = GetGlContext(theView);
  if (aGlCtx.IsNull())
    return;

  // Qt expects default OpenGL pack/unpack alignment setup,
  // while OCCT manages it dynamically;
  // without resetting alignment setup, Qt will draw
  // some textures corrupted (like fonts)
  aGlCtx->core11fwd->glPixelStorei(GL_PACK_ALIGNMENT, 4);
  aGlCtx->core11fwd->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

  if (aGlCtx->core15fwd != nullptr)
  {
    // Qt expects first texture object to be bound,
    // but OCCT might leave another object bound within multi-texture mapping
    aGlCtx->core15fwd->glActiveTexture(GL_TEXTURE0);
  }
}
