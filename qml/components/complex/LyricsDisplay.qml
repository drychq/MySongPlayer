// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-19
import QtQuick
import QtQuick.Controls
import MySongPlayer
Item {
    id: root

    //property LyricsModel lyricsModel: null
    property color primaryTextColor: AppStyles.textPrimary
    property color highlightTextColor: AppStyles.primaryColor
    property color secondaryTextColor: AppStyles.textSecondary
    
    Rectangle {
        id: lyricsContainer
        anchors.fill: parent
        color: AppStyles.transparentColor
        
        Text {
            id: noLyricsText
            anchors.centerIn: parent
            text: qsTr("No lyrics available")
            color: root.secondaryTextColor
            font: AppStyles.subtitleFont
            visible: !root.lyricsModel || !root.lyricsModel.hasLyrics
        }
        
        ListView {
            id: lyricsListView
            anchors.fill: parent
            anchors.margins: AppStyles.mediumSpacing
            
            visible: root.lyricsModel && root.lyricsModel.hasLyrics
            model: root.lyricsModel ? root.lyricsModel.allLyrics : null
            
            interactive: false

            preferredHighlightBegin: height / 2 - 30
            preferredHighlightEnd: height / 2 + 30
            highlightRangeMode: ListView.StrictlyEnforceRange
            
            currentIndex: root.lyricsModel ? root.lyricsModel.currentLineIndex : -1

            Behavior on currentIndex {
                NumberAnimation {
                    duration: 500
                    easing.type: Easing.OutCubic
                }
            }
            
            delegate: Item {
                id: lyricItem
                width: lyricsListView.width
                height: lyricText.height + AppStyles.smallSpacing * 2
                
                Text {
                    id: lyricText
                    anchors.centerIn: parent
                    width: parent.width - AppStyles.mediumSpacing * 2
                    
                    text: modelData
                    color: (root.lyricsModel && index === root.lyricsModel.currentLineIndex) ? 
                           root.highlightTextColor : root.primaryTextColor
                    
                    font.pixelSize: (root.lyricsModel && index === root.lyricsModel.currentLineIndex) ? 
                                   AppStyles.titleFont.pixelSize : AppStyles.bodyFont.pixelSize
                    font.weight: (root.lyricsModel && index === root.lyricsModel.currentLineIndex) ? 
                                Font.DemiBold : Font.Normal
                    
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    
                    Behavior on color {
                        ColorAnimation {
                            duration: 300
                            easing.type: Easing.OutQuad
                        }
                    }
                    
                    Behavior on font.pixelSize {
                        NumberAnimation {
                            duration: 300
                            easing.type: Easing.OutQuad
                        }
                    }
                    
                    opacity: (root.lyricsModel && index === root.lyricsModel.currentLineIndex) ? 1.0 : 0.7
                    
                    Behavior on opacity {
                        NumberAnimation {
                            duration: 300
                            easing.type: Easing.OutQuad
                        }
                    }
                }
            }
            
            onCurrentIndexChanged: {
                if (currentIndex >= 0 && currentIndex < count) {
                    positionViewAtIndex(currentIndex, ListView.Center)
                }
            }
        }
        
        Rectangle {
            id: topGradient
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: AppStyles.controlBarHeight + AppStyles.mediumSpacing
            
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: AppStyles.backgroundColor
                }
                GradientStop {
                    position: 1.0
                    color: AppStyles.transparentColor
                }
            }
            
            visible: root.lyricsModel && root.lyricsModel.hasLyrics
        }
        
        Rectangle {
            id: bottomGradient
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: AppStyles.controlBarHeight + AppStyles.mediumSpacing
            
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: AppStyles.transparentColor
                }
                GradientStop {
                    position: 1.0
                    color: AppStyles.backgroundColor
                }
            }
            
            visible: root.lyricsModel && root.lyricsModel.hasLyrics
        }
    }
    
} 
