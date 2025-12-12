// Copyright (c) 2025 Kirill Gavrilov

#include "OcctQQuickFramebufferViewer.h"

#include <Standard_WarningsDisable.hxx>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <Standard_WarningsRestore.hxx>

#include <OSD_Environment.hxx>
#include <Standard_Version.hxx>

int main(int theNbArgs, char** theArgVec)
{
#if defined(_WIN32)
  //
#elif defined(__APPLE__)
  //
#else
  // Qt6 tries to use Wayland platform by default, which is incompatible with OCCT depending on Xlib;
  // Force 'xcb' platform plugin (alternatively, could be passed QApplication as '-platfom xcb' argument).
  OSD_Environment aQpaPlat("QT_QPA_PLATFORM");
  if (aQpaPlat.Value().IsEmpty())
  {
    aQpaPlat.SetValue("xcb");
    aQpaPlat.Build();
  }
#endif

  // Qt by default will attempt offloading rendering
  // into a separate working thread (QSGRenderThread) on some systems,
  // which requires addition of multithreading synchronization mechanism
  // when dealing with OCCT 3D Viewer from GUI thread.
  // Uncomment following lines if these complexities are undesired
  // to ask Qt managing rendering from GUI thread.
  /*OSD_Environment aQsgLoop("QSG_RENDER_LOOP");
  if (aQsgLoop.Value().IsEmpty())
  {
    aQsgLoop.SetValue("basic");
    aQsgLoop.Build();
  }*/

  QApplication aQApp(theNbArgs, theArgVec);

  QCoreApplication::setApplicationName("OCCT Qt/QtQuick Viewer sample");
  QCoreApplication::setOrganizationName("OpenCASCADE");
  QCoreApplication::setApplicationVersion(OCC_VERSION_STRING_EXT);
  //QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#if defined(_WIN32)
  // never use ANGLE on Windows, since OCCT 3D Viewer does not expect this
  QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
  // QCoreApplication::setAttribute (Qt::AA_UseOpenGLES);
#endif

  qmlRegisterType<OcctQQuickFramebufferViewer>("OcctQQuickFramebufferViewer", 1, 0, "OcctQQuickFramebufferViewer");

  QQmlApplicationEngine aQmlEngine;
  aQmlEngine.rootContext()->setContextProperty("QT_VERSION_STR", QString(QT_VERSION_STR));
  aQmlEngine.rootContext()->setContextProperty("OCC_VERSION_STRING_EXT", QString(OCC_VERSION_STRING_EXT));
  aQmlEngine.load(QUrl(QStringLiteral("qrc:/main.qml")));
  return aQApp.exec();
}
