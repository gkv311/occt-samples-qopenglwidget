cmake_minimum_required (VERSION 3.13)

# common solution for multiple independent projects
project (occt-qt)
set_property (GLOBAL PROPERTY USE_FOLDERS ON)
add_subdirectory (occt-qt-tools)
add_subdirectory (occt-qwidget)
add_subdirectory (occt-qopenglwidget)
add_subdirectory (occt-qtquick)
