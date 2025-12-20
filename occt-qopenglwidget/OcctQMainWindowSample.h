// Copyright (c) 2025 Kirill Gavrilov

#ifndef _OcctQMainWindowSample_HeaderFile
#define _OcctQMainWindowSample_HeaderFile

#include <Standard_WarningsDisable.hxx>
#include <QMainWindow>
#include <Standard_WarningsRestore.hxx>

class OcctQOpenGLWidgetViewer;

//! Main application window.
class OcctQMainWindowSample : public QMainWindow
{
public:
  //! Window constructor.
  OcctQMainWindowSample();

private:
  //! Define menu bar with Quit item.
  void createMenuBar();

  //! Define controls over 3D viewer.
  void createLayoutOverViewer();

  //! Advanced method splitting 3D Viewer into sub-views.
  void splitSubviews();

private:
  OcctQOpenGLWidgetViewer* myViewer = nullptr;
};

#endif // _OcctQMainWindowSample_HeaderFile
