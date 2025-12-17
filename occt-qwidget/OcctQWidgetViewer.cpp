// Copyright (c) 2025 Kirill Gavrilov

#ifdef _WIN32
  // should be included before other headers to avoid missing definitions
  #include <windows.h>
#endif

#include "OcctQWidgetViewer.h"

#include "../occt-qt-tools/OcctQtTools.h"
#include "../occt-qt-tools/OcctGlTools.h"

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
// Function : OcctQWidgetViewer
// ================================================================
OcctQWidgetViewer::OcctQWidgetViewer(QWidget* theParent)
    : QWidget(theParent)
{
  Handle(Aspect_DisplayConnection) aDisp   = new Xw_DisplayConnection();
  Handle(OpenGl_GraphicDriver)     aDriver = new OpenGl_GraphicDriver(aDisp, false);

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
  setAttribute(Qt::WA_PaintOnScreen);
  setAttribute(Qt::WA_NoSystemBackground);
  setAttribute(Qt::WA_NativeWindow);      // request native window for this widget to create OpenGL context
  setAttribute(Qt::WA_AcceptTouchEvents); // necessary to receive QTouchEvent events
  setMouseTracking(true);
  setBackgroundRole(QPalette::NoRole); // or NoBackground
  setFocusPolicy(Qt::StrongFocus);     // set focus policy to threat QContextMenuEvent from keyboard
  setUpdatesEnabled(true);

  initializeGL();
}

// ================================================================
// Function : ~OcctQWidgetViewer
// ================================================================
OcctQWidgetViewer::~OcctQWidgetViewer()
{
  Handle(Aspect_DisplayConnection) aDisp = myViewer->Driver()->GetDisplayConnection();

  // release OCCT viewer
  myContext->RemoveAll(false);
  myContext.Nullify();
  myView->Remove();
  myView.Nullify();
  myViewer.Nullify();

  aDisp.Nullify();
}

// ================================================================
// Function : event
// ================================================================
bool OcctQWidgetViewer::event(QEvent* theEvent)
{
  if (myView.IsNull())
    return QWidget::event(theEvent);

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

  return QWidget::event(theEvent);
}

// ================================================================
// Function : closeEvent
// ================================================================
void OcctQWidgetViewer::closeEvent(QCloseEvent* theEvent)
{
  theEvent->accept();
}

// ================================================================
// Function : keyPressEvent
// ================================================================
void OcctQWidgetViewer::keyPressEvent(QKeyEvent* theEvent)
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
      myView->FitAll(0.01, false);
      update();
      theEvent->accept();
      return;
    }
  }
  QWidget::keyPressEvent(theEvent);
}

// ================================================================
// Function : mousePressEvent
// ================================================================
void OcctQWidgetViewer::mousePressEvent(QMouseEvent* theEvent)
{
  QWidget::mousePressEvent(theEvent);
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
void OcctQWidgetViewer::mouseReleaseEvent(QMouseEvent* theEvent)
{
  QWidget::mouseReleaseEvent(theEvent);
  if (myView.IsNull())
    return;

  theEvent->accept();
  if (OcctQtTools::qtHandleMouseEvent(*this, myView, theEvent))
    updateView();
}

// ================================================================
// Function : mouseMoveEvent
// ================================================================
void OcctQWidgetViewer::mouseMoveEvent(QMouseEvent* theEvent)
{
  QWidget::mouseMoveEvent(theEvent);
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
void OcctQWidgetViewer::wheelEvent(QWheelEvent* theEvent)
{
  QWidget::wheelEvent(theEvent);
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
void OcctQWidgetViewer::updateView()
{
  QWidget::update();
  // if (window() != NULL) { window()->update(); }
}

// ================================================================
// Function : resizeEvent
// ================================================================
void OcctQWidgetViewer::resizeEvent(QResizeEvent* )
{
  if (!myView.IsNull())
    myView->MustBeResized();
}

// ================================================================
// Function : handleViewRedraw
// ================================================================
void OcctQWidgetViewer::handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx, const Handle(V3d_View)& theView)
{
  AIS_ViewController::handleViewRedraw(theCtx, theView);
  if (myToAskNextFrame)
    updateView(); // ask more frames for animation
}

#if (OCC_VERSION_HEX >= 0x070700)
// ================================================================
// Function : OnSubviewChanged
// ================================================================
void OcctQWidgetViewer::OnSubviewChanged(const Handle(AIS_InteractiveContext)&,
                                         const Handle(V3d_View)&,
                                         const Handle(V3d_View)& theNewView)
{
  myFocusView = theNewView;
}
#endif

// ================================================================
// Function : dumpGlInfo
// ================================================================
void OcctQWidgetViewer::dumpGlInfo(bool theIsBasic, bool theToPrint)
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
void OcctQWidgetViewer::initializeGL()
{
  const QRect  aRect        = rect();
  const double aDevPixRatio = devicePixelRatioF();
  const Graphic3d_Vec2i aViewSize(Graphic3d_Vec2d(Round((aRect.right() - aRect.left()) * aDevPixRatio),
                                                  Round((aRect.bottom() - aRect.top()) * aDevPixRatio)));

  const Aspect_Drawable aNativeWin = (Aspect_Drawable)winId();

  Handle(OcctGlTools::OcctNeutralWindow) aWindow = Handle(OcctGlTools::OcctNeutralWindow)::DownCast(myView->Window());
  const bool isFirstInit = aWindow.IsNull();
  if (aWindow.IsNull())
  {
    aWindow = new OcctGlTools::OcctNeutralWindow();
    aWindow->SetVirtual(true);
  }
  aWindow->SetNativeHandle(aNativeWin);
  aWindow->SetSize(aViewSize.x(), aViewSize.y());
  aWindow->SetDevicePixelRatio(aDevPixRatio);
  myView->SetWindow(aWindow);
  dumpGlInfo(true, true);
#if (OCC_VERSION_HEX >= 0x070700)
  for (const Handle(V3d_View)& aSubviewIter : myView->Subviews())
  {
    aSubviewIter->MustBeResized();
    aSubviewIter->Invalidate();
  }
#endif

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
// Function : paintEvent
// ================================================================
void OcctQWidgetViewer::paintEvent(QPaintEvent* )
{
  if (myView.IsNull() || myView->Window().IsNull())
    return;

  const double aDevPixelRatioOld = myView->Window()->DevicePixelRatio();
  const double aDevPixelRatioNew   = devicePixelRatioF();
  if (myView->Window()->NativeHandle() != (Aspect_Drawable)winId())
  {
    // workaround window recreation done by Qt on monitor (QScreen) disconnection
    Message::SendWarning() << "Native window handle has changed by QWidget!";
    initializeGL();
  }
  else if (aDevPixelRatioNew != aDevPixelRatioOld)
  {
    initializeGL();
  }
  else
  {
    Graphic3d_Vec2i aViewSizeOld; myView->Window()->Size(aViewSizeOld.x(), aViewSizeOld.y());

    const QRect     aRect = rect();
    Graphic3d_Vec2i aViewSizeNew(Graphic3d_Vec2d(Round((aRect.right() - aRect.left()) * aDevPixelRatioNew),
                                                 Round((aRect.bottom() - aRect.top()) * aDevPixelRatioNew)));
    if (aViewSizeNew != aViewSizeOld)
    {
      Handle(OcctGlTools::OcctNeutralWindow) aWindow = Handle(OcctGlTools::OcctNeutralWindow)::DownCast(myView->Window());
      aWindow->SetSize(aViewSizeNew.x(), aViewSizeNew.y());
      myView->MustBeResized();
      myView->Invalidate();
      dumpGlInfo(true, false);

#if (OCC_VERSION_HEX >= 0x070700)
      for (const Handle(V3d_View)& aSubviewIter : myView->Subviews())
      {
        aSubviewIter->MustBeResized();
        aSubviewIter->Invalidate();
      }
#endif
    }
  }

  // flush pending input events and redraw the viewer
  Handle(V3d_View) aView = !myFocusView.IsNull() ? myFocusView : myView;
  aView->InvalidateImmediate();
  AIS_ViewController::FlushViewEvents(myContext, aView, true);
}
