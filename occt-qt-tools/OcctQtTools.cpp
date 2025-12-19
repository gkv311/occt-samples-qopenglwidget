// Copyright (c) 2025 Kirill Gavrilov

#include "OcctQtTools.h"

#include <Aspect_ScrollDelta.hxx>
#include <Message.hxx>
#include <OpenGl_Caps.hxx>
#include <OSD_Environment.hxx>
#include <Standard_Version.hxx>
#include <V3d_View.hxx>

#include <Standard_WarningsDisable.hxx>
#include <QCoreApplication>
#include <QGuiApplication>
#include <Standard_WarningsRestore.hxx>

// ================================================================
// Function : qtColorToOcct
// ================================================================
Quantity_Color OcctQtTools::qtColorToOcct(const QColor& theColor)
{
  return Quantity_Color(theColor.red() / 255.0, theColor.green() / 255.0, theColor.blue() / 255.0, Quantity_TOC_sRGB);
}

// ================================================================
// Function : qtColorFromOcct
// ================================================================
QColor OcctQtTools::qtColorFromOcct(const Quantity_Color& theColor)
{
  NCollection_Vec3<double> anRgb; theColor.Values(anRgb.r(), anRgb.g(), anRgb.b(), Quantity_TOC_sRGB);
  return QColor((int)Round(anRgb.r() * 255.0), (int)Round(anRgb.g() * 255.0), (int)Round(anRgb.b() * 255.0));
}

// ================================================================
// Function : qtColorToOcctRgba
// ================================================================
Quantity_ColorRGBA OcctQtTools::qtColorToOcctRgba(const QColor& theColor)
{
  return Quantity_ColorRGBA(OcctQtTools::qtColorToOcct(theColor), theColor.alpha() / 255.0f);
}

// ================================================================
// Function : qtColorFromOcctRgba
// ================================================================
QColor OcctQtTools::qtColorFromOcctRgba(const Quantity_ColorRGBA& theColor)
{
  QColor aColor = OcctQtTools::qtColorFromOcct(theColor.GetRGB());
  aColor.setAlpha((int)Round(theColor.Alpha() * 255.0));
  return aColor;
}

// ================================================================
// Function : qtStringToOcct
// ================================================================
TCollection_AsciiString OcctQtTools::qtStringToOcct(const QString& theText)
{
  return TCollection_AsciiString(theText.toUtf8().data());
}

// ================================================================
// Function : qtStringFromOcct
// ================================================================
QString OcctQtTools::qtStringFromOcct(const TCollection_AsciiString& theText)
{
  return QString::fromUtf8(theText.ToCString());
}

// ================================================================
// Function : qtStringToOcctExt
// ================================================================
TCollection_ExtendedString OcctQtTools::qtStringToOcctExt(const QString& theText)
{
  return TCollection_ExtendedString(theText.toUtf8().data(), true);
}

// ================================================================
// Function : qtStringFromOcctExt
// ================================================================
QString OcctQtTools::qtStringFromOcctExt(const TCollection_ExtendedString& theText)
{
  return QString::fromUtf16(theText.ToExtString());
}

// ================================================================
// Function : qtMsgTypeToGravity
// ================================================================
Message_Gravity OcctQtTools::qtMsgTypeToGravity(QtMsgType theType)
{
  switch (theType)
  {
    case QtDebugMsg: return Message_Trace;
    case QtInfoMsg: return Message_Info;
    case QtWarningMsg: return Message_Warning;
    case QtCriticalMsg: return Message_Alarm;
    case QtFatalMsg: return Message_Fail;
    default: return Message_Fail;
  }
}

// ================================================================
// Function : qtMessageHandlerToOcct
// ================================================================
void OcctQtTools::qtMessageHandlerToOcct(QtMsgType theType,
                                         const QMessageLogContext& theCtx,
                                         const QString& theMsg)
{
  Message_Gravity aGravity = OcctQtTools::qtMsgTypeToGravity(theType);
  if (aGravity < (int)Message_Alarm)
    aGravity = Message_Trace; // lower gravity to trace for non-critical errors

  const QString aQMsg = qFormatLogMessage(theType, theCtx, theMsg);
  const TCollection_AsciiString aMsg = aQMsg.toUtf8().data();
  Message::Send(aMsg, aGravity);
}

// ================================================================
// Function : qtGlPlatformSetup
// ================================================================
void OcctQtTools::qtGlPlatformSetup()
{
#if defined(_WIN32)
  // never use ANGLE on Windows, since OCCT 3D Viewer does not expect this;
  // use Qt::AA_UseOpenGLES for embedded systems
  QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
#elif defined(__APPLE__)
  //
#else
  // Qt6 tries to use Wayland platform by default, which is incompatible with OCCT depending on Xlib;
  // Force 'xcb' platform plugin (alternatively, could be passed QApplication as '-platform xcb' argument).
  OSD_Environment aQpaPlat("QT_QPA_PLATFORM");
  if (aQpaPlat.Value().IsEmpty())
  {
    aQpaPlat.SetValue("xcb");
    aQpaPlat.Build();
  }
#endif

/*#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  // workaround for some bugs in Qt5
  OSD_Environment aQGlyph("QT_ENABLE_GLYPH_CACHE_WORKAROUND");
  if (aQGlyph.Value().IsEmpty())
  {
    aQGlyph.SetValue("1");
    aQGlyph.Build();
  }
#endif*/

  // global OpenGL setup managed by Qt
  const QSurfaceFormat aGlFormat = OcctQtTools::qtGlSurfaceFormat();
  QSurfaceFormat::setDefaultFormat(aGlFormat);

  // ask Qt managing rendering from GUI thread instead of QSGRenderThread
  // for QtQuick applications
  /*OSD_Environment aQsgLoop("QSG_RENDER_LOOP");
  if (aQsgLoop.Value().IsEmpty())
  {
    aQsgLoop.SetValue("basic");
    aQsgLoop.Build();
  }*/

  // enable auto-scaling for high-density screens and fractional scale factors
  // (this is default since Qt6)
#if (QT_VERSION_MAJOR == 5)
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#if (QT_VERSION_MINOR >= 14)
  QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
#endif

  // redirect Qt messages to OCCT
  /*static QtMessageHandler aDefHandler = qInstallMessageHandler([](QtMsgType theType,
                                                                  const QMessageLogContext& theCtx,
                                                                  const QString& theMsg){
    OcctQtTools::qtMessageHandlerToOcct(theType, theCtx, theMsg);
    //if (aDefHandler != nullptr) { aDefHandler(QtCriticalMsg, theCtx, theMsg); }
  });*/
}

// ================================================================
// Function : qtGlSurfaceFormat
// ================================================================
QSurfaceFormat OcctQtTools::qtGlSurfaceFormat(QSurfaceFormat::OpenGLContextProfile theProfile,
                                              bool theToDebug)
{
  const bool isDeepColor = false;
  QSurfaceFormat::OpenGLContextProfile aProfile = theProfile;
  if (theProfile == QSurfaceFormat::NoProfile)
  {
    aProfile = QSurfaceFormat::CompatibilityProfile;
#ifdef __APPLE__
    // suppress Qt warning "QCocoaGLContext: Falling back to unshared context"
    aProfile = QSurfaceFormat::CoreProfile;
#endif
  }

  QSurfaceFormat aGlFormat;
  if (isDeepColor)
  {
    aGlFormat.setRedBufferSize(10);
    aGlFormat.setGreenBufferSize(10);
    aGlFormat.setBlueBufferSize(10);
    aGlFormat.setAlphaBufferSize(2);
  }
  aGlFormat.setDepthBufferSize(24);
  aGlFormat.setStencilBufferSize(8);
  aGlFormat.setProfile(aProfile);
  if (aProfile == QSurfaceFormat::CoreProfile)
    aGlFormat.setVersion(4, 5);

  // request sRGBColorSpace color-space to meet OCCT expectations or use OcctQtFrameBuffer fallback.
  /*#if (QT_VERSION_MAJOR > 5) || (QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR >= 10)
    aGlFormat.setColorSpace(QSurfaceFormat::sRGBColorSpace);
  #endif*/

  if (theToDebug)
    aGlFormat.setOption(QSurfaceFormat::DebugContext, true);

  return aGlFormat;
}

// ================================================================
// Function : qtGlCapsFromSurfaceFormat
// ================================================================
void OcctQtTools::qtGlCapsFromSurfaceFormat(OpenGl_Caps& theCaps, const QSurfaceFormat& theFormat)
{
  theCaps.contextDebug = theFormat.testOption(QSurfaceFormat::DebugContext);
  theCaps.contextSyncDebug = theCaps.contextDebug;
  theCaps.contextCompatible = theFormat.profile() != QSurfaceFormat::CoreProfile;
#if (OCC_VERSION_HEX >= 0x070700)
  theCaps.buffersDeepColor = theFormat.redBufferSize() == 10
                          && theFormat.greenBufferSize() == 10
                          && theFormat.blueBufferSize() == 10;
#endif
}

// ================================================================
// Function : qtHandleHoverEvent
// ================================================================
bool OcctQtTools::qtHandleHoverEvent(Aspect_WindowInputListener& theListener,
                                     const Handle(V3d_View)& theView,
                                     const QHoverEvent* theEvent)
{
  if (theView->Window().IsNull())
    return false;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  const Graphic3d_Vec2d aPnt2d(theEvent->position().x(), theEvent->position().y());
#else
  const Graphic3d_Vec2d aPnt2d(theEvent->pos().x(), theEvent->pos().y());
#endif
  const Graphic3d_Vec2i  aPnt2i(theView->Window()->ConvertPointToBacking(aPnt2d) + Graphic3d_Vec2d(0.5));
  const Aspect_VKeyMouse aButtons = Aspect_VKeyMouse_NONE;
  const Aspect_VKeyFlags aFlags = OcctQtTools::qtMouseModifiers2VKeys(theEvent->modifiers());
  return theListener.UpdateMousePosition(aPnt2i, aButtons, aFlags, false);
}

// ================================================================
// Function : qtHandleMouseEvent
// ================================================================
bool OcctQtTools::qtHandleMouseEvent(Aspect_WindowInputListener& theListener,
                                     const Handle(V3d_View)& theView,
                                     const QMouseEvent* theEvent)
{
  if (theView->Window().IsNull())
    return false;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  const Graphic3d_Vec2d aPnt2d(theEvent->position().x(), theEvent->position().y());
#else
  const Graphic3d_Vec2d aPnt2d(theEvent->pos().x(), theEvent->pos().y());
#endif
  const Graphic3d_Vec2i  aPnt2i(theView->Window()->ConvertPointToBacking(aPnt2d) + Graphic3d_Vec2d(0.5));
  const Aspect_VKeyMouse aButtons = OcctQtTools::qtMouseButtons2VKeys(theEvent->buttons());
  const Aspect_VKeyFlags aFlags = OcctQtTools::qtMouseModifiers2VKeys(theEvent->modifiers());
  if (theEvent->type() == QEvent::MouseMove)
    return theListener.UpdateMousePosition(aPnt2i, aButtons, aFlags, false);

  return theListener.UpdateMouseButtons(aPnt2i, aButtons, aFlags, false);
}

// ================================================================
// Function : qtHandleWheelEvent
// ================================================================
bool OcctQtTools::qtHandleWheelEvent(Aspect_WindowInputListener& theListener,
                                     const Handle(V3d_View)& theView,
                                     const QWheelEvent* theEvent)
{
  if (theView->Window().IsNull())
    return false;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  const Graphic3d_Vec2d aPnt2d(theEvent->position().x(), theEvent->position().y());
#else
  const Graphic3d_Vec2d aPnt2d(theEvent->pos().x(), theEvent->pos().y());
#endif
  const Graphic3d_Vec2i aPnt2i(theView->Window()->ConvertPointToBacking(aPnt2d) + Graphic3d_Vec2d(0.5));
  return theListener.UpdateMouseScroll(Aspect_ScrollDelta(aPnt2i, double(theEvent->angleDelta().y()) / 120.0));
}

// ================================================================
// Function : qtHandleTouchEvent
// ================================================================
bool OcctQtTools::qtHandleTouchEvent(Aspect_WindowInputListener& theListener,
                                     const Handle(V3d_View)& theView,
                                     const QTouchEvent* theEvent)
{
  if (theView->Window().IsNull())
    return false;

  bool hasUpdates = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  for (const QTouchEvent::TouchPoint& aQTouch : theEvent->points())
  {
    const Standard_Size   aTouchId = aQTouch.id();
    const Graphic3d_Vec2d aNewPos2d =
      theView->Window()->ConvertPointToBacking(Graphic3d_Vec2d(aQTouch.position().x(), aQTouch.position().y()));
    const Graphic3d_Vec2i aNewPos2i = Graphic3d_Vec2i(aNewPos2d + Graphic3d_Vec2d(0.5));
    if (aQTouch.state() == QEventPoint::Pressed
     && aNewPos2i.minComp() >= 0)
    {
      hasUpdates = true;
      theListener.AddTouchPoint(aTouchId, aNewPos2d);
    }
    else if (aQTouch.state() == QEventPoint::Updated
          && theListener.TouchPoints().Contains(aTouchId))
    {
      hasUpdates = true;
      theListener.UpdateTouchPoint(aTouchId, aNewPos2d);
    }
    else if (aQTouch.state() == QEventPoint::Released
          && theListener.RemoveTouchPoint(aTouchId))
    {
      hasUpdates = true;
    }
  }
#else
  for (const QTouchEvent::TouchPoint& aQTouch : theEvent->touchPoints())
  {
    const Standard_Size   aTouchId = aQTouch.id();
    const Graphic3d_Vec2d aNewPos2d =
      theView->Window()->ConvertPointToBacking(Graphic3d_Vec2d(aQTouch.pos().x(), aQTouch.pos().y()));
    const Graphic3d_Vec2i aNewPos2i = Graphic3d_Vec2i(aNewPos2d + Graphic3d_Vec2d(0.5));
    if (aQTouch.state() == Qt::TouchPointPressed
     && aNewPos2i.minComp() >= 0)
    {
      hasUpdates = true;
      theListener.AddTouchPoint(aTouchId, aNewPos2d);
    }
    else if (aQTouch.state() == Qt::TouchPointMoved
          && theListener.TouchPoints().Contains(aTouchId))
    {
      hasUpdates = true;
      theListener.UpdateTouchPoint(aTouchId, aNewPos2d);
    }
    else if (aQTouch.state() == Qt::TouchPointReleased
          && theListener.RemoveTouchPoint(aTouchId))
    {
      hasUpdates = true;
    }
  }
#endif
  return hasUpdates;
}

// ================================================================
// Function : qtMouseButtons2VKeys
// ================================================================
Aspect_VKeyMouse OcctQtTools::qtMouseButtons2VKeys(Qt::MouseButtons theButtons)
{
  Aspect_VKeyMouse aButtons = Aspect_VKeyMouse_NONE;
  if ((theButtons & Qt::LeftButton) != 0)
    aButtons |= Aspect_VKeyMouse_LeftButton;

  if ((theButtons & Qt::MiddleButton) != 0)
    aButtons |= Aspect_VKeyMouse_MiddleButton;

  if ((theButtons & Qt::RightButton) != 0)
    aButtons |= Aspect_VKeyMouse_RightButton;

  return aButtons;
}

// ================================================================
// Function : qtMouseModifiers2VKeys
// ================================================================
Aspect_VKeyFlags OcctQtTools::qtMouseModifiers2VKeys(Qt::KeyboardModifiers theModifiers)
{
  Aspect_VKeyFlags aFlags = Aspect_VKeyFlags_NONE;
  if ((theModifiers & Qt::ShiftModifier) != 0)
    aFlags |= Aspect_VKeyFlags_SHIFT;

  if ((theModifiers & Qt::ControlModifier) != 0)
    aFlags |= Aspect_VKeyFlags_CTRL;

  if ((theModifiers & Qt::AltModifier) != 0)
    aFlags |= Aspect_VKeyFlags_ALT;

  return aFlags;
}

// ================================================================
// Function : qtKey2VKey
// ================================================================
Aspect_VKey OcctQtTools::qtKey2VKey(int theKey)
{
  switch (theKey)
  {
    case 1060: // ru
    case Qt::Key_A:
      return Aspect_VKey_A;
    case 1048: // ru
    case Qt::Key_B:
      return Aspect_VKey_B;
    case 1057: // ru
    case Qt::Key_C:
      return Aspect_VKey_C;
    case 1042: // ru
    case Qt::Key_D:
      return Aspect_VKey_D;
    case 1059: // ru
    case Qt::Key_E:
      return Aspect_VKey_E;
    case 1040: // ru
    case Qt::Key_F:
      return Aspect_VKey_F;
    case Qt::Key_G:
      return Aspect_VKey_G;
    case Qt::Key_H:
      return Aspect_VKey_H;
    case Qt::Key_I:
      return Aspect_VKey_I;
    case Qt::Key_J:
      return Aspect_VKey_J;
    case Qt::Key_K:
      return Aspect_VKey_K;
    case 1044: // ru
    case Qt::Key_L:
      return Aspect_VKey_L;
    case Qt::Key_M:
      return Aspect_VKey_M;
    case Qt::Key_N:
      return Aspect_VKey_N;
    case Qt::Key_O:
      return Aspect_VKey_O;
    case Qt::Key_P:
      return Aspect_VKey_P;
    case 1049: // ru
    case Qt::Key_Q:
      return Aspect_VKey_Q;
    case 1050: // ru
    case Qt::Key_R:
      return Aspect_VKey_R;
    case 1067: // ru
    case Qt::Key_S:
      return Aspect_VKey_S;
    case 1045: // ru
    case Qt::Key_T:
      return Aspect_VKey_T;
    case Qt::Key_U:
      return Aspect_VKey_U;
    case 1052: // ru
    case Qt::Key_V:
      return Aspect_VKey_V;
    case 1062: // ru
    case Qt::Key_W:
      return Aspect_VKey_W;
    case 1063: // ru
    case Qt::Key_X:
      return Aspect_VKey_X;
    case Qt::Key_Y:
      return Aspect_VKey_Y;
    case 1071: // ru
    case Qt::Key_Z:
      return Aspect_VKey_Z;
      //
    case Qt::Key_0:
      return Aspect_VKey_0;
    case Qt::Key_1:
      return Aspect_VKey_1;
    case Qt::Key_2:
      return Aspect_VKey_2;
    case Qt::Key_3:
      return Aspect_VKey_3;
    case Qt::Key_4:
      return Aspect_VKey_4;
    case Qt::Key_5:
      return Aspect_VKey_5;
    case Qt::Key_6:
      return Aspect_VKey_6;
    case Qt::Key_7:
      return Aspect_VKey_7;
    case Qt::Key_8:
      return Aspect_VKey_8;
    case Qt::Key_9:
      return Aspect_VKey_9;
      //
    case Qt::Key_F1:
      return Aspect_VKey_F1;
    case Qt::Key_F2:
      return Aspect_VKey_F2;
    case Qt::Key_F3:
      return Aspect_VKey_F3;
    case Qt::Key_F4:
      return Aspect_VKey_F4;
    case Qt::Key_F5:
      return Aspect_VKey_F5;
    case Qt::Key_F6:
      return Aspect_VKey_F6;
    case Qt::Key_F7:
      return Aspect_VKey_F7;
    case Qt::Key_F8:
      return Aspect_VKey_F8;
    case Qt::Key_F9:
      return Aspect_VKey_F9;
    case Qt::Key_F10:
      return Aspect_VKey_F10;
    case Qt::Key_F11:
      return Aspect_VKey_F11;
    case Qt::Key_F12:
      return Aspect_VKey_F12;
      //
    case Qt::Key_Up:
      return Aspect_VKey_Up;
    case Qt::Key_Left:
      return Aspect_VKey_Left;
    case Qt::Key_Right:
      return Aspect_VKey_Right;
    case Qt::Key_Down:
      return Aspect_VKey_Down;
    case Qt::Key_Plus:
      return Aspect_VKey_Plus;
    case Qt::Key_Minus:
      return Aspect_VKey_Minus;
    case Qt::Key_Equal:
      return Aspect_VKey_Equal;
    case Qt::Key_PageDown:
      return Aspect_VKey_PageDown;
    case Qt::Key_PageUp:
      return Aspect_VKey_PageUp;
    case Qt::Key_Home:
      return Aspect_VKey_Home;
    case Qt::Key_End:
      return Aspect_VKey_End;
    case Qt::Key_Escape:
      return Aspect_VKey_Escape;
    case Qt::Key_Back:
      return Aspect_VKey_Back;
    case Qt::Key_Enter:
      return Aspect_VKey_Enter;
    case Qt::Key_Backspace:
      return Aspect_VKey_Backspace;
    case Qt::Key_Space:
      return Aspect_VKey_Space;
    case Qt::Key_Delete:
      return Aspect_VKey_Delete;
    case Qt::Key_Tab:
      return Aspect_VKey_Tab;
    case 1025:
    case Qt::Key_QuoteLeft:
      return Aspect_VKey_Tilde;
      //
    case Qt::Key_Shift:
      return Aspect_VKey_Shift;
    case Qt::Key_Control:
      return Aspect_VKey_Control;
    case Qt::Key_Alt:
      return Aspect_VKey_Alt;
    case Qt::Key_Menu:
      return Aspect_VKey_Menu;
    case Qt::Key_Meta:
      return Aspect_VKey_Meta;
    default:
      return Aspect_VKey_UNKNOWN;
  }
}
