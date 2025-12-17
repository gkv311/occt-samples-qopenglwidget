// Copyright (c) 2023 Kirill Gavrilov

#ifdef _WIN32
  // should be included before other headers to avoid missing definitions
  #include <windows.h>
#endif
#include <OpenGl_Context.hxx>

#include "OcctQQuickFramebufferViewer.h"

#include "../occt-qt-tools/OcctGlTools.h"

#include <Standard_WarningsDisable.hxx>
#include <QApplication>
#include <QMessageBox>
#include <QMouseEvent>
#include <QOpenGLFramebufferObject>
#include <QQuickWindow>
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
// Function : OcctQQuickFramebufferViewer
// ================================================================
OcctQQuickFramebufferViewer::OcctQQuickFramebufferViewer(QQuickItem* theParent)
    : QQuickFramebufferObject(theParent)
{
  Handle(Aspect_DisplayConnection) aDisp   = new Xw_DisplayConnection();
  Handle(OpenGl_GraphicDriver)     aDriver = new OpenGl_GraphicDriver(aDisp, false);
  // lets QtQuick to manage buffer swap
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

  // QtQuick item setup
  setAcceptedMouseButtons(Qt::AllButtons);
  //setAcceptTouchEvents(true); // necessary to receive QTouchEvent events
  setAcceptHoverEvents(true);
  setMirrorVertically(true);

  // GUI elements cannot be created from GL rendering thread - make queued connection
  connect(this, &OcctQQuickFramebufferViewer::glCriticalError, this, [this](QString theMsg)
  {
    QMessageBox::critical(0, "Critical error", theMsg);
    QApplication::exit(1);
  }, Qt::QueuedConnection);
}

// ================================================================
// Function : ~OcctQQuickFramebufferViewer
// ================================================================
OcctQQuickFramebufferViewer::~OcctQQuickFramebufferViewer()
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
  //makeCurrent();
  aDisp.Nullify();
}

// ================================================================
// Function : event
// ================================================================
bool OcctQQuickFramebufferViewer::event(QEvent* theEvent)
{
  if (myView.IsNull())
    return QQuickFramebufferObject::event(theEvent);

  if (theEvent->type() == QEvent::UpdateLater)
  {
    update();
    theEvent->accept();
    return true;
  }

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

  return QQuickFramebufferObject::event(theEvent);
}

// ================================================================
// Function : keyPressEvent
// ================================================================
void OcctQQuickFramebufferViewer::keyPressEvent(QKeyEvent* theEvent)
{
  if (myView.IsNull())
    return;

  const Aspect_VKey aKey = OcctQtTools::qtKey2VKey(theEvent->key());
  switch (aKey)
  {
    case Aspect_VKey_Escape:
    {
      QApplication::exit();
      return;
    }
    case Aspect_VKey_F:
    {
      {
        Standard_Mutex::Sentry aLock(myViewerMutex);
        myView->FitAll(0.01, false);
      }
      update();
      theEvent->accept();
      return;
    }
  }
  QQuickFramebufferObject::keyPressEvent(theEvent);
}

// ================================================================
// Function : mousePressEvent
// ================================================================
void OcctQQuickFramebufferViewer::mousePressEvent(QMouseEvent* theEvent)
{
  QQuickFramebufferObject::mousePressEvent(theEvent);
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
void OcctQQuickFramebufferViewer::mouseReleaseEvent(QMouseEvent* theEvent)
{
  QQuickFramebufferObject::mouseReleaseEvent(theEvent);
  if (myView.IsNull())
    return;

  theEvent->accept();
  if (OcctQtTools::qtHandleMouseEvent(*this, myView, theEvent))
    updateView();

  // take keyboard focus on mouse click
  setFocus(true);
}

// ================================================================
// Function : mouseMoveEvent
// ================================================================
void OcctQQuickFramebufferViewer::mouseMoveEvent(QMouseEvent* theEvent)
{
  QQuickFramebufferObject::mouseMoveEvent(theEvent);
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
void OcctQQuickFramebufferViewer::wheelEvent(QWheelEvent* theEvent)
{
  QQuickFramebufferObject::wheelEvent(theEvent);
  if (myView.IsNull())
    return;

  theEvent->accept();
  if (OcctQtTools::qtHandleWheelEvent(*this, myView, theEvent))
    updateView();
}

// ================================================================
// Function : hoverMoveEvent
// ================================================================
void OcctQQuickFramebufferViewer::hoverEnterEvent(QHoverEvent* theEvent)
{
  QQuickFramebufferObject::hoverEnterEvent(theEvent);
  if (myView.IsNull())
    return;

  theEvent->accept();
}

// ================================================================
// Function : hoverMoveEvent
// ================================================================
void OcctQQuickFramebufferViewer::hoverLeaveEvent(QHoverEvent* theEvent)
{
  QQuickFramebufferObject::hoverLeaveEvent(theEvent);
  if (myView.IsNull())
    return;

  theEvent->accept();
}

// ================================================================
// Function : hoverMoveEvent
// ================================================================
void OcctQQuickFramebufferViewer::hoverMoveEvent(QHoverEvent* theEvent)
{
  QQuickFramebufferObject::hoverMoveEvent(theEvent);
  if (myView.IsNull())
    return;

  theEvent->accept();
  if (OcctQtTools::qtHandleHoverEvent(*this, myView, theEvent))
    updateView();
}

// =======================================================================
// Function : updateView
// =======================================================================
void OcctQQuickFramebufferViewer::updateView()
{
  update();
}

// ================================================================
// Function : handleViewRedraw
// ================================================================
void OcctQQuickFramebufferViewer::handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                                   const Handle(V3d_View)&               theView)
{
  AIS_ViewController::handleViewRedraw(theCtx, theView);
  if (myToAskNextFrame)
    QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateLater)); // ask more frames for animation
}

// =======================================================================
// Function : createRenderer
// =======================================================================
QQuickFramebufferObject::Renderer* OcctQQuickFramebufferViewer::createRenderer() const
{
  return new Renderer(const_cast<OcctQQuickFramebufferViewer*>(this));
}

// ================================================================
// Function : Renderer
// ================================================================
OcctQQuickFramebufferViewer::Renderer::Renderer(OcctQQuickFramebufferViewer* theViewer)
: myViewer(theViewer)
{
  //
}

// ================================================================
// Function : ~Renderer
// ================================================================
OcctQQuickFramebufferViewer::Renderer::~Renderer()
{
  //
}

// ================================================================
// Function : createFramebufferObject
// ================================================================
QOpenGLFramebufferObject* OcctQQuickFramebufferViewer::Renderer::createFramebufferObject(const QSize& theSize)
{
  QOpenGLFramebufferObjectFormat aQFormat;
  aQFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
  //aQFormat.setInternalTextureFormat(GL_RGBA8);
  //aQFormat.setSamples(4); do not create MSAA buffer here

/*#if (QT_VERSION_MAJOR > 5) || (QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR >= 10)
  const QQuickWindow*  aQWindow = window();
  const QSurfaceFormat aQtGlFormat = aQWindow->format();
  if (aQtGlFormat.colorSpace() == QSurfaceFormat::sRGBColorSpace)
    aQFormat.setInternalTextureFormat(GL_SRGB8_ALPHA8);
#endif*/

  return new QOpenGLFramebufferObject(theSize, aQFormat);
}

// ================================================================
// Function : synchronize
// ================================================================
void OcctQQuickFramebufferViewer::Renderer::synchronize(QQuickFramebufferObject* theItem)
{
  if (myViewer != theItem)
    std::cerr << "Error: unexpected synchronize\n";

  if (myViewer != nullptr)
    myViewer->synchronize(framebufferObject());
}

// ================================================================
// Function : render
// ================================================================
void OcctQQuickFramebufferViewer::Renderer::render()
{
  if (myViewer != nullptr)
    myViewer->render(framebufferObject());
}

// ================================================================
// Function : dumpGlInfo
// ================================================================
void OcctQQuickFramebufferViewer::dumpGlInfo(bool theIsBasic, bool theToPrint)
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
  Q_EMIT glInfoChanged();
}

// ================================================================
// Function : synchronize
// ================================================================
void OcctQQuickFramebufferViewer::synchronize(QOpenGLFramebufferObject* )
{
  // this method will be called from GL rendering thread while GUI thread is locked,
  // the place to synchronize GUI / GL rendering states
  if (myGlBackColor.first)
  {
    myGlBackColor.first = false;
    const Quantity_Color aColor = OcctQtTools::qtColorToOcct(myGlBackColor.second);
    myView->SetBgGradientColors(aColor, Quantity_NOC_BLACK, Aspect_GradientFillMethod_Elliptical);
    myView->Invalidate();
  }
}

// ================================================================
// Function : initializeGL
// ================================================================
void OcctQQuickFramebufferViewer::initializeGL(QOpenGLFramebufferObject* theFbo)
{
  // handle OCCT 3D Viewer initialization
  Standard_Mutex::Sentry aLock(myViewerMutex);
  const QQuickWindow* aQWindow = window();
  if (theFbo == nullptr)
    return;

  Handle(OpenGl_GraphicDriver) aDriver = Handle(OpenGl_GraphicDriver)::DownCast(myViewer->Driver());
  OcctQtTools::qtGlCapsFromSurfaceFormat(aDriver->ChangeOptions(), aQWindow->format());

  const Aspect_Drawable aNativeWin = aQWindow != nullptr ? (Aspect_Drawable)aQWindow->winId() : 0;
  const Graphic3d_Vec2i aViewSize(theFbo->size().width(), theFbo->size().height());

  const bool isFirstInit = myView->Window().IsNull();
  if (!OcctGlTools::InitializeGlWindow(myView, aNativeWin, aViewSize, aQWindow->devicePixelRatio()))
  {
    Q_EMIT glCriticalError("OpenGl_Context is unable to wrap OpenGL context");
    return;
  }

  theFbo->bind();
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
// Function : render
// ================================================================
void OcctQQuickFramebufferViewer::render(QOpenGLFramebufferObject* theFbo)
{
  // this method is called from GL rendering thread;
  // accessing GUI items is not allowed here!
  Standard_Mutex::Sentry aLock(myViewerMutex);
  const QQuickWindow* aQWindow = window();
  if (theFbo == nullptr || myView.IsNull() || aQWindow == nullptr)
    return;

  const Aspect_Drawable aNativeWin = OcctGlTools::GetGlNativeWindow((Aspect_Drawable)aQWindow->winId());
  if (myView->Window().IsNull()
   || myView->Window()->NativeHandle() != aNativeWin)
  {
    initializeGL(theFbo);
    if (myView->Window().IsNull())
      return;
  }

  Graphic3d_Vec2i aViewSizeOld; myView->Window()->Size(aViewSizeOld.x(), aViewSizeOld.y());
  const double aDevPixelRatioOld = myView->Window()->DevicePixelRatio();
  if (aDevPixelRatioOld != aQWindow->devicePixelRatio())
    initializeGL(theFbo);

  // wrap FBO created by QOpenGLFramebufferObject
  if (!OcctGlTools::InitializeGlFbo(myView))
  {
    Q_EMIT glCriticalError("Default FBO wrapper creation failed");
    return;
  }

  Graphic3d_Vec2i aViewSizeNew; myView->Window()->Size(aViewSizeNew.x(), aViewSizeNew.y());
  if (aViewSizeNew != aViewSizeOld || myView->Window()->DevicePixelRatio() != aDevPixelRatioOld)
    dumpGlInfo(true, false);

  // reset global GL state from Qt before redrawing OCCT
  OcctGlTools::ResetGlStateBeforeOcct(myView);

  // flush pending input events and redraw the viewer
  myView->InvalidateImmediate();
  AIS_ViewController::FlushViewEvents(myContext, myView, true);

  // reset global GL state after OCCT before redrawing Qt
  // (alternative to QQuickOpenGLUtils::resetOpenGLState())
  OcctGlTools::ResetGlStateAfterOcct(myView);
/*#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  QQuickOpenGLUtils::resetOpenGLState()
#else
  if (aQWindow != nullptr)
    aQWindow->resetOpenGLState();
#endif*/
}
