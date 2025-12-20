// Copyright (c) 2025 Kirill Gavrilov

#include "OcctQMainWindowSample.h"

#include "../occt-qt-tools/OcctQtTools.h"

#include <Standard_WarningsDisable.hxx>
#include <QApplication>
#include <Standard_WarningsRestore.hxx>

#include <Standard_Version.hxx>

int main(int theNbArgs, char** theArgVec)
{
  // before creating QApplication: define platform plugin to load (e.g. xcb on Linux)
  // and graphic driver (e.g. desktop OpenGL with desired profile/surface)
  OcctQtTools::qtGlPlatformSetup();
  QApplication aQApp(theNbArgs, theArgVec);

  QCoreApplication::setApplicationName("OCCT Qt/QWidget Viewer sample");
  QCoreApplication::setOrganizationName("OpenCASCADE");
  QCoreApplication::setApplicationVersion(OCC_VERSION_STRING_EXT);

  OcctQMainWindowSample aMainWindow;
  aMainWindow.resize(aMainWindow.sizeHint());
  aMainWindow.show();
  return aQApp.exec();
}
