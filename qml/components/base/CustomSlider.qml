// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-19
import QtQuick
import QtQuick.Controls
import MySongPlayer

Slider {
    id: root

    property color sliderColor: AppStyles.primaryColor
    property color backgroundColor: AppStyles.transparentWhite
    property int handleSize: AppStyles.sliderHandle
    property int trackHeight: AppStyles.sliderTrack
    property bool showClickableArea: false
    property bool enableDragFeedback: false

    property bool isDragging: pressed
    property real dragValue: value

    signal dragStarted()
    signal dragEnded()
    signal clicked(real clickPosition)

    onPressedChanged: {
        if (pressed) {
            root.dragStarted()
        } else {
            root.dragEnded()
        }
    }

    onMoved: {
        if (root.isDragging) {
            root.dragValue = value
        }
    }

    handle: Rectangle {
        x: root.visualPosition * (root.width - width)
        y: (root.height - height) / 2
        width: root.handleSize
        height: root.handleSize
        radius: root.handleSize / 2
        color: root.sliderColor

        Behavior on scale {
            NumberAnimation {
                duration: AppStyles.shortAnimation
            }

        }

        scale: root.pressed ? 1.2 : 1.0
    }

    background: Rectangle {
        x: 0
        y: (root.height - height) / 2
        width: root.width
        height: root.trackHeight
        radius: root.trackHeight / 2
        color: root.backgroundColor

        Rectangle {
            width: root.visualPosition * parent.width
            height: parent.height
            color: root.sliderColor
            radius: parent.radius
        }

        TapHandler {
            enabled: root.showClickableArea && root.enabled

            onTapped: function(eventPoint) {
                let handleX = root.visualPosition * root.width
                let handleArea = root.handleSize
                let clickX = eventPoint.position.x

                if (Math.abs(clickX - handleX) > handleArea / 2) {
                    let clickPosition = Math.max(0, Math.min(1, clickX / width))
                    root.clicked(clickPosition)
                }
            }
        }
    }
}
