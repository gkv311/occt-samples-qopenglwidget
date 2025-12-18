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
#include <BRepPrimAPI_MakeBox.hxx>
#include <Message.hxx>
#include <OpenGl_GraphicDriver.hxx>

#if !defined(__APPLE__) && !defined(_WIN32) && defined(__has_include)
  #if __has_include(<Xw_DisplayConnection.hxx>)
    #include <Xw_DisplayConnection.hxx>
    #define USE_XW_DISPLAY
  #endif
#endif
#ifndef USE_XW_DISPLAY
typedef Aspect_DisplayConnection Xw_DisplayConnection;
#endif

// ================================================================
// Function : OcctQOpenGLWidgetViewer
// ================================================================
OcctQOpenGLWidgetViewer::OcctQOpenGLWidgetViewer(QWidget* theParent)
    : QOpenGLWidget(theParent)
{
  Handle(Aspect_DisplayConnection) aDisp   = new Xw_DisplayConnection();
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
  myView->ChangeRenderingParams().ToShowStats = true;
  // NOLINTNEXTLINE
  myView->ChangeRenderingParams().CollectedStats = (Graphic3d_RenderingParams::PerfCounters)(
    Graphic3d_RenderingParams::PerfCounters_FrameRate | Graphic3d_RenderingParams::PerfCounters_Triangles);

  // Qt widget setup
  setAttribute(Qt::WA_AcceptTouchEvents); // necessary to receive QTouchEvent events
  setMouseTracking(true);
  setBackgroundRole(QPalette::NoRole); // or NoBackground
  setFocusPolicy(Qt::StrongFocus);     // set focus policy to threat QContextMenuEvent from keyboard
  setUpdatesEnabled(true);
  setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

  // OpenGL setup managed by Qt - it is better to do this globally
  // via QSurfaceFormat::setDefaultFormat() - see main() function
  //const QSurfaceFormat aGlFormat = OcctQtTools::qtGlSurfaceFormat();
  //setFormat(aGlFormat);
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
// Function : event
// ================================================================
bool OcctQOpenGLWidgetViewer::event(QEvent* theEvent)
{
  if (myView.IsNull())
    return QOpenGLWidget::event(theEvent);

  if (theEvent->type() == QEvent::TouchBegin
   || theEvent->type() == QEvent::TouchUpdate
   || theEvent->type() == QEvent::TouchEnd)
  {
    theEvent->accept();
    myHasTouchInput = true;
    if (OcctQtTools::qtHandleTouchEvent(*this, myView, static_cast<QTouchEvent*>(theEvent)))
      updateView();

    return true;
  }

  return QOpenGLWidget::event(theEvent);
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
      theEvent->accept();
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

  theEvent->accept();
  if (OcctQtTools::qtHandleMouseEvent(*this, myView, theEvent))
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

  theEvent->accept();
  if (OcctQtTools::qtHandleMouseEvent(*this, myView, theEvent))
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

  theEvent->accept();
  if (OcctQtTools::qtHandleMouseEvent(*this, myView, theEvent))
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

  theEvent->accept();

#if (OCC_VERSION_HEX >= 0x070700)
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  const Graphic3d_Vec2d aPnt2d(theEvent->position().x(), theEvent->position().y());
#else
  const Graphic3d_Vec2d aPnt2d(theEvent->pos().x(), theEvent->pos().y());
#endif
  const Graphic3d_Vec2i aPnt2i(myView->Window()->ConvertPointToBacking(aPnt2d) + Graphic3d_Vec2d(0.5));
  if (!myView->Subviews().IsEmpty())
  {
    Handle(V3d_View) aPickedView = myView->PickSubview(aPnt2i);
    if (!aPickedView.IsNull() && aPickedView != myFocusView)
    {
      // switch input focus to another subview
      OnSubviewChanged(myContext, myFocusView, aPickedView);
      updateView();
      return;
    }
  }
#endif

  if (OcctQtTools::qtHandleWheelEvent(*this, myView, theEvent))
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
  Handle(OpenGl_GraphicDriver) aDriver = Handle(OpenGl_GraphicDriver)::DownCast(myViewer->Driver());
  OcctQtTools::qtGlCapsFromSurfaceFormat(aDriver->ChangeOptions(), format());

  const Aspect_Drawable aNativeWin = (Aspect_Drawable)effectiveWinId();
  const Graphic3d_Vec2i aViewSize(rect().right() - rect().left(), rect().bottom() - rect().top());

  const bool isFirstInit = myView->Window().IsNull();
  if (!OcctGlTools::InitializeGlWindow(myView, aNativeWin, aViewSize, devicePixelRatioF()))
  {
    QMessageBox::critical(0, "Failure", "OpenGl_Context is unable to wrap OpenGL context");
    QApplication::exit(1);
    return;
  }

  makeCurrent(); // restore Qt framebuffer
  dumpGlInfo(true, true);
  if (isFirstInit)
  {
    myContext->Display(myViewCube, 0, 0, false);

    // dummy shape for testing
    TopoDS_Shape      aBox   = BRepPrimAPI_MakeBox(100.0, 50.0, 90.0).Shape();
    Handle(AIS_Shape) aShape = new AIS_Shape(aBox);
    myContext->Display(aShape, AIS_Shaded, 0, false);
  }
}

// ================================================================
// Function : paintGL
// ================================================================
void OcctQOpenGLWidgetViewer::paintGL()
{
  if (myView.IsNull() || myView->Window().IsNull())
    return;

  const double aDevPixelRatioOld = myView->Window()->DevicePixelRatio();
  if (myView->Window()->NativeHandle() != OcctGlTools::GetGlNativeWindow((Aspect_Drawable)effectiveWinId()))
  {
    // workaround window recreation done by Qt on monitor (QScreen) disconnection
    Message::SendWarning() << "Native window handle has changed by QOpenGLWidget!";
    initializeGL();
  }
  else if (devicePixelRatioF() != aDevPixelRatioOld)
  {
    initializeGL();
  }

  Graphic3d_Vec2i aViewSizeOld; myView->Window()->Size(aViewSizeOld.x(), aViewSizeOld.y());

  // wrap FBO created by QOpenGLFramebufferObject
  if (!OcctGlTools::InitializeGlFbo(myView))
  {
    QMessageBox::critical(0, "Failure", "Default FBO wrapper creation failed");
    QApplication::exit(1);
    return;
  }

  Graphic3d_Vec2i aViewSizeNew; myView->Window()->Size(aViewSizeNew.x(), aViewSizeNew.y());
  if (aViewSizeNew != aViewSizeOld || myView->Window()->DevicePixelRatio() != aDevPixelRatioOld)
    dumpGlInfo(true, false);

  // reset global GL state from Qt before redrawing OCCT
  OcctGlTools::ResetGlStateBeforeOcct(myView);

  // flush pending input events and redraw the viewer
  Handle(V3d_View) aView = !myFocusView.IsNull() ? myFocusView : myView;
  aView->InvalidateImmediate();
  AIS_ViewController::FlushViewEvents(myContext, aView, true);

  // reset global GL state after OCCT before redrawing Qt
  OcctGlTools::ResetGlStateAfterOcct(myView);
}
