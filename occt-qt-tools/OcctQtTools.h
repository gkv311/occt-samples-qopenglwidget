// Copyright (c) 2025 Kirill Gavrilov

#ifndef _OcctQtTools_HeaderFile
#define _OcctQtTools_HeaderFile

#include <Aspect_WindowInputListener.hxx>
#include <Message_Gravity.hxx>
#include <Quantity_ColorRGBA.hxx>

#include <Standard_WarningsDisable.hxx>
#include <QColor>
#include <QMouseEvent>
#include <QSurfaceFormat>
#include <Standard_WarningsRestore.hxx>

class OpenGl_Caps;
class V3d_View;

//! Auxiliary tools between Qt and OCCT definitions.
class OcctQtTools
{
public: //! @name Qt application pre-setup for OCCT 3D Viewer integration

  //! Perform global Qt platform setup - to be called before QApplication creation.
  //! Defines platform plugin to load (e.g. xcb on Linux)
  //! and graphic driver (e.g. desktop OpenGL with desired profile/surface).
  static void qtGlPlatformSetup();

  //! Define default Qt surface format for GL context.
  static QSurfaceFormat qtGlSurfaceFormat(QSurfaceFormat::OpenGLContextProfile theProfile = QSurfaceFormat::NoProfile,
                                          bool theToDebug = false);

  //! Fill in OCCT GL caps from Qt surface format.
  static void qtGlCapsFromSurfaceFormat(OpenGl_Caps& theCaps, const QSurfaceFormat& theFormat);

public: //! @name common conversion tools

  //! Map QColor into Quantity_Color.
  static Quantity_Color qtColorToOcct(const QColor& theColor);

  //! Map Quantity_Color into QColor.
  static QColor qtColorFromOcct(const Quantity_Color& theColor);

  //! Map QColor into Quantity_ColorRGBA.
  static Quantity_ColorRGBA qtColorToOcctRgba(const QColor& theColor);

  //! Map Quantity_ColorRGBA into QColor.
  static QColor qtColorFromOcctRgba(const Quantity_ColorRGBA& theColor);

  //! Map QString into TCollection_AsciiString (UTF-8).
  static TCollection_AsciiString qtStringToOcct(const QString& theText);

  //! Map TCollection_AsciiString (UTF-8) into QString.
  static QString qtStringFromOcct(const TCollection_AsciiString& theText);

  //! Map QString into TCollection_ExtendedString (UTF-16).
  static TCollection_ExtendedString qtStringToOcctExt(const QString& theText);

  //! Map TCollection_ExtendedString (UTF-16) into QString.
  static QString qtStringFromOcctExt(const TCollection_ExtendedString& theText);

public: //! @name methods for message logs

  //! Map QtMsgType into Message_Gravity.
  static Message_Gravity qtMsgTypeToGravity(QtMsgType theType);

  //! Callback for qInstallMessageHandler() redirecting Qt messages to OCCT messenger.
  static void qtMessageHandlerToOcct(QtMsgType theType,
                                     const QMessageLogContext& theCtx,
                                     const QString& theMsg);

public: //! @name methods for wrapping Qt input events into Aspect_WindowInputListener events

  //! Queue Qt mouse hover event to OCCT listener.
  static bool qtHandleHoverEvent(Aspect_WindowInputListener& theListener,
                                 const Handle(V3d_View)& theView,
                                 const QHoverEvent* theEvent);

  //! Queue Qt mouse event to OCCT listener.
  static bool qtHandleMouseEvent(Aspect_WindowInputListener& theListener,
                                 const Handle(V3d_View)& theView,
                                 const QMouseEvent* theEvent);

  //! Queue Qt mouse wheel event to OCCT listener.
  static bool qtHandleWheelEvent(Aspect_WindowInputListener& theListener,
                                 const Handle(V3d_View)& theView,
                                 const QWheelEvent* theEvent);

  //! Queue Qt touch event to OCCT listener.
  static bool qtHandleTouchEvent(Aspect_WindowInputListener& theListener,
                                 const Handle(V3d_View)& theView,
                                 const QTouchEvent* theEvent);

  //! Map Qt buttons bitmask to virtual keys.
  static Aspect_VKeyMouse qtMouseButtons2VKeys(Qt::MouseButtons theButtons);

  //! Map Qt mouse modifiers bitmask to virtual keys.
  static Aspect_VKeyFlags qtMouseModifiers2VKeys(Qt::KeyboardModifiers theModifiers);

  //! Map Qt key to virtual key.
  static Aspect_VKey qtKey2VKey(int theKey);

};

#endif // _OcctQtTools_HeaderFile
