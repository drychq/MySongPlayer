// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-17
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
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

    anchors.centerIn: Overlay.overlay
    
    background: Rectangle {
        color: AppStyles.surfaceColor
        radius: 8
        border.color: AppStyles.primaryColor
        border.width: 1
        
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowHorizontalOffset: 0
            shadowVerticalOffset: 4
            shadowBlur: 0.6
            shadowOpacity: 0.25
            shadowColor: "#000000"
        }
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
                color: localImportMouseArea.containsMouse ? Qt.lighter(AppStyles.primaryColor, 1.2) : AppStyles.primaryColor
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
                        Layout.preferredWidth: AppStyles.mediumIcon
                        Layout.preferredHeight: AppStyles.mediumIcon
                        source: AppStyles.addIcon
                        
                        onClicked: {
                            root.localImportRequested()
                        }
                    }
                    
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Add from local file")
                        font: AppStyles.bodyFont
                        color: AppStyles.textPrimary
                        horizontalAlignment: Text.AlignLeft
                    }
                }
                
                MouseArea {
                    id: localImportMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.localImportRequested()
                    }
                }
            }
            
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                color: networkImportMouseArea.containsMouse ? Qt.lighter(AppStyles.surfaceColor, 1.3) : Qt.lighter(AppStyles.surfaceColor, 1.1)
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
                    
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Add from network")
                        font: AppStyles.bodyFont
                        color: AppStyles.textPrimary
                        horizontalAlignment: Text.AlignLeft
                    }
                }
                
                TapHandler {
                    id: networkImportMouseArea

                    onTapped: {
                        root.networkImportRequested()
                    }
                }
            }
        }
    }
    
    // 进入和退出动画
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
