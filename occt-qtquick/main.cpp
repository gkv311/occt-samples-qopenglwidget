// Copyright (c) 2025 Kirill Gavrilov

#include "OcctQQuickFramebufferViewer.h"

#include <Standard_WarningsDisable.hxx>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSurfaceFormat>
#include <Standard_WarningsRestore.hxx>

#include <Message.hxx>
#include <Standard_Version.hxx>

int main(int theNbArgs, char** theArgVec)
{
  //Message::DefaultMessenger()->Printers().First()->SetTraceLevel(Message_Trace);

  // before creating QApplication: define platform plugin to load (e.g. xcb on Linux)
  // and graphic driver (e.g. desktop OpenGL with desired profile/surface)
  OcctQtTools::qtGlPlatformSetup();

  // Qt by default will attempt offloading rendering
  // into a separate working thread (QSGRenderThread) on some systems,
  // which requires addition of multi-threading synchronization mechanism
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

  qmlRegisterType<OcctQQuickFramebufferViewer>("OcctQQuickFramebufferViewer", 1, 0, "OcctQQuickFramebufferViewer");

  QQmlApplicationEngine aQmlEngine;
  aQmlEngine.rootContext()->setContextProperty("QT_VERSION_STR", QString(QT_VERSION_STR));
  aQmlEngine.rootContext()->setContextProperty("OCC_VERSION_STRING_EXT", QString(OCC_VERSION_STRING_EXT));
#if (QT_VERSION_MAJOR >= 6)
  aQmlEngine.load(QUrl(QStringLiteral("qrc:/main6.qml")));
#else
  aQmlEngine.load(QUrl(QStringLiteral("qrc:/main5.qml")));
#endif
  return aQApp.exec();
}
