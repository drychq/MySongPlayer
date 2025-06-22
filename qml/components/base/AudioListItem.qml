// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-20
import QtQuick
import QtQuick.Layouts
import MySongPlayer

Rectangle {
    id: root

    property string audioTitle: ""
    property string audioAuthor: ""
    property url audioImageSource: ""
    property url audioSource: ""
    property url audioVideoSource: ""
    property int audioIndex: -1

    property color itemColor: AppStyles.backgroundColor
    property color textPrimaryColor: AppStyles.textPrimary
    property color textSecondaryColor: AppStyles.textSecondary
    property bool showImage: true
    property bool showActionButton: true
    property string actionButtonIcon: ""

    signal clicked()
    signal actionClicked()
    width: parent ? parent.width : 200
    height: AppStyles.listItemHeight
    color: root.itemColor

    RowLayout {
        anchors.fill: parent
        anchors.margins: AppStyles.smallSpacing
        spacing: AppStyles.smallSpacing

        Image {
            id: audioImage
            Layout.preferredWidth: AppStyles.mediumIcon
            Layout.preferredHeight: AppStyles.mediumIcon
            Layout.alignment: Qt.AlignVCenter
            source: root.audioImageSource
            visible: root.showImage
            mipmap: true
            fillMode: Image.PreserveAspectFit
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: AppStyles.smallSpacing


            Text {
                Layout.fillWidth: true
                text: root.audioTitle
                color: root.textPrimaryColor
                fontSizeMode: Text.Fit
                minimumPixelSize: 12
                elide: Text.ElideRight
                font: AppStyles.subtitleFont
            }

            Text {
                Layout.fillWidth: true
                text: root.audioAuthor
                color: root.textSecondaryColor
                fontSizeMode: Text.Fit
                minimumPixelSize: 8
                elide: Text.ElideRight
                font: AppStyles.smallFont
            }
        }

        ImageButton {
            id: actionButton
            Layout.preferredWidth: AppStyles.mediumIcon
            Layout.preferredHeight: AppStyles.mediumIcon
            Layout.alignment: Qt.AlignVCenter
            source: root.actionButtonIcon
            visible: root.showActionButton
            onClicked: root.actionClicked()
        }
    }


    TapHandler {
        gesturePolicy: TapHandler.ReleaseWithinBounds
        onTapped: root.clicked()
    }


    property bool enableHoverEffect: true
    opacity: (enableHoverEffect && mainHoverHandler.hovered) ? 0.8 : 1.0

    HoverHandler {
        id: mainHoverHandler
        enabled: root.enableHoverEffect
    }

    Behavior on opacity {
        NumberAnimation {
            duration: AppStyles.shortAnimation
        }
    }
}

