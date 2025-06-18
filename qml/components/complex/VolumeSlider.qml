// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-17
import QtQuick
import QtQuick.Layouts
import MySongPlayer
import "../base"

Item {
    id: root

    property color sliderColor: AppStyles.primaryColor
    property color backgroundColor: AppStyles.transparentWhite
    property color iconColor: AppStyles.textPrimary
    property int iconSize: AppStyles.smallIcon
    
    RowLayout {
        anchors.fill: parent
        spacing: AppStyles.mediumSpacing

        Image {
            id: volumeIcon
            Layout.preferredWidth: root.iconSize
            Layout.preferredHeight: root.iconSize
            Layout.alignment: Qt.AlignVCenter

            source: {
                if (PlayerController.muted || PlayerController.volume === 0) {
                    return AppStyles.volumeMuteIcon
                } else if (PlayerController.volume < 0.4) {
                    return AppStyles.volumeLowIcon
                } else if (PlayerController.volume >= 0.4 && PlayerController.volume < 0.6) {
                    return AppStyles.volumeMedium
                } else {
                    return AppStyles.volumeHighIcon
                }
            }
            
            mipmap: true
            fillMode: Image.PreserveAspectFit
            opacity: volumeHoverHandler.hovered ? 0.75 : 1
            
            HoverHandler {
                id: volumeHoverHandler
            }
            
            TapHandler {
                onTapped: {
                    PlayerController.muted = !PlayerController.muted
                }
            }
        }
        
        CustomSlider {
            id: volumeSlider
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            
            from: 0
            to: 1
            //value: PlayerController.muted ? 0 : PlayerController.volume
            enabled: !!PlayerController.currentSong
            height: 30
            
            sliderColor: root.sliderColor
            backgroundColor: root.backgroundColor
            
            onMoved: {
                if (PlayerController.muted && value > 0) {
                    PlayerController.muted = false
                }
                PlayerController.volume = value
            }
        }
    }
} 
