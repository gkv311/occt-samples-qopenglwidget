// Copyright (c) 2025 Kirill Gavrilov

#ifndef _OcctQtTools_HeaderFile
#define _OcctQtTools_HeaderFile

#include <Aspect_VKey.hxx>
#include <Quantity_Color.hxx>

#include <Standard_WarningsDisable.hxx>
#include <QColor>
#include <QMouseEvent>
#include <Standard_WarningsRestore.hxx>

//! Auxiliary tools between Qt and OCCT definitions.
namespace OcctQtTools
{
//! Map QColor into Quantity_Color.
inline Quantity_Color qtColorToOcct(const QColor& theColor)
{
  return Quantity_Color(theColor.red() / 255.0, theColor.green() / 255.0, theColor.blue() / 255.0, Quantity_TOC_sRGB);
}

//! Map Quantity_Color into QColor.
inline QColor qtColorFromOcct(const Quantity_Color& theColor)
{
  NCollection_Vec3<double> anRgb; theColor.Values(anRgb.r(), anRgb.g(), anRgb.b(), Quantity_TOC_sRGB);
  return QColor((int)Round(anRgb.r() * 255.0), (int)Round(anRgb.g() * 255.0), (int)Round(anRgb.b() * 255.0));
}

//! Map Qt buttons bitmask to virtual keys.
inline Aspect_VKeyMouse qtMouseButtons2VKeys(Qt::MouseButtons theButtons);

//! Map Qt mouse modifiers bitmask to virtual keys.
inline Aspect_VKeyFlags qtMouseModifiers2VKeys(Qt::KeyboardModifiers theModifiers);

//! Map Qt key to virtual key.
inline Aspect_VKey qtKey2VKey(int theKey);
} // namespace OcctQtTools

// ================================================================
// Function : qtMouseButtons2VKeys
// ================================================================
inline Aspect_VKeyMouse OcctQtTools::qtMouseButtons2VKeys(Qt::MouseButtons theButtons)
{
  Aspect_VKeyMouse aButtons = Aspect_VKeyMouse_NONE;
  if ((theButtons & Qt::LeftButton) != 0)
    aButtons |= Aspect_VKeyMouse_LeftButton;

  if ((theButtons & Qt::MiddleButton) != 0)
    aButtons |= Aspect_VKeyMouse_MiddleButton;

  if ((theButtons & Qt::RightButton) != 0)
    aButtons |= Aspect_VKeyMouse_RightButton;

  return aButtons;
}

// ================================================================
// Function : qtMouseModifiers2VKeys
// ================================================================
inline Aspect_VKeyFlags OcctQtTools::qtMouseModifiers2VKeys(Qt::KeyboardModifiers theModifiers)
{
  Aspect_VKeyFlags aFlags = Aspect_VKeyFlags_NONE;
  if ((theModifiers & Qt::ShiftModifier) != 0)
    aFlags |= Aspect_VKeyFlags_SHIFT;

  if ((theModifiers & Qt::ControlModifier) != 0)
    aFlags |= Aspect_VKeyFlags_CTRL;

  if ((theModifiers & Qt::AltModifier) != 0)
    aFlags |= Aspect_VKeyFlags_ALT;

  return aFlags;
}

// ================================================================
// Function : qtKey2VKey
// ================================================================
inline Aspect_VKey OcctQtTools::qtKey2VKey(int theKey)
{
  switch (theKey)
  {
    case 1060: // ru
    case Qt::Key_A:
      return Aspect_VKey_A;
    case 1048: // ru
    case Qt::Key_B:
      return Aspect_VKey_B;
    case 1057: // ru
    case Qt::Key_C:
      return Aspect_VKey_C;
    case 1042: // ru
    case Qt::Key_D:
      return Aspect_VKey_D;
    case 1059: // ru
    case Qt::Key_E:
      return Aspect_VKey_E;
    case 1040: // ru
    case Qt::Key_F:
      return Aspect_VKey_F;
    case Qt::Key_G:
      return Aspect_VKey_G;
    case Qt::Key_H:
      return Aspect_VKey_H;
    case Qt::Key_I:
      return Aspect_VKey_I;
    case Qt::Key_J:
      return Aspect_VKey_J;
    case Qt::Key_K:
      return Aspect_VKey_K;
    case 1044: // ru
    case Qt::Key_L:
      return Aspect_VKey_L;
    case Qt::Key_M:
      return Aspect_VKey_M;
    case Qt::Key_N:
      return Aspect_VKey_N;
    case Qt::Key_O:
      return Aspect_VKey_O;
    case Qt::Key_P:
      return Aspect_VKey_P;
    case 1049: // ru
    case Qt::Key_Q:
      return Aspect_VKey_Q;
    case 1050: // ru
    case Qt::Key_R:
      return Aspect_VKey_R;
    case 1067: // ru
    case Qt::Key_S:
      return Aspect_VKey_S;
    case 1045: // ru
    case Qt::Key_T:
      return Aspect_VKey_T;
    case Qt::Key_U:
      return Aspect_VKey_U;
    case 1052: // ru
    case Qt::Key_V:
      return Aspect_VKey_V;
    case 1062: // ru
    case Qt::Key_W:
      return Aspect_VKey_W;
    case 1063: // ru
    case Qt::Key_X:
      return Aspect_VKey_X;
    case Qt::Key_Y:
      return Aspect_VKey_Y;
    case 1071: // ru
    case Qt::Key_Z:
      return Aspect_VKey_Z;
      //
    case Qt::Key_0:
      return Aspect_VKey_0;
    case Qt::Key_1:
      return Aspect_VKey_1;
    case Qt::Key_2:
      return Aspect_VKey_2;
    case Qt::Key_3:
      return Aspect_VKey_3;
    case Qt::Key_4:
      return Aspect_VKey_4;
    case Qt::Key_5:
      return Aspect_VKey_5;
    case Qt::Key_6:
      return Aspect_VKey_6;
    case Qt::Key_7:
      return Aspect_VKey_7;
    case Qt::Key_8:
      return Aspect_VKey_8;
    case Qt::Key_9:
      return Aspect_VKey_9;
      //
    case Qt::Key_F1:
      return Aspect_VKey_F1;
    case Qt::Key_F2:
      return Aspect_VKey_F2;
    case Qt::Key_F3:
      return Aspect_VKey_F3;
    case Qt::Key_F4:
      return Aspect_VKey_F4;
    case Qt::Key_F5:
      return Aspect_VKey_F5;
    case Qt::Key_F6:
      return Aspect_VKey_F6;
    case Qt::Key_F7:
      return Aspect_VKey_F7;
    case Qt::Key_F8:
      return Aspect_VKey_F8;
    case Qt::Key_F9:
      return Aspect_VKey_F9;
    case Qt::Key_F10:
      return Aspect_VKey_F10;
    case Qt::Key_F11:
      return Aspect_VKey_F11;
    case Qt::Key_F12:
      return Aspect_VKey_F12;
      //
    case Qt::Key_Up:
      return Aspect_VKey_Up;
    case Qt::Key_Left:
      return Aspect_VKey_Left;
    case Qt::Key_Right:
      return Aspect_VKey_Right;
    case Qt::Key_Down:
      return Aspect_VKey_Down;
    case Qt::Key_Plus:
      return Aspect_VKey_Plus;
    case Qt::Key_Minus:
      return Aspect_VKey_Minus;
    case Qt::Key_Equal:
      return Aspect_VKey_Equal;
    case Qt::Key_PageDown:
      return Aspect_VKey_PageDown;
    case Qt::Key_PageUp:
      return Aspect_VKey_PageUp;
    case Qt::Key_Home:
      return Aspect_VKey_Home;
    case Qt::Key_End:
      return Aspect_VKey_End;
    case Qt::Key_Escape:
      return Aspect_VKey_Escape;
    case Qt::Key_Back:
      return Aspect_VKey_Back;
    case Qt::Key_Enter:
      return Aspect_VKey_Enter;
    case Qt::Key_Backspace:
      return Aspect_VKey_Backspace;
    case Qt::Key_Space:
      return Aspect_VKey_Space;
    case Qt::Key_Delete:
      return Aspect_VKey_Delete;
    case Qt::Key_Tab:
      return Aspect_VKey_Tab;
    case 1025:
    case Qt::Key_QuoteLeft:
      return Aspect_VKey_Tilde;
      //
    case Qt::Key_Shift:
      return Aspect_VKey_Shift;
    case Qt::Key_Control:
      return Aspect_VKey_Control;
    case Qt::Key_Alt:
      return Aspect_VKey_Alt;
    case Qt::Key_Menu:
      return Aspect_VKey_Menu;
    case Qt::Key_Meta:
      return Aspect_VKey_Meta;
    default:
      return Aspect_VKey_UNKNOWN;
  }
}

#endif // _OcctQtTools_HeaderFile
