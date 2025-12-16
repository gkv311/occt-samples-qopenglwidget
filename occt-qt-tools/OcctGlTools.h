// Copyright (c) 2023 Kirill Gavrilov

#ifndef _OcctGlTools_HeaderFile
#define _OcctGlTools_HeaderFile

#include <V3d_View.hxx>

class OpenGl_Context;

//! Auxiliary wrapper to avoid OpenGL macros collisions between Qt and OCCT headers.
class OcctGlTools
{
public:
  //! Return GL context.
  static Handle(OpenGl_Context) GetGlContext(const Handle(V3d_View)& theView);

  //! Cleanup up global GL state after Qt before redrawing OCCT Viewer.
  static void ResetGlStateBeforeOcct(const Handle(V3d_View)& theView);

  //! Cleanup up global GL state after OCCT before redrawing Qt.
  //! Alternative to QQuickOpenGLUtils::resetOpenGLState().
  static void ResetGlStateAfterOcct(const Handle(V3d_View)& theView);
};

#endif // _OcctGlTools_HeaderFile
