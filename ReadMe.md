OCCT QOpenGLWidget sample
==================

This sample demonstrates how to use Open CASCADE Technology (OCCT) 3D Viewer within a window created using Qt5 on Windows and Linux platforms.

![sample screenshot](/occt-qopenglwidget-sample-wnt.png)

OpenGL-based OCCT viewer is embedded into Qt5 Widgets application via `QOpenGLWidget` widget.

Use qmake or Qt Creator for configuring the project and setting up paths to Qt and Open CASCADE Technology (OCCT) libraries.
Create `custom.pri` from `custom.pri.template` with filled in paths to OCCT.
Building has been checked within OCCT 7.6.0 (beta).

![sample screenshot](/occt-qopenglwidget-sample-x11.png)
