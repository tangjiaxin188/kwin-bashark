import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    visible: true
    width: 1920
    height: 1080
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

    Rectangle {
        anchors.fill: parent
        color: "#111827"
    }

    MouseSpark {
        anchors.fill: parent
    }
}
