// Copyright (c) 2023 Kirill Gavrilov

#ifdef _WIN32
  // should be included before other headers to avoid missing definitions
  #include <windows.h>
#endif
#include <OpenGl_Context.hxx>

#include "OcctQOpenGLWidgetViewer.h"

#include "../occt-qt-tools/OcctGlTools.h"
#include "../occt-qt-tools/OcctQtTools.h"

#include <Standard_WarningsDisable.hxx>
#include <QApplication>
#include <QMessageBox>
#include <QMouseEvent>
#include <Standard_WarningsRestore.hxx>

#include <AIS_Shape.hxx>
#include <AIS_ViewCube.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_NeutralWindow.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <Message.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <OpenGl_FrameBuffer.hxx>

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
// Function : OcctQOpenGLWidgetViewer
// ================================================================
OcctQOpenGLWidgetViewer::OcctQOpenGLWidgetViewer(QWidget* theParent)
    : QOpenGLWidget(theParent)
{
  Handle(Aspect_DisplayConnection) aDisp   = new Aspect_DisplayConnection();
  Handle(OpenGl_GraphicDriver)     aDriver = new OpenGl_GraphicDriver(aDisp, false);
  // lets QOpenGLWidget to manage buffer swap
  aDriver->ChangeOptions().buffersNoSwap = true;
  // don't write into alpha channel
  aDriver->ChangeOptions().buffersOpaqueAlpha = true;
  // offscreen FBOs should be always used
  aDriver->ChangeOptions().useSystemBuffer = false;

  // create viewer
  myViewer = new V3d_Viewer(aDriver);
  myViewer->SetDefaultBackgroundColor(Quantity_NOC_BLACK);
  myViewer->SetDefaultLights();
  myViewer->SetLightOn();
  myViewer->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);

  // create AIS context
  myContext = new AIS_InteractiveContext(myViewer);

  myViewCube = new AIS_ViewCube();
  myViewCube->SetViewAnimation(myViewAnimation);
  myViewCube->SetFixedAnimationLoop(false);
  myViewCube->SetAutoStartAnimation(true);
  myViewCube->TransformPersistence()->SetOffset2d(Graphic3d_Vec2i(100, 150));

  // note - window will be created later within initializeGL() callback!
  myView = myViewer->CreateView();
  myView->SetImmediateUpdate(false);
#ifndef __APPLE__
  myView->ChangeRenderingParams().NbMsaaSamples = 4; // warning - affects performance
#endif
  myView->ChangeRenderingParams().ToShowStats    = true;
  myView->ChangeRenderingParams().CollectedStats = (Graphic3d_RenderingParams::PerfCounters)(
    Graphic3d_RenderingParams::PerfCounters_FrameRate | Graphic3d_RenderingParams::PerfCounters_Triangles);

  // Qt widget setup
  setAttribute(Qt::WA_AcceptTouchEvents); // necessary to recieve QTouchEvent events
  setMouseTracking(true);
  setBackgroundRole(QPalette::NoRole); // or NoBackground
  setFocusPolicy(Qt::StrongFocus);     // set focus policy to threat QContextMenuEvent from keyboard
  setUpdatesEnabled(true);
  setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

  // OpenGL setup managed by Qt
  QSurfaceFormat aGlFormat;
  aGlFormat.setDepthBufferSize(24);
  aGlFormat.setStencilBufferSize(8);
  // aGlFormat.setOption (QSurfaceFormat::DebugContext, true);
  aDriver->ChangeOptions().contextDebug = aGlFormat.testOption(QSurfaceFormat::DebugContext);
  // aGlFormat.setOption (QSurfaceFormat::DeprecatedFunctions, true);
  if (myIsCoreProfile)
    aGlFormat.setVersion(4, 5);

  aGlFormat.setProfile(myIsCoreProfile ? QSurfaceFormat::CoreProfile : QSurfaceFormat::CompatibilityProfile);

  // request sRGBColorSpace colorspace to meet OCCT expectations or use OcctQtFrameBuffer fallback.
  /*#if (QT_VERSION_MAJOR > 5) || (QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR >= 10)
    aGlFormat.setColorSpace(QSurfaceFormat::sRGBColorSpace);
    setTextureFormat(GL_SRGB8_ALPHA8);
  #else
    Message::SendWarning("Warning! Qt 5.10+ is required for sRGB setup.\n"
                         "Colors in 3D Viewer might look incorrect (Qt " QT_VERSION_STR " is used).\n");
    aDriver->ChangeOptions().sRGBDisable = true;
  #endif*/

  setFormat(aGlFormat);

#if defined(_WIN32)
  // never use ANGLE on Windows, since OCCT 3D Viewer does not expect this
  QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
  // QCoreApplication::setAttribute (Qt::AA_UseOpenGLES);
#endif
}

// ================================================================
// Function : ~OcctQOpenGLWidgetViewer
// ================================================================
OcctQOpenGLWidgetViewer::~OcctQOpenGLWidgetViewer()
{
  // hold on X11 display connection till making another connection active by glXMakeCurrent()
  // to workaround sudden crash in QOpenGLWidget destructor
  Handle(Aspect_DisplayConnection) aDisp = myViewer->Driver()->GetDisplayConnection();

  // release OCCT viewer
  myContext->RemoveAll(false);
  myContext.Nullify();
  myView->Remove();
  myView.Nullify();
  myViewer.Nullify();

  // make active OpenGL context created by Qt
  makeCurrent();
  aDisp.Nullify();
}

// ================================================================
// Function : dumpGlInfo
// ================================================================
void OcctQOpenGLWidgetViewer::dumpGlInfo(bool theIsBasic, bool theToPrint)
{
  TColStd_IndexedDataMapOfStringString aGlCapsDict;
  myView->DiagnosticInformation(aGlCapsDict,
                                theIsBasic ? Graphic3d_DiagnosticInfo_Basic : Graphic3d_DiagnosticInfo_Complete);
  TCollection_AsciiString anInfo;
  for (TColStd_IndexedDataMapOfStringString::Iterator aValueIter(aGlCapsDict); aValueIter.More(); aValueIter.Next())
  {
    if (!aValueIter.Value().IsEmpty())
    {
      if (!anInfo.IsEmpty())
        anInfo += "\n";

      anInfo += aValueIter.Key() + ": " + aValueIter.Value();
    }
  }

  if (theToPrint)
    Message::SendInfo(anInfo);

  myGlInfo = QString::fromUtf8(anInfo.ToCString());
}

// ================================================================
// Function : initializeGL
// ================================================================
void OcctQOpenGLWidgetViewer::initializeGL()
{
  const QRect           aRect = rect();
  const Graphic3d_Vec2i aViewSize(aRect.right() - aRect.left(), aRect.bottom() - aRect.top());

  Aspect_Drawable aNativeWin = (Aspect_Drawable)winId();
#ifdef _WIN32
  HDC  aWglDevCtx = wglGetCurrentDC();
  HWND aWglWin    = WindowFromDC(aWglDevCtx);
  aNativeWin      = (Aspect_Drawable)aWglWin;
#endif

  Handle(OpenGl_Context) aGlCtx = new OpenGl_Context();
  if (!aGlCtx->Init(myIsCoreProfile))
  {
    Message::SendFail() << "Error: OpenGl_Context is unable to wrap OpenGL context";
    QMessageBox::critical(0, "Failure", "OpenGl_Context is unable to wrap OpenGL context");
    QApplication::exit(1);
    return;
  }

  Handle(Aspect_NeutralWindow) aWindow = Handle(Aspect_NeutralWindow)::DownCast(myView->Window());
  if (!aWindow.IsNull())
  {
    aWindow->SetNativeHandle(aNativeWin);
    aWindow->SetSize(aViewSize.x(), aViewSize.y());
    myView->SetWindow(aWindow, aGlCtx->RenderingContext());
    dumpGlInfo(true, true);
  }
  else
  {
    aWindow = new Aspect_NeutralWindow();
    aWindow->SetVirtual(true);
    aWindow->SetNativeHandle(aNativeWin);
    aWindow->SetSize(aViewSize.x(), aViewSize.y());
    myView->SetWindow(aWindow, aGlCtx->RenderingContext());
    dumpGlInfo(true, true);

    myContext->Display(myViewCube, 0, 0, false);
  }

  {
    // dummy shape for testing
    TopoDS_Shape      aBox   = BRepPrimAPI_MakeBox(100.0, 50.0, 90.0).Shape();
    Handle(AIS_Shape) aShape = new AIS_Shape(aBox);
    myContext->Display(aShape, AIS_Shaded, 0, false);
  }
}

// ================================================================
// Function : event
// ================================================================
bool OcctQOpenGLWidgetViewer::event(QEvent* theEvent)
{
  if (myView.IsNull())
    return QOpenGLWidget::event(theEvent);

  const bool isTouch = theEvent->type() == QEvent::TouchBegin
                    || theEvent->type() == QEvent::TouchUpdate
                    || theEvent->type() == QEvent::TouchEnd;
  if (!isTouch)
    return QOpenGLWidget::event(theEvent);

  bool hasUpdates = false;
  const QTouchEvent* aQTouchEvent = static_cast<QTouchEvent*>(theEvent);
  for (const QTouchEvent::TouchPoint& aQTouch : aQTouchEvent->touchPoints())
  {
    const Standard_Size   aTouchId = aQTouch.id();
    const Graphic3d_Vec2d aNewPos2d(aQTouch.pos().x(), aQTouch.pos().y());
    const Graphic3d_Vec2i aNewPos2i = Graphic3d_Vec2i(aNewPos2d + Graphic3d_Vec2d(0.5));
    if (aQTouch.state() == Qt::TouchPointPressed
     && aNewPos2i.minComp() >= 0)
    {
      hasUpdates = true;
      AIS_ViewController::AddTouchPoint(aTouchId, aNewPos2d);
    }
    else if (aQTouch.state() == Qt::TouchPointMoved
          && AIS_ViewController::TouchPoints().Contains(aTouchId))
    {
      hasUpdates = true;
      AIS_ViewController::UpdateTouchPoint(aTouchId, aNewPos2d);
    }
    else if (aQTouch.state() == Qt::TouchPointReleased
          && AIS_ViewController::RemoveTouchPoint(aTouchId))
    {
      hasUpdates = true;
    }
  }

  myHasTouchInput = true;
  if (hasUpdates)
    updateView();

  return true;
}

// ================================================================
// Function : closeEvent
// ================================================================
void OcctQOpenGLWidgetViewer::closeEvent(QCloseEvent* theEvent)
{
  theEvent->accept();
}

// ================================================================
// Function : keyPressEvent
// ================================================================
void OcctQOpenGLWidgetViewer::keyPressEvent(QKeyEvent* theEvent)
{
  if (myView.IsNull())
    return;

  const Aspect_VKey aKey = OcctQtTools::qtKey2VKey(theEvent->key());
  switch (aKey)
  {
    case Aspect_VKey_Escape: {
      QApplication::exit();
      return;
    }
    case Aspect_VKey_F: {
      myView->FitAll(0.01, false);
      update();
      return;
    }
  }
  QOpenGLWidget::keyPressEvent(theEvent);
}

// ================================================================
// Function : mousePressEvent
// ================================================================
void OcctQOpenGLWidgetViewer::mousePressEvent(QMouseEvent* theEvent)
{
  QOpenGLWidget::mousePressEvent(theEvent);
  if (myView.IsNull())
    return;

  if (myHasTouchInput && theEvent->source() == Qt::MouseEventSynthesizedBySystem)
    return; // skip mouse events emulated by system from screen touches

  const Graphic3d_Vec2i  aPnt(theEvent->pos().x(), theEvent->pos().y());
  const Aspect_VKeyMouse aButtons = OcctQtTools::qtMouseButtons2VKeys(theEvent->buttons());
  const Aspect_VKeyFlags aFlags = OcctQtTools::qtMouseModifiers2VKeys(theEvent->modifiers());
  if (AIS_ViewController::UpdateMouseButtons(aPnt, aButtons, aFlags, false))
    updateView();
}

// ================================================================
// Function : mouseReleaseEvent
// ================================================================
void OcctQOpenGLWidgetViewer::mouseReleaseEvent(QMouseEvent* theEvent)
{
  QOpenGLWidget::mouseReleaseEvent(theEvent);
  if (myView.IsNull())
    return;

  const Graphic3d_Vec2i  aPnt(theEvent->pos().x(), theEvent->pos().y());
  const Aspect_VKeyMouse aButtons = OcctQtTools::qtMouseButtons2VKeys(theEvent->buttons());
  const Aspect_VKeyFlags aFlags = OcctQtTools::qtMouseModifiers2VKeys(theEvent->modifiers());
  if (AIS_ViewController::UpdateMouseButtons(aPnt, aButtons, aFlags, false))
    updateView();
}

// ================================================================
// Function : mouseMoveEvent
// ================================================================
void OcctQOpenGLWidgetViewer::mouseMoveEvent(QMouseEvent* theEvent)
{
  QOpenGLWidget::mouseMoveEvent(theEvent);
  if (myView.IsNull())
    return;

  if (myHasTouchInput && theEvent->source() == Qt::MouseEventSynthesizedBySystem)
    return; // skip mouse events emulated by system from screen touches

  const Graphic3d_Vec2i  aNewPos(theEvent->pos().x(), theEvent->pos().y());
  const Aspect_VKeyMouse aButtons = OcctQtTools::qtMouseButtons2VKeys(theEvent->buttons());
  const Aspect_VKeyFlags aFlags = OcctQtTools::qtMouseModifiers2VKeys(theEvent->modifiers());
  if (AIS_ViewController::UpdateMousePosition(aNewPos, aButtons, aFlags, false))
    updateView();
}

// ==============================================================================
// function : wheelEvent
// ==============================================================================
void OcctQOpenGLWidgetViewer::wheelEvent(QWheelEvent* theEvent)
{
  QOpenGLWidget::wheelEvent(theEvent);
  if (myView.IsNull())
    return;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  const Graphic3d_Vec2i aPos(Graphic3d_Vec2d(theEvent->position().x(), theEvent->position().y()));
#else
  const Graphic3d_Vec2i aPos(theEvent->pos().x(), theEvent->pos().y());
#endif

#if (OCC_VERSION_HEX >= 0x070700)
  if (!myView->Subviews().IsEmpty())
  {
    Handle(V3d_View) aPickedView = myView->PickSubview(aPos);
    if (!aPickedView.IsNull() && aPickedView != myFocusView)
    {
      // switch input focus to another subview
      OnSubviewChanged(myContext, myFocusView, aPickedView);
      updateView();
      return;
    }
  }
#endif

  if (AIS_ViewController::UpdateZoom(Aspect_ScrollDelta(aPos, double(theEvent->angleDelta().y()) / 8.0)))
    updateView();
}

// =======================================================================
// function : updateView
// =======================================================================
void OcctQOpenGLWidgetViewer::updateView()
{
  update();
  // if (window() != NULL) { window()->update(); }
}

// ================================================================
// Function : paintGL
// ================================================================
void OcctQOpenGLWidgetViewer::paintGL()
{
  if (myView.IsNull() || myView->Window().IsNull())
    return;

  Aspect_Drawable aNativeWin = (Aspect_Drawable)winId();
#ifdef _WIN32
  HDC  aWglDevCtx = wglGetCurrentDC();
  HWND aWglWin    = WindowFromDC(aWglDevCtx);
  aNativeWin      = (Aspect_Drawable)aWglWin;
#endif
  if (myView->Window()->NativeHandle() != aNativeWin)
  {
    // workaround window recreation done by Qt on monitor (QScreen) disconnection
    Message::SendWarning() << "Native window handle has changed by QOpenGLWidget!";
    initializeGL();
    return;
  }

  // wrap FBO created by QOpenGLWidget
  // get context from this (composer) view rather than from arbitrary one
  // Handle(OpenGl_GraphicDriver) aDriver =
  //   Handle(OpenGl_GraphicDriver)::DownCast(myContext->CurrentViewer()->Driver());
  // Handle(OpenGl_Context) aGlCtx = aDriver->GetSharedContext();
  Handle(OpenGl_Context)     aGlCtx      = OcctGlTools::GetGlContext(myView);
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
    QMessageBox::critical(0, "Failure", "Default FBO wrapper creation failed");
    QApplication::exit(1);
    return;
  }

  Graphic3d_Vec2i aViewSizeOld;
  // don't use QWidget::rect() as it might return misleading information
  // const QRect aRect = rect();
  // Graphic3d_Vec2i aViewSizeNew(aRect.right() - aRect.left(), aRect.bottom() - aRect.top());
  const Graphic3d_Vec2i        aViewSizeNew = aDefaultFbo->GetVPSize();
  Handle(Aspect_NeutralWindow) aWindow      = Handle(Aspect_NeutralWindow)::DownCast(myView->Window());
  aWindow->Size(aViewSizeOld.x(), aViewSizeOld.y());
  if (aViewSizeNew != aViewSizeOld)
  {
    aWindow->SetSize(aViewSizeNew.x(), aViewSizeNew.y());
    myView->MustBeResized();
    myView->Invalidate();
    dumpGlInfo(true, false);

#if (OCC_VERSION_HEX >= 0x070700)
    for (const Handle(V3d_View)& aSubviewIter : myView->Subviews())
    {
      aSubviewIter->MustBeResized();
      aSubviewIter->Invalidate();
      aDefaultFbo->SetupViewport(aGlCtx);
    }
#endif
  }

  // reset global GL state from Qt before redrawing OCCT
  OcctGlTools::ResetGlStateBeforeOcct(myView);

  // flush pending input events and redraw the viewer
  Handle(V3d_View) aView = !myFocusView.IsNull() ? myFocusView : myView;
  aView->InvalidateImmediate();
  AIS_ViewController::FlushViewEvents(myContext, aView, true);

  // reset global GL state after OCCT before redrawing Qt
  OcctGlTools::ResetGlStateAfterOcct(myView);
}

// ================================================================
// Function : handleViewRedraw
// ================================================================
void OcctQOpenGLWidgetViewer::handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                               const Handle(V3d_View)&               theView)
{
  AIS_ViewController::handleViewRedraw(theCtx, theView);
  if (myToAskNextFrame)
    updateView(); // ask more frames for animation
}

#if (OCC_VERSION_HEX >= 0x070700)
// ================================================================
// Function : OnSubviewChanged
// ================================================================
void OcctQOpenGLWidgetViewer::OnSubviewChanged(const Handle(AIS_InteractiveContext)&,
                                               const Handle(V3d_View)&,
                                               const Handle(V3d_View)& theNewView)
{
  myFocusView = theNewView;
}
#endif
