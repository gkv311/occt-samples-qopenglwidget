// Copyright (c) 2025 Kirill Gavrilov

#ifndef _OcctQtTools_HeaderFile
#define _OcctQtTools_HeaderFile

#include <Aspect_VKey.hxx>
#include <Quantity_Color.hxx>

#include <Standard_WarningsDisable.hxx>
#include <QColor>
#include <QMouseEvent>
#include <QSurfaceFormat>
#include <Standard_WarningsRestore.hxx>

class OpenGl_Caps;

//! Auxiliary tools between Qt and OCCT definitions.
namespace OcctQtTools
{
//! Return default Qt surface format for GL context.
QSurfaceFormat qtGlSurfaceFormat(QSurfaceFormat::OpenGLContextProfile theProfile = QSurfaceFormat::NoProfile,
                                 bool theToDebug = false);

//! Fill in OCCT GL caps from Qt surface format.
void qtGlCapsFromSurfaceFormat(OpenGl_Caps& theCaps, const QSurfaceFormat& theFormat);

//! Map QColor into Quantity_Color.
Quantity_Color qtColorToOcct(const QColor& theColor);

//! Map Quantity_Color into QColor.
QColor qtColorFromOcct(const Quantity_Color& theColor);

//! Map Qt buttons bitmask to virtual keys.
Aspect_VKeyMouse qtMouseButtons2VKeys(Qt::MouseButtons theButtons);

//! Map Qt mouse modifiers bitmask to virtual keys.
Aspect_VKeyFlags qtMouseModifiers2VKeys(Qt::KeyboardModifiers theModifiers);

//! Map Qt key to virtual key.
Aspect_VKey qtKey2VKey(int theKey);
} // namespace OcctQtTools

#endif // _OcctQtTools_HeaderFile
