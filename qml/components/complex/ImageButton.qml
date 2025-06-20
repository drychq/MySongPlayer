// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-19
import QtQuick
import MySongPlayer

Image {
    id: root

    signal clicked
    fillMode: Image.PreserveAspectFit

    opacity: buttonHoverHandler.hovered ? 0.75 : 1
    mipmap: true


    HoverHandler {
        id: buttonHoverHandler
    }

    TapHandler {
        id: buttonTapHandler

        onTapped: {
            root.clicked()
        }
    }
    
    Behavior on opacity {
        NumberAnimation {
            duration: AppStyles.shortAnimation
        }
    }
} 
