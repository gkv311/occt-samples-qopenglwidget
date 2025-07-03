// Copyright (c) 2025 Kirill Gavrilov

#ifndef _OcctQWidgetViewer_HeaderFile
#define _OcctQWidgetViewer_HeaderFile

#include <Standard_WarningsDisable.hxx>
#include <QWidget>
#include <Standard_WarningsRestore.hxx>

#include <AIS_InteractiveContext.hxx>
#include <AIS_ViewController.hxx>
#include <V3d_View.hxx>

class AIS_ViewCube;

//! OpenGL Qt widget holding OCCT 3D View.
//!
//! OCCT 3D Viewer will create its own OpenGL context for native window of QWidget;
//! widgets on top should have not semitransparent background, otherwise Qt will be unable to blend them correctly.
//!
//! Inheritance from AIS_ViewController is used to translate
//! user input events (mouse, keyboard, window resize, etc.)
//! to 3D Viewer (panning, rotation, zooming, etc.).
class OcctQWidgetViewer : public QWidget, public AIS_ViewController
{
  Q_OBJECT
public:
  //! Main constructor.
  OcctQWidgetViewer(QWidget* theParent = nullptr);

  //! Destructor.
  virtual ~OcctQWidgetViewer();

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

protected: // drawing events
  //! Initial OpenGL setup.
  void initializeGL();

  //! Redraw the widget (3D Viewer).
  virtual void paintEvent(QPaintEvent* theEvent) override;

  //! Resize the widget (3D Viewer).
  virtual void resizeEvent(QResizeEvent* theEvent) override;

  //! Important - prevent Qt to try drawing in this widget.
  virtual QPaintEngine* paintEngine() const override { return nullptr; }

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

#endif // _OcctQWidgetViewer_HeaderFile
