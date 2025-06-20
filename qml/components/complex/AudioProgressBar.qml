// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-19
import QtQuick
import QtQuick.Layouts
import MySongPlayer

Item {
    id: root
    
    property bool isDragging: false
    property real temporaryPosition: 0.0
    
    property real lastUpdateTime: 0
    readonly property real updateInterval: 100
    
    property color sliderColor: AppStyles.primaryColor
    property color backgroundColor: AppStyles.transparentWhite
    property color textColor: AppStyles.textPrimary
    
    Connections {
        target: PlayerController
        enabled: !root.isDragging
        
        function onPositionChanged() {
            let currentTime = Date.now()
            if (currentTime - root.lastUpdateTime < root.updateInterval) {
                return
            }
            root.lastUpdateTime = currentTime
        }
        
        function onDurationChanged() {
            if (PlayerController.duration <= 0) {
                root.isDragging = false
                root.temporaryPosition = 0
            }
        }
        
        function onCurrentSongChanged() {
            root.isDragging = false
            root.temporaryPosition = 0
            root.lastUpdateTime = 0
        }
        
        function onPlayingChanged() {
        }
    }
    
    RowLayout {
        anchors.fill: parent
        spacing: AppStyles.smallSpacing
        
        Text {
            id: currentTimeText
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: AppStyles.progressTimeWidth
            color: root.textColor
            font: AppStyles.smallFont
            horizontalAlignment: Text.AlignRight
            visible: PlayerController.currentSong

            text: {
                if (root.isDragging && PlayerController.duration > 0) {
                    return TimeUtils.formatTime(root.temporaryPosition * PlayerController.duration)
                } else {
                    return TimeUtils.formatTime(PlayerController.position || 0)
                }
            }

            opacity: (PlayerController.currentSong && PlayerController.duration > 0) ? 1.0 : 0.5
            
            Behavior on text {
                enabled: !root.isDragging && PlayerController.playing
                SequentialAnimation {
                    NumberAnimation { 
                        target: currentTimeText
                        property: "opacity"
                        to: 0.7
                        duration: 50
                    }
                    NumberAnimation { 
                        target: currentTimeText
                        property: "opacity" 
                        to: 1.0
                        duration: 50
                    }
                }
            }
        }

        Item {
            Layout.preferredWidth: AppStyles.progressTimeWidth
            visible: !PlayerController.currentSong
        }
        
        CustomSlider {
            id: progressSlider
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: AppStyles.progressBarHeight
            
            from: 0
            to: 1
            value: {
                if (root.isDragging) {
                    return root.temporaryPosition
                }
                let progress = TimeUtils.getProgress(PlayerController.position, PlayerController.duration)
                return Math.max(0, Math.min(1, progress))
            }
            
            enabled: !!PlayerController.currentSong && PlayerController.duration > 0
            
            sliderColor: root.sliderColor
            backgroundColor: root.backgroundColor
            showClickableArea: true
            enableDragFeedback: true
            
            onPressedChanged: {
                if (pressed) {
                    root.isDragging = true
                    root.temporaryPosition = Math.max(0, Math.min(1, value))
                } else {
                    if (PlayerController.duration > 0 && PlayerController.currentSong) {
                        let newPosition = TimeUtils.getPositionFromProgress(root.temporaryPosition, PlayerController.duration)
                        newPosition = Math.max(0, Math.min(PlayerController.duration, newPosition))
                        PlayerController.setPosition(newPosition)
                    }
                    root.isDragging = false
                    root.lastUpdateTime = Date.now()
                }
            }
            
            onMoved: {
                if (root.isDragging) {
                    root.temporaryPosition = Math.max(0, Math.min(1, value))
                }
            }
            
            onClicked: function(clickPosition) {
                if (PlayerController.duration > 0 && PlayerController.currentSong) {
                    clickPosition = Math.max(0, Math.min(1, clickPosition))
                    let newPosition = TimeUtils.getPositionFromProgress(clickPosition, PlayerController.duration)
                    newPosition = Math.max(0, Math.min(PlayerController.duration, newPosition))
                    PlayerController.setPosition(newPosition)
                    root.lastUpdateTime = Date.now()
                }
            }

            Behavior on opacity {
                NumberAnimation { 
                    duration: AppStyles.shortAnimation 
                }
            }
            
            opacity: enabled ? 1.0 : 0.3
        }


        Item {
            Layout.preferredWidth: AppStyles.progressTimeWidth
            visible: !PlayerController.currentSong
        }

        Text {
            id: totalTimeText
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: AppStyles.progressTimeWidth
            color: root.textColor
            font: AppStyles.smallFont
            horizontalAlignment: Text.AlignLeft
            text: TimeUtils.formatTime(PlayerController.duration || 0)
            visible: PlayerController.currentSong
            
            opacity: (PlayerController.currentSong && PlayerController.duration > 0) ? 1.0 : 0.5
        }
    }
    
    Rectangle {
        anchors.fill: parent
        color: AppStyles.transparentColor
        border.color: AppStyles.primaryColor
        border.width: root.isDragging ? AppStyles.standardBorderWidth : 0
        radius: AppStyles.progressRadius
        opacity: 0.3
        
        Behavior on border.width {
            NumberAnimation { 
                duration: AppStyles.shortAnimation 
            }
        }
    }
} 
