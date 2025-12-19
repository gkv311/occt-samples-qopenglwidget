OCCT 3D Viewer setup within Qt
==============================

The samples in this repository demonstrate Open CASCADE Technology (OCCT) 3D Viewer setup
within Qt GUI framework on Windows and Linux platforms.

![sample screenshot](/images/occt-qopenglwidget-sample-wnt.png)

## Building

Use *CMake* for building the project.
See `CMakeLists.txt` in the folder defining building rules.
Use `cmake-gui` or command-line interface for configuring project dependencies (Qt, OCCT).
Building has been checked within OCCT 7.7.0.

Make sure to copy necessary dependencies (*OCCT*, *FreeType*, *FreeImage*, etc. - depending on *OCCT* building configuration)
into sample folder or configuring environment (`PATH`, `LD_LIBRARY_PATH`).

*Legacy: some samples contains qmake project definition like `occt-qopenglwidget-sample.pro`,*
*which could be used as alternative to CMake.*
*Create `custom.pri` from `custom.pri.template` with filled in paths to OCCT.*

## OCCT Qt tools

Pseudo-project within `occt-qt-tools` subfolder provides source code for gluing between Qt and OCCT:
- `OcctQtTools`
  - conversion between common Qt and OCCT structures like colors and strings;
  - Qt application setup for embedding 3D viewer;
  - Qt input events conversion into OCCT 3D Viewer events.
- `OcctGlTools` - common tools (independent from Qt) for wrapping externally created OpenGL context to setup OCCT 3D Viewer.

Each Qt sample in the list below is defined independently
so that it could be easily extracted to define your own application based on selected Qt module
without bloating project with unnecessary code and dependencies.
However, this leads to some code duplication in samples, which `OcctQtTools` tries to reduce.

## OCCT QWidget sample

Project within `occt-qwidget` subfolder shows OCCT 3D viewer setup within
basic `QWidget` from native window handle (`QWidget::setAttribute(Qt::WA_NativeWindow)` attribute).

Within this approach, OCCT will be responsible to initialize OpenGL / OpenGL ES context on its own (Qt will be unaware of it).
Unlike `QOpenGLWidget` sample, the widgets on top of `QWidget` holding 3D Viewer should have solid background color,
as Qt will be unable to blend widgets together on its own.

*Notice: `QOpenGLWidget` (see next) is a preferred way for integrating OpenGL viewer into Qt Widgets application.*
*This `QWidget` sample demonstrates a working approach commonly used by applications based on Qt3/Qt4 and other GUI frameworks.*

## OCCT QOpenGLWidget sample

Project within `occt-qopenglwidget` subfolder shows OCCT 3D viewer setup
from *OpenGL* context created by `QOpenGLWidget` within Qt Widgets application.

The approach with `QOpenGLWidget` requires careful gluing layer for Qt and OCCT 3D Viewer to share common OpenGL context.
Unlike `QWidget` sample, the widgets on top of `QOpenGLWidget` holding 3D Viewer might have semitransparent background color,
as Qt will be able to blend widgets together on its own.

![sample screenshot](/images/occt-qopenglwidget-sample-x11.png)

## OCCT QtQuick/QML sample

Project within `occt-qtquick` subfolder shows OCCT 3D viewer setup
from *OpenGL* context created by `QQuickFramebufferObject` within Qt5 QtQuick/QML application.

The approach with `QQuickFramebufferObject` requires careful gluing layer for Qt and OCCT 3D Viewer to share common OpenGL context.

## Common tips

### QSGRenderThread

`QtQuick` by default will attempt offloading rendering into a separate working thread (`QSGRenderThread`) on some systems,
which might improve application GUI performance in some cases.

This, however, requires addition of multithreading synchronization mechanism when dealing with OCCT 3D Viewer from GUI thread.
Uncomment lines setting `QSG_RENDER_LOOP` environment variable,
if these complexities are undesired for your application to ask Qt managing rendering from GUI thread.

It is still responsibility of application logic to offload expensive operations (e.g. not GUI and rendering)
into a separate threads - see next tip.

### Background thread for expensive operations

CAD applications usually perform many operations that might take considerable time.
When called from the main (or rendering) threads, these operations will make GUI and 3D Viewer unresponsive.
Apart from poor user experience, some system might suggest to terminate hanged applications.

The preferred approach is to offload into a background thread operations like:
- Blocking operations (reading from files, network);
- Import/export of models;
- Modeling operations (Booleans and others);
- Other expensive algorithms (like `BRepMesh`).

### Message log

Provide `Message_Printer` implementation to redirect messages coming from OCCT algorithms to end user (GUI)
or to application logs (for developers).

Define your own handler for Qt messages via `qInstallMessageHandler`;
redirect these messages to `Message::DefaultMessenger()`, to GUI or to application logs.

### Exception handling

OCCT algorithms may raise C++ exceptions.
Don't forget to put algorithms into `try`/`catch` blocks to prevent GUI crashes.

Override `QApplication::notify()` to catch `Standard_Failure` exceptions
for gracefully handling internal/algorithmic errors to end user without crashing application.

Use `OSD::SetSignal()` to further protect application from crashes due to internal OCCT errors,
although this mechanism may hinder such OCCT bugs and should be used with caution.
