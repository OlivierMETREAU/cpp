import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls

Window {
    visible: true
    width: 640
    height: 480
    title: "QtQuickApplication1"

    Rectangle {
        anchors.fill: parent
        color: "lightgreen"
    }

    Button {
        anchors.centerIn: parent
        text: "Click Me"
        onClicked: {
            text = helloText
        }
    }
}
