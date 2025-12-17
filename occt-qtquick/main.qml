import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.2

import OcctQQuickFramebufferViewer 1.0

// Sample application window
ApplicationWindow {
  id: wnd_root
  title: Qt.application.name
  visible: true

  minimumWidth:  200
  minimumHeight: 200
  width:  720
  height: 480

  // OCCT 3D Viewer item
  OcctQQuickFramebufferViewer {
    id: occt_view
    anchors.fill: parent
    focus: true // to accept keyboard events
  }

  // Main menu bar (added to Qt 5.10, QtQuick.Controls 2.3)
  /*MenuBar {
    Menu {
      title: qsTr("&File")
      MenuItem {
        text: qsTr("&Quit")
        onTriggered: Qt.quit();
      }
    }
  }*/

  // Viewer background color slider
  Slider {
    anchors.bottom:       parent.bottom
    anchors.bottomMargin: 10
    anchors.left:         parent.left
    anchors.leftMargin:   20
    anchors.right:        btn_about.left
    anchors.rightMargin:  20
    from:  0
    value: 0
    to:    255
    onMoved: occt_view.backgroundColor = Qt.rgba(value/255.0, value/255.0, value/255.0);
  }

  // About button
  Rectangle {
    id: btn_about
    anchors.bottom:       parent.bottom
    anchors.bottomMargin: 5
    anchors.right:        parent.right
    anchors.rightMargin:  5
    radius: 10
    width:  100
    height: 50
    property var mainColor: "gray"
    color: mainColor
    opacity: 0.5
    border.color: "black"
    Text {
      text: qsTr("About")
      anchors.centerIn: parent
    }

    MouseArea {
      anchors.fill: parent
      onClicked:    dlg_about.open()
      hoverEnabled: true
      onEntered: btn_about.color = "cyan"
      onExited:  btn_about.color = btn_about.mainColor
    }
  }

  // About sample dialog
  MessageDialog {
    id: dlg_about
    title: qsTr("About Sample")
    text:  qsTr("OCCT 3D Viewer sample embedded into QtQuick/QML.")
    informativeText: "Open CASCADE Technology v." + OCC_VERSION_STRING_EXT + "\n"
                   + "Qt v." + QT_VERSION_STR + "\n"
                   + "\nOpenGL info:\n" + occt_view.glInfo;
  }
}
