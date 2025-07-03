// Copyright (c) 2023 Kirill Gavrilov

#ifndef _OcctQOpenGLWidgetViewer_HeaderFile
#define _OcctQOpenGLWidgetViewer_HeaderFile

#include <Standard_WarningsDisable.hxx>
#include <QOpenGLWidget>
#include <Standard_WarningsRestore.hxx>

#include <AIS_InteractiveContext.hxx>
#include <AIS_ViewController.hxx>
#include <V3d_View.hxx>

class AIS_ViewCube;

//! OpenGL Qt widget holding OCCT 3D View.
//!
//! OCCT 3D Viewer will reuse OpenGL context created by QOpenGLWidget;
//! widgets on top will be blended by Qt naturally.
//!
//! Inheritance from AIS_ViewController is used to translate
//! user input events (mouse, keyboard, window resize, etc.)
//! to 3D Viewer (panning, rotation, zooming, etc.).
class OcctQOpenGLWidgetViewer : public QOpenGLWidget, public AIS_ViewController
{
  Q_OBJECT
public:
  //! Main constructor.
  OcctQOpenGLWidgetViewer(QWidget* theParent = nullptr);

  //! Destructor.
  virtual ~OcctQOpenGLWidgetViewer();

  //! Return Viewer.
  const Handle(V3d_Viewer)& Viewer() const { return myViewer; }

  //! Return View.
  const Handle(V3d_View)& View() const { return myView; }

  //! Return AIS context.
  const Handle(AIS_InteractiveContext)& Context() const { return myContext; }

  //! Return OpenGL info.
  const QString& getGlInfo() const { return myGlInfo; }

  //! Minimal widget size.
  virtual QSize minimumSizeHint() const override { return QSize(200, 200); }

  //! Default widget size.
  virtual QSize sizeHint() const override { return QSize(720, 480); }

public:
  //! Handle subview focus change.
  virtual void OnSubviewChanged(const Handle(AIS_InteractiveContext)&,
                                const Handle(V3d_View)&,
                                const Handle(V3d_View)& theNewView) override;

protected: // OpenGL events
  virtual void initializeGL() override;
  virtual void paintGL() override;
  // virtual void resizeGL(int , int ) override;

protected: // user input events
  virtual void closeEvent(QCloseEvent* theEvent) override;
  virtual void keyPressEvent(QKeyEvent* theEvent) override;
  virtual void mousePressEvent(QMouseEvent* theEvent) override;
  virtual void mouseReleaseEvent(QMouseEvent* theEvent) override;
  virtual void mouseMoveEvent(QMouseEvent* theEvent) override;
  virtual void wheelEvent(QWheelEvent* theEvent) override;

private:
  //! Dump OpenGL info.
  void dumpGlInfo(bool theIsBasic, bool theToPrint);

  //! Request widget paintGL() event.
  void updateView();

  //! Handle view redraw.
  virtual void handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx, const Handle(V3d_View)& theView) override;

private:
  Handle(V3d_Viewer)             myViewer;
  Handle(V3d_View)               myView;
  Handle(AIS_InteractiveContext) myContext;
  Handle(AIS_ViewCube)           myViewCube;

  Handle(V3d_View) myFocusView;

  QString myGlInfo;
  bool    myIsCoreProfile = true;
};

#endif // _OcctQOpenGLWidgetViewer_HeaderFile
