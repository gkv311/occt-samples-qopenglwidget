cmake_minimum_required (VERSION 3.13)

# dummy project - just to collect files in folder
project (occt-qt-tools)
add_custom_target (${PROJECT_NAME} SOURCES
  OcctQtTools.h
  OcctQtTools.cpp
  OcctGlTools.h
  OcctGlTools.cpp
  ../ReadMe.md
)
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Tools")
