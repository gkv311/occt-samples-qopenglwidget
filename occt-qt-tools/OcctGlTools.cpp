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

// ================================================================
// Function : GetGlContext
// ================================================================
Handle(OpenGl_Context) OcctGlTools::GetGlContext(const Handle(V3d_View)& theView)
{
  Handle(OpenGl_View) aGlView = Handle(OpenGl_View)::DownCast(theView->View());
  return aGlView->GlWindow()->GetGlContext();
}

// ================================================================
// Function : InitializeGlWindow
// ================================================================
bool OcctGlTools::InitializeGlWindow(const Handle(V3d_View)& theView,
                                     const Aspect_Drawable theNativeWin,
                                     const Graphic3d_Vec2i& theSize)
{
  Aspect_Drawable aNativeWin = theNativeWin;
#ifdef _WIN32
  HDC  aWglDevCtx = wglGetCurrentDC();
  HWND aWglWin = WindowFromDC(aWglDevCtx);
  aNativeWin = (Aspect_Drawable)aWglWin;
#endif

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
