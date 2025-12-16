// Copyright (c) 2023 Kirill Gavrilov

#include "OcctGlTools.h"

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
