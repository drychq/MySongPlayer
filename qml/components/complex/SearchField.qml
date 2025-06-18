// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-17
import QtQuick
import QtQuick.Controls
import MySongPlayer

Rectangle {
    id: root

    property alias text: searchInput.text
    signal accepted(string value)

    color: "#1e1e1e"
    border.color: searchInput.activeFocus ? Qt.lighter("#5F8575") : "transparent"
    border.width: 1

    opacity: enabled ? 1 : 0.6

    TextField {
        id: searchInput

        anchors.fill: parent

        font: AppStyles.bodyFont
        color: AppStyles.textPrimary

        leftPadding: 30
        verticalAlignment: TextInput.AlignVCenter

        Image {
            anchors {
                left: parent.left
                leftMargin: 5
                verticalCenter: parent.verticalCenter
            }

            width: 16
            height: 16

            mipmap: true
            source: AppStyles.searchIcon
        }

        onAccepted: {
            root.accepted(text)
        }
    }
}
