// Copyright (c) 2025 Kirill Gavrilov

#include "OcctQWidgetViewer.h"

#include "../occt-qt-tools/OcctQtTools.h"

#include <Standard_WarningsDisable.hxx>
#include <QApplication>
#include <QSurfaceFormat>

#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <Standard_WarningsRestore.hxx>

#include <Standard_Version.hxx>

//! Main application window.
class MyMainWindow : public QMainWindow
{
  OcctQWidgetViewer* myViewer = nullptr;

public:
  //! Window constructor.
  MyMainWindow()
  {
    // 3D Viewer widget as a central widget
    myViewer = new OcctQWidgetViewer();
    setCentralWidget(myViewer);

    // menu bar
    createMenuBar();

    // some controls on top of 3D Viewer
    createLayoutOverViewer();
  }

private:
  //! Define menu bar with Quit item.
  void createMenuBar()
  {
    QMenuBar* aMenuBar    = new QMenuBar();
    QMenu*    aMenuWindow = aMenuBar->addMenu("&File");
#if (OCC_VERSION_HEX >= 0x070700)
    {
      QAction* anActionSplit = new QAction(aMenuWindow);
      anActionSplit->setText("Split Views");
      aMenuWindow->addAction(anActionSplit);
      connect(anActionSplit, &QAction::triggered, [this]() { splitSubviews(); });
    }
#endif
    {
      QAction* anActionQuit = new QAction(aMenuWindow);
      anActionQuit->setText("Quit");
      aMenuWindow->addAction(anActionQuit);
      connect(anActionQuit, &QAction::triggered, [this]() { close(); });
    }
    setMenuBar(aMenuBar);
  }

  //! Define controls over 3D viewer.
  void createLayoutOverViewer()
  {
    QVBoxLayout* aLayout = new QVBoxLayout(myViewer);
    aLayout->setDirection(QBoxLayout::BottomToTop);
    aLayout->setAlignment(Qt::AlignBottom);
    {
      // button displaying message window with OpenGL info
      QPushButton* aQuitBtn = new QPushButton("About");
      aLayout->addWidget(aQuitBtn);
      connect(aQuitBtn, &QPushButton::clicked, [this]() {
        QMessageBox::information(0,
                                 "About Sample",
                                 QString() + "OCCT 3D Viewer sample embedded into Qt Widgets.\n\n"
                                   + "Open CASCADE Technology v." OCC_VERSION_STRING_EXT "\n"
                                   + "Qt v." QT_VERSION_STR "\n\n" + "OpenGL info:\n" + myViewer->getGlInfo());
      });
    }
    {
      // slider changing viewer background color

      // make sure widget on top of OCCT 3D Viewer (implemented as QWidget and not QOpenGLWidget)
      // is filled with color - semitransparent widgets will have artifacts
      QWidget* aSliderBox = new QWidget();
      aSliderBox->setStyleSheet("QWidget { background-color: black; }");

      QHBoxLayout* aSliderLayout = new QHBoxLayout(aSliderBox);
      {
        QLabel* aSliderLabel = new QLabel("Background");
        aSliderLabel->setStyleSheet("QLabel { background-color: rgba(0, 0, 0, 0); color: white; }");
        aSliderLabel->setGeometry(50, 50, 50, 50);
        aSliderLabel->adjustSize();
        aSliderLayout->addWidget(aSliderLabel);
      }
      {
        QSlider* aSlider = new QSlider(Qt::Horizontal);
        aSlider->setRange(0, 255);
        aSlider->setSingleStep(1);
        aSlider->setPageStep(15);
        aSlider->setTickInterval(15);
        aSlider->setTickPosition(QSlider::TicksRight);
        aSlider->setValue(0);
        aSliderLayout->addWidget(aSlider);
        connect(aSlider, &QSlider::valueChanged, [this](int theValue) {
          const float          aVal = theValue / 255.0f;
          const Quantity_Color aColor(aVal, aVal, aVal, Quantity_TOC_sRGB);
#if (OCC_VERSION_HEX >= 0x070700)
          for (const Handle(V3d_View)& aSubviewIter : myViewer->View()->Subviews())
          {
            aSubviewIter->SetBgGradientColors(aColor, Quantity_NOC_BLACK, Aspect_GradientFillMethod_Elliptical);
            aSubviewIter->Invalidate();
          }
#endif
          // myViewer->View()->SetBackgroundColor(aColor);
          myViewer->View()->SetBgGradientColors(aColor, Quantity_NOC_BLACK, Aspect_GradientFillMethod_Elliptical);
          myViewer->View()->Invalidate();
          myViewer->update();
        });
      }

      aLayout->addWidget(aSliderBox);
    }
  }

#if (OCC_VERSION_HEX >= 0x070700)
  //! Advanced method splitting 3D Viewer into sub-views.
  void splitSubviews()
  {
    if (!myViewer->View()->Subviews().IsEmpty())
    {
      // remove subviews
      myViewer->View()->View()->SetSubviewComposer(false);
      NCollection_Sequence<Handle(V3d_View)> aSubviews = myViewer->View()->Subviews();
      for (const Handle(V3d_View)& aSubviewIter : aSubviews)
        aSubviewIter->Remove();

      myViewer->OnSubviewChanged(myViewer->Context(), nullptr, myViewer->View());
    }
    else
    {
      // create two subviews splitting window horizontally
      myViewer->View()->View()->SetSubviewComposer(true);

      Handle(V3d_View) aSubView1 = new V3d_View(myViewer->Viewer());
      aSubView1->SetImmediateUpdate(false);
      aSubView1->SetWindow(myViewer->View(),
                           Graphic3d_Vec2d(0.5, 1.0),
                           Aspect_TOTP_LEFT_UPPER,
                           Graphic3d_Vec2d(0.0, 0.0));

      Handle(V3d_View) aSubView2 = new V3d_View(myViewer->Viewer());
      aSubView2->SetImmediateUpdate(false);
      aSubView2->SetWindow(myViewer->View(),
                           Graphic3d_Vec2d(0.5, 1.0),
                           Aspect_TOTP_LEFT_UPPER,
                           Graphic3d_Vec2d(0.5, 0.0));

      myViewer->OnSubviewChanged(myViewer->Context(), nullptr, aSubView1);
    }
    myViewer->View()->Invalidate();
    myViewer->update();
  }
#endif
};

int main(int theNbArgs, char** theArgVec)
{
  // before creaing QApplication: define platform plugin to load (e.g. xcb on Linux)
  // and graphic driver (e.g. desktop OpenGL with desired profile/surface)
  OcctQtTools::qtGlPlatformSetup();
  QApplication aQApp(theNbArgs, theArgVec);

  QCoreApplication::setApplicationName("OCCT Qt/QWidget Viewer sample");
  QCoreApplication::setOrganizationName("OpenCASCADE");
  QCoreApplication::setApplicationVersion(OCC_VERSION_STRING_EXT);

  MyMainWindow aMainWindow;
  aMainWindow.resize(aMainWindow.sizeHint());
  aMainWindow.show();
  return aQApp.exec();
}
