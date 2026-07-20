import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MySongPlayer

Popup {
    id: root

    signal localImportRequested()
    signal networkImportRequested()

    width: 280
    height: 200
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // Background event catcher, prevents click through to MainArea
    TapHandler {
        gesturePolicy: TapHandler.ReleaseWithinBounds
        // Empty onTapped handler, only used to catch events
        onTapped: {}
    }

    anchors.centerIn: Overlay.overlay

    background: Rectangle {
        color: AppStyles.surfaceColor
        radius: 8
        border.color: AppStyles.primaryColor
        border.width: 2
    }

    ColumnLayout {
        id: contentLayout
        anchors.fill: parent
        anchors.margins: AppStyles.largeSpacing
        spacing: AppStyles.mediumSpacing


        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Add Song")
            font: AppStyles.titleFont
            color: AppStyles.textPrimary
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: AppStyles.primaryColor
            opacity: 0.5
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: AppStyles.mediumSpacing

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                color: localImportHoverHandler.hovered ? Qt.lighter(AppStyles.primaryColor, 1.2) : AppStyles.primaryColor
                radius: 6

                Behavior on color {
                    ColorAnimation {
                        duration: AppStyles.shortAnimation
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: AppStyles.mediumSpacing
                    spacing: AppStyles.mediumSpacing

                    ImageButton {
                        // Test hook for the offscreen single-signal interaction test.
                        objectName: "localImportButton"
                        Layout.preferredWidth: AppStyles.mediumIcon
                        Layout.preferredHeight: AppStyles.mediumIcon
                        source: AppStyles.addIcon

                        onClicked: {
                            root.localImportRequested()
                        }
                    }

                    Item {
                        // Test hook paired with localImportButton to verify disjoint tap regions.
                        objectName: "localImportSelectionArea"
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Text {
                            anchors.fill: parent
                            text: qsTr("Add from local file")
                            font: AppStyles.bodyFont
                            color: AppStyles.textPrimary
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                        }

                        TapHandler {
                            id: localImportTapHandler
                            gesturePolicy: TapHandler.ReleaseWithinBounds
                            onTapped: root.localImportRequested()
                        }
                    }
                }

                HoverHandler {
                    id: localImportHoverHandler
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                color: networkImportHoverHandler.hovered ? Qt.lighter(AppStyles.surfaceColor, 1.3) : Qt.lighter(AppStyles.surfaceColor, 1.1)
                radius: 6
                border.color: AppStyles.primaryColor
                border.width: 1

                Behavior on color {
                    ColorAnimation {
                        duration: AppStyles.shortAnimation
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: AppStyles.mediumSpacing
                    spacing: AppStyles.mediumSpacing

                    ImageButton {
                        Layout.preferredWidth: AppStyles.mediumIcon
                        Layout.preferredHeight: AppStyles.mediumIcon
                        source: AppStyles.searchIcon

                        onClicked: {
                            root.networkImportRequested()
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Text {
                            anchors.fill: parent
                            text: qsTr("Add from network")
                            font: AppStyles.bodyFont
                            color: AppStyles.textPrimary
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                        }

                        TapHandler {
                            id: networkImportTapHandler
                            gesturePolicy: TapHandler.ReleaseWithinBounds
                            onTapped: root.networkImportRequested()
                        }
                    }
                }

                HoverHandler {
                    id: networkImportHoverHandler
                }
            }
        }
    }

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: AppStyles.mediumAnimation
        }
        NumberAnimation {
            property: "scale"
            from: 0.9
            to: 1.0
            duration: AppStyles.mediumAnimation
            easing.type: Easing.OutQuart
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: AppStyles.shortAnimation
        }
        NumberAnimation {
            property: "scale"
            from: 1.0
            to: 0.95
            duration: AppStyles.shortAnimation
            easing.type: Easing.InQuart
        }
    }
}
