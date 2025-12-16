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
// Function : OcctQQuickFramebufferViewer
// ================================================================
OcctQQuickFramebufferViewer::OcctQQuickFramebufferViewer(QQuickItem* theParent)
    : QQuickFramebufferObject(theParent)
{
  Handle(Aspect_DisplayConnection) aDisp   = new Aspect_DisplayConnection();
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
  myView->ChangeRenderingParams().ToShowStats    = true;
  myView->ChangeRenderingParams().CollectedStats = (Graphic3d_RenderingParams::PerfCounters)(
    Graphic3d_RenderingParams::PerfCounters_FrameRate | Graphic3d_RenderingParams::PerfCounters_Triangles);

  // QtQuick item setup
  setAcceptedMouseButtons(Qt::AllButtons);
  //setAcceptTouchEvents(true); // necessary to recieve QTouchEvent events
  setAcceptHoverEvents(true);
  setMirrorVertically(true);
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

  const bool isTouch = theEvent->type() == QEvent::TouchBegin
                    || theEvent->type() == QEvent::TouchUpdate
                    || theEvent->type() == QEvent::TouchEnd;
  if (!isTouch)
    return QQuickFramebufferObject::event(theEvent);

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
  const Graphic3d_Vec2i  aPnt(theEvent->pos().x(), theEvent->pos().y());
  const Aspect_VKeyMouse aButtons = OcctQtTools::qtMouseButtons2VKeys(theEvent->buttons());
  const Aspect_VKeyFlags aFlags = OcctQtTools::qtMouseModifiers2VKeys(theEvent->modifiers());
  if (AIS_ViewController::UpdateMouseButtons(aPnt, aButtons, aFlags, false))
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
  const Graphic3d_Vec2i  aPnt(theEvent->pos().x(), theEvent->pos().y());
  const Aspect_VKeyMouse aButtons = OcctQtTools::qtMouseButtons2VKeys(theEvent->buttons());
  const Aspect_VKeyFlags aFlags = OcctQtTools::qtMouseModifiers2VKeys(theEvent->modifiers());
  if (AIS_ViewController::UpdateMouseButtons(aPnt, aButtons, aFlags, false))
    updateView();
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
  const Graphic3d_Vec2i  aNewPos(theEvent->pos().x(), theEvent->pos().y());
  const Aspect_VKeyMouse aButtons = OcctQtTools::qtMouseButtons2VKeys(theEvent->buttons());
  const Aspect_VKeyFlags aFlags = OcctQtTools::qtMouseModifiers2VKeys(theEvent->modifiers());
  if (AIS_ViewController::UpdateMousePosition(aNewPos, aButtons, aFlags, false))
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  const Graphic3d_Vec2i aPos(Graphic3d_Vec2d(theEvent->position().x(), theEvent->position().y()));
#else
  const Graphic3d_Vec2i aPos(theEvent->pos().x(), theEvent->pos().y());
#endif
  if (AIS_ViewController::UpdateZoom(Aspect_ScrollDelta(aPos, double(theEvent->angleDelta().y()) / 8.0)))
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
  const Graphic3d_Vec2i  aNewPos(theEvent->pos().x(), theEvent->pos().y());
  const Aspect_VKeyMouse aButtons = Aspect_VKeyMouse_NONE;
  const Aspect_VKeyFlags aFlags = OcctQtTools::qtMouseModifiers2VKeys(theEvent->modifiers());
  if (AIS_ViewController::UpdateMousePosition(aNewPos, aButtons, aFlags, false))
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
  //aQFormat.setInternalTextureFormat(isSrgb ? GL_SRGB8_ALPHA8 ? GL_RGBA8);
  //aQFormat.setSamples(4); do not create MSAA buffer here
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
// Function : synchronize
// ================================================================
void OcctQQuickFramebufferViewer::synchronize(QOpenGLFramebufferObject* theFbo)
{
  // this method will be called from GL rendering thread while GUI thread is locked,
  // the place to sycnhronize GUI / GL rendering states

  if (myView.IsNull() || myView->Window().IsNull())
    initializeGL(theFbo);
}

// ================================================================
// Function : initializeGL
// ================================================================
void OcctQQuickFramebufferViewer::initializeGL(QOpenGLFramebufferObject* theFbo)
{
  // handle OCCT 3D Viewer initialization
  if (theFbo == nullptr)
    return;

  const QQuickWindow*   aQWindow = window();
  const QSize           aRect    = theFbo->size();
  const Graphic3d_Vec2i aViewSize(aRect.width(), aRect.height());

  Aspect_Drawable aNativeWin = aQWindow != nullptr ? (Aspect_Drawable)aQWindow->winId() : 0;
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

  Standard_Mutex::Sentry aLock(myViewerMutex);
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
// Function : render
// ================================================================
void OcctQQuickFramebufferViewer::render(QOpenGLFramebufferObject* theFbo)
{
  // this method is called from GL rendering thread;
  // accessing GUI items is not allowed here!
  if (theFbo == nullptr || myView.IsNull() || myView->Window().IsNull())
    return;

  QQuickWindow*   aQWindow   = window();
  Aspect_Drawable aNativeWin = aQWindow != nullptr ? (Aspect_Drawable)aQWindow->winId() : 0;
#ifdef _WIN32
  HDC  aWglDevCtx = wglGetCurrentDC();
  HWND aWglWin    = WindowFromDC(aWglDevCtx);
  aNativeWin      = (Aspect_Drawable)aWglWin;
#endif

  Standard_Mutex::Sentry aLock(myViewerMutex);
  if (myView->Window()->NativeHandle() != aNativeWin)
  {
    // workaround window recreation done by Qt on monitor (QScreen) disconnection
    Message::SendWarning() << "Native window handle has changed by QQuickWindow!";
    synchronize(theFbo);
    return;
  }

  // wrap FBO created by QOpenGLFramebufferObject
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
  const Graphic3d_Vec2i        aViewSizeNew = aDefaultFbo->GetVPSize();
  Handle(Aspect_NeutralWindow) aWindow      = Handle(Aspect_NeutralWindow)::DownCast(myView->Window());
  aWindow->Size(aViewSizeOld.x(), aViewSizeOld.y());
  if (aViewSizeNew != aViewSizeOld)
  {
    aWindow->SetSize(aViewSizeNew.x(), aViewSizeNew.y());
    myView->MustBeResized();
    myView->Invalidate();
    dumpGlInfo(true, false);
  }

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
