OCCT 3D Viewer setup within Qt
==============================

This sample demonstrates how to use Open CASCADE Technology (OCCT) 3D Viewer within Qt5 GUI framework on Windows and Linux platforms.

![sample screenshot](/images/occt-qopenglwidget-sample-wnt.png)

## Building

Use *CMake* for building the project.
See `CMakeLists.txt` in the folder defining building rules.
Use `cmake-gui` or command-line interface for configuring project dependencies (Qt, OCCT).
Building has been checked within OCCT 7.6.0.

Make sure to copy necessary dependencies (*OCCT*, *FreeType*, *FreeImage*, etc. - depending on *OCCT* building configuration)
into sample folder or configuring environment (`PATH`, `LD_LIBRARY_PATH`).

*Legacy: some samples contains qmake project definition like `occt-qopenglwidget-sample.pro`,*
*which could be used as alternative to CMake.*
*Create `custom.pri` from `custom.pri.template` with filled in paths to OCCT.*

## OCCT QWidget sample

Project within `occt-qwidget` subfolder shows initialization of OCCT 3D viewer using
basic `QWidget` with own native window handle (`QWidget::setAttribute(Qt::WA_NativeWindow)` attribute).

Within this approach, OCCT will be responsible to initialize OpenGL / OpenGL ES context on its own.
Unlike `QOpenGLWidget` sample, the widgets on top of `QWidget` holding 3D Viewer should have solid background color,
as Qt will be unable to blend widgets together on its own.

## OCCT QOpenGLWidget sample

Project within `occt-qopenglwidget` subfolder shows initialization of OCCT 3D viewer
from OpenGL context created by `QOpenGLWidget` within Qt5 Widgets application.

The approach with `QOpenGLWidget` requires careful gluing layer for Qt and OCCT 3D Viewer to share common OpenGL context.
Unlike `QWidget` sample, the widgets on top of `QOpenGLWidget` holding 3D Viewer might have semitransparent background color,
as Qt will be able to blend widgets together on its own.

![sample screenshot](/images/occt-qopenglwidget-sample-x11.png)
