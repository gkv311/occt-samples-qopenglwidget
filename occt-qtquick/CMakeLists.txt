cmake_minimum_required (VERSION 3.2)

project (occt-qtquick-sample)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../adm/cmake" ${CMAKE_MODULE_PATH})
set (APP_VERSION_MAJOR 1)
set (APP_VERSION_MINOR 0)

# compiler flags
set (CMAKE_CXX_STANDARD 11)
if (MSVC)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:precise /EHa /MP")
  string (REGEX REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  add_definitions (-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -DUNICODE)
else()
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -fPIC")
  add_definitions (-DOCC_CONVERT_SIGNALS)
endif()

# increase compiler warnings level (-W3 for MSVC, -Wextra for GCC)
if (MSVC)
  if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string (REGEX REPLACE "/W[0-4]" "/W3" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
  endif()
elseif (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshorten-64-to-32")
  endif()
  if (BUILD_SHARED_LIBS)
    if (APPLE)
      set (CMAKE_SHARED_LINKER_FLAGS "-lm ${CMAKE_SHARED_LINKER_FLAGS}")
    elseif (NOT WIN32)
      set (CMAKE_SHARED_LINKER_FLAGS "-lm ${CMAKE_SHARED_LINKER_FLAGS}")
    endif()
  endif()
endif()

# Find dependencies
set (OpenCASCADE_DIR "" CACHE PATH "Path to Open CASCADE libraries.")
if (MSVC)
  set (3RDPARTY_DLL_DIRS "" CACHE STRING "Paths to external DLLs separated by semicolon (FreeImage, FreeType, etc.)")
endif()

# Find OpenGL
find_package (OpenGL REQUIRED)

# Find Qt
set (QT_VERSION "Qt5" CACHE STRING "Qt major version to use")
set_property (CACHE QT_VERSION PROPERTY STRINGS Qt5 Qt6)
set (CMAKE_AUTOUIC ON)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)
set_property (GLOBAL PROPERTY AUTOMOC_SOURCE_GROUP "Generated Files/Moc")
set_property (GLOBAL PROPERTY AUTORCC_SOURCE_GROUP "Generated Files/Resources")
set (Qt_COMPONENTS Core Gui Qml Quick QuickControls2 Widgets)
set (Qt_LIBS)
foreach (aLibIter ${Qt_COMPONENTS})
  list (APPEND Qt_LIBS "${QT_VERSION}::${aLibIter}")
endforeach()
find_package (${QT_VERSION} COMPONENTS ${Qt_COMPONENTS} REQUIRED)
message (STATUS "Using ${QT_VERSION} from \"${${QT_VERSION}_DIR}\"")

# Find Open CASCADE Technology
find_package (OpenCASCADE REQUIRED)
if (NOT OpenCASCADE_FOUND)
  message (FATAL_ERROR "could not find OpenCASCADE, please set OpenCASCADE_DIR variable" )
else()
  message (STATUS "Using OpenCASCADE from \"${OpenCASCADE_INSTALL_PREFIX}\"" )
  message (STATUS "OpenCASCADE_INCLUDE_DIR=${OpenCASCADE_INCLUDE_DIR}")
  message (STATUS "OpenCASCADE_LIBRARY_DIR=${OpenCASCADE_LIBRARY_DIR}")
  include_directories(${OpenCASCADE_INCLUDE_DIR})
  link_directories   (${OpenCASCADE_LIBRARY_DIR})
endif()
set (OpenCASCADE_LIBS TKRWMesh TKBinXCAF TKBin TKBinL TKOpenGl TKXCAF TKVCAF TKCAF TKV3d TKHLR TKMesh TKService TKShHealing TKPrim TKTopAlgo TKGeomAlgo TKBRep TKGeomBase TKG3d TKG2d TKMath TKLCAF TKCDF TKernel)

# main project target
add_executable (${PROJECT_NAME}
  ../occt-qt-tools/OcctQtTools.h
  ../occt-qt-tools/OcctQtTools.cpp
  ../occt-qt-tools/OcctGlTools.h
  ../occt-qt-tools/OcctGlTools.cpp
  main.cpp
  main.qml
  OcctQQuickFramebufferViewer.h
  OcctQQuickFramebufferViewer.cpp
  occt-qtquick.qrc
)
target_link_libraries (${PROJECT_NAME} PRIVATE
  ${Qt_LIBS}
  ${OpenCASCADE_LIBS}
  ${OPENGL_LIBRARIES}
)

# auxiliary development environment
if (MSVC)
  set (X_COMPILER_BITNESS x64)
  get_target_property (QtCore_location ${QT_VERSION}::Core LOCATION)
  get_filename_component (QT_BINARY_DIR ${QtCore_location} DIRECTORY)
  set (QT_PLUGINS_DIR)
  if (EXISTS "${QT_BINARY_DIR}/../plugins")
    set (QT_PLUGINS_DIR "${QT_BINARY_DIR}/../plugins")
  endif()

  get_target_property (aTKernelRel "TKernel" IMPORTED_LOCATION_RELEASE)
  get_target_property (aTKernelDbg "TKernel" IMPORTED_LOCATION_DEBUG)
  get_filename_component (OpenCASCADE_BINARY_DIR_RELEASE ${aTKernelRel} DIRECTORY)
  get_filename_component (OpenCASCADE_BINARY_DIR_DEBUG   ${aTKernelDbg} DIRECTORY)
  if (NOT EXISTS "${OpenCASCADE_BINARY_DIR_DEBUG}" AND EXISTS "${OpenCASCADE_BINARY_DIR_RELEASE}")
    set (OpenCASCADE_BINARY_DIR_DEBUG "${OpenCASCADE_BINARY_DIR_RELEASE}")
  elseif (NOT EXISTS "${OpenCASCADE_BINARY_DIR_RELEASE}" AND EXISTS "${OpenCASCADE_BINARY_DIR_DEBUG}")
    set (OpenCASCADE_BINARY_DIR_RELEASE "${OpenCASCADE_BINARY_DIR_DEBUG}")
  endif()

  set_target_properties (${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_ENVIRONMENT "\
PATH=%PATH%;$<IF:$<CONFIG:Debug>,${OpenCASCADE_BINARY_DIR_DEBUG},${OpenCASCADE_BINARY_DIR_RELEASE}>;${3RDPARTY_DLL_DIRS};${QT_BINARY_DIR}\n\
QT_PLUGIN_PATH=${QT_PLUGINS_DIR}"
  )
endif()
