// Copyright (c) 2025 Kirill Gavrilov

#include "OcctQQuickFramebufferViewer.h"

#include <Standard_WarningsDisable.hxx>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSurfaceFormat>
#include <Standard_WarningsRestore.hxx>

#include <OSD_Environment.hxx>
#include <Standard_Version.hxx>

int main(int theNbArgs, char** theArgVec)
{
  // before creaing QApplication: define platform plugin to load (e.g. xcb on Linux)
  // and graphic driver (e.g. desktop OpenGL with desired profile/surface)
  OcctQtTools::qtGlPlatformSetup();

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

  qmlRegisterType<OcctQQuickFramebufferViewer>("OcctQQuickFramebufferViewer", 1, 0, "OcctQQuickFramebufferViewer");

  QQmlApplicationEngine aQmlEngine;
  aQmlEngine.rootContext()->setContextProperty("QT_VERSION_STR", QString(QT_VERSION_STR));
  aQmlEngine.rootContext()->setContextProperty("OCC_VERSION_STRING_EXT", QString(OCC_VERSION_STRING_EXT));
  aQmlEngine.load(QUrl(QStringLiteral("qrc:/main.qml")));
  return aQApp.exec();
}
