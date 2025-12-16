// Copyright (c) 2025 Kirill Gavrilov

#ifndef _OcctQQuickFramebufferViewer_HeaderFile
#define _OcctQQuickFramebufferViewer_HeaderFile

#include "../occt-qt-tools/OcctQtTools.h"

#include <Standard_WarningsDisable.hxx>
#include <QQuickFramebufferObject>
#include <Standard_WarningsRestore.hxx>

#include <AIS_InteractiveContext.hxx>
#include <AIS_ViewController.hxx>
#include <V3d_View.hxx>
#include <Standard_Version.hxx>

class AIS_ViewCube;

//! OpenGL QtQuick framebuffer control holding OCCT 3D View.
//!
//! OCCT 3D Viewer will reuse OpenGL context created by QtQuick;
//! controls on top will be blended by Qt naturally.
//!
//! Inheritance from AIS_ViewController is used to translate
//! user input events (mouse, keyboard, window resize, etc.)
//! to 3D Viewer (panning, rotation, zooming, etc.).
class OcctQQuickFramebufferViewer : public QQuickFramebufferObject, public AIS_ViewController
{
  Q_OBJECT

  // QML properties
  Q_PROPERTY(QColor  backgroundColor READ getBackgroundColor WRITE setBackgroundColor)
  Q_PROPERTY(QString glInfo READ getGlInfo NOTIFY glInfoChanged)
public:
  //! Main constructor.
  OcctQQuickFramebufferViewer(QQuickItem* theParent = nullptr);

  //! Destructor.
  virtual ~OcctQQuickFramebufferViewer();

  //! Return Viewer.
  const Handle(V3d_Viewer)& Viewer() const { return myViewer; }

  //! Return View.
  const Handle(V3d_View)& View() const { return myView; }

  //! Return AIS context.
  const Handle(AIS_InteractiveContext)& Context() const { return myContext; }

public: // QML accessors
  //! Return OpenGL info.
  const QString& getGlInfo() const { return myGlInfo; }

  //! Return background color.
  QColor getBackgroundColor()
  {
    Standard_Mutex::Sentry aLock(myViewerMutex);
    return OcctQtTools::qtColorFromOcct(myView->BackgroundColor());
  }

  //! Set background color.
  void setBackgroundColor(const QColor& theColor)
  {
    const Quantity_Color aColor = OcctQtTools::qtColorToOcct(theColor);
    {
      Standard_Mutex::Sentry aLock(myViewerMutex);
      myView->SetBgGradientColors(aColor, Quantity_NOC_BLACK, Aspect_GradientFillMethod_Elliptical);
      myView->Invalidate();
    }
    update();
  }

signals:
  void glInfoChanged();

protected:
  //! OpenGL renderer interface.
  class Renderer : public QQuickFramebufferObject::Renderer
  {
  public:
    Renderer(OcctQQuickFramebufferViewer* theViewer);
    virtual ~Renderer();
  private:
    virtual QOpenGLFramebufferObject* createFramebufferObject(const QSize& theSize) override;
    virtual void synchronize(QQuickFramebufferObject* theItem) override;
    virtual void render() override;

  private:
    OcctQQuickFramebufferViewer* myViewer = nullptr;
  };

  virtual QQuickFramebufferObject::Renderer* createRenderer() const override;
  //virtual void releaseResources() override;

  void initializeGL(QOpenGLFramebufferObject* theFbo);
  void synchronize(QOpenGLFramebufferObject* theFbo);
  void render(QOpenGLFramebufferObject* theFbo);

protected: // user input events
  virtual bool event(QEvent* theEvent) override;
  virtual void keyPressEvent(QKeyEvent* theEvent) override;
  virtual void mousePressEvent(QMouseEvent* theEvent) override;
  virtual void mouseReleaseEvent(QMouseEvent* theEvent) override;
  virtual void mouseMoveEvent(QMouseEvent* theEvent) override;
  virtual void wheelEvent(QWheelEvent* theEvent) override;
  virtual void hoverEnterEvent(QHoverEvent* theEvent) override;
  virtual void hoverMoveEvent(QHoverEvent* theEvent) override;
  virtual void hoverLeaveEvent(QHoverEvent* theEvent) override;
  //virtual void touchEvent(QTouchEvent* theEvent) override;

private:
  //! Dump OpenGL info.
  void dumpGlInfo(bool theIsBasic, bool theToPrint);

  //! Request 3D viewer redrawing from GUI thread.
  void updateView();

  //! Handle view redraw.
  virtual void handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx, const Handle(V3d_View)& theView) override;

private:
  Handle(V3d_Viewer)             myViewer;
  Handle(V3d_View)               myView;
  Handle(AIS_InteractiveContext) myContext;
  Handle(AIS_ViewCube)           myViewCube;

  Standard_Mutex myViewerMutex;

  QString myGlInfo;
  bool    myHasTouchInput = false;
};

#endif // _OcctQQuickFramebufferViewer_HeaderFile
