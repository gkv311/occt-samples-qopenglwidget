OCCT 3D Viewer setup within Qt
==============================

This sample demonstrates how to use Open CASCADE Technology (OCCT) 3D Viewer within a window created using Qt5 on Windows and Linux platforms.

![sample screenshot](/images/occt-qopenglwidget-sample-wnt.png)

## OCCT QOpenGLWidget sample

Project within `occt-qopenglwidget` subfolder shows initialization of OCCT 3D viewer
from OpenGL context created by `QOpenGLWidget` within Qt5 Widgets application.

Project could be built in two ways:

- Using *CMake*.
  See `CMakeLists.txt` in the folder defining building rules.
  Use `cmake-gui` or command-line interface for configuring project dependencies (Qt, OCCT).
- Using *qmake*.
  See `occt-qopenglwidget-sample.pro` in the folder defining building rules.
  Create `custom.pri` from `custom.pri.template` with filled in paths to OCCT.

Qt Creator could be used to open `qmake` project directly.
Make sure to copy necessary dependencies (OCCT, FreeType, FreeImage, etc. - depending on OCCT building configuration)
into sample folder or configuring environment (`PATH`, `LD_LIBRARY_PATH`).

Building has been checked within OCCT 7.6.0.

![sample screenshot](/images/occt-qopenglwidget-sample-x11.png)
