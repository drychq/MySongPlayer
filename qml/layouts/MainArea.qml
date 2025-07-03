// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-17
import QtQuick
import MySongPlayer

Rectangle {
    id: root
    
    signal areaClicked()
    
    color: AppStyles.backgroundColor

    HoverHandler {
        onHoveredChanged: {
            if (PlayerController.lyricsModel.hasLyrics) {
                parent.opacity = hovered ? 0.8 : 1.0
            }
        }
    }

    TapHandler {
        onTapped: {
            root.areaClicked()
        }
    }

    AudioInfoBox {
        id: audioInfo
        anchors {
            left: parent.left
            right: parent.right
            verticalCenter: parent.verticalCenter
            margins: AppStyles.largeSpacing
        }
        visible: !PlayerController.lyricsModel.showLyrics
    }
    
    LyricsDisplay {
        id: lyricsDisplay
        anchors.fill: parent
        visible: PlayerController.lyricsModel.showLyrics && PlayerController.lyricsModel.hasLyrics
        lyricsModel: PlayerController.lyricsModel
        
        Behavior on opacity {
            NumberAnimation {
                duration: 300
                easing.type: Easing.OutQuad
            }
        }
    }
} 
