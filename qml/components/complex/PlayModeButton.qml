// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-19
import QtQuick
import MySongPlayer

ImageButton {
    id: root
    
    readonly property int modeLoop: 0
    readonly property int modeShuffle: 1  
    readonly property int modeRepeatOne: 2
    
    source: {
        switch(PlayerController.playMode) {
            case modeLoop:
                return AppStyles.listCycleIcon
            case modeShuffle:
                return AppStyles.randomIcon
            case modeRepeatOne:
                return AppStyles.repeatOneIcon
            default:
                return AppStyles.listCycleIcon
        }
    }
    
    onClicked: {
        const currentMode = PlayerController.playMode
        let nextMode
        
        switch(currentMode) {
            case modeLoop:
                nextMode = modeShuffle
                break
            case modeShuffle:
                nextMode = modeRepeatOne
                break
            case modeRepeatOne:
                nextMode = modeLoop
                break
            default:
                nextMode = modeLoop
        }
        
        PlayerController.playMode = nextMode
    }

    Behavior on source {
        SequentialAnimation {
            NumberAnimation { target: root; property: "opacity"; to: 0.5; duration: AppStyles.shortAnimation }
            PropertyAction { target: root; property: "source" }
            NumberAnimation { target: root; property: "opacity"; to: 1.0; duration: AppStyles.shortAnimation }
        }
    }
}
