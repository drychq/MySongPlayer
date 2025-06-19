// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-19
import QtQuick
import QtQuick.Controls
import SongPlayer

Slider {
    id: root
    
    // 样式配置属性
    property color sliderColor: AppStyles.primaryColor
    property color backgroundColor: AppStyles.transparentWhite
    property int handleSize: AppStyles.sliderHandle
    property int trackHeight: AppStyles.sliderTrack
    property bool showClickableArea: false
    property bool enableDragFeedback: false
    
    // 拖拽状态相关
    property bool isDragging: pressed
    property real dragValue: value
    
    // 自定义信号
    signal dragStarted()
    signal dragEnded()
    signal clicked(real clickPosition)
    
    // 拖拽状态变化处理
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
    
    // 自定义手柄
    handle: Rectangle {
        x: root.visualPosition * (root.width - width)
        y: (root.height - height) / 2
        width: root.handleSize
        height: root.handleSize
        radius: root.handleSize / 2
        color: root.sliderColor
        
        // 按下时的缩放动画
        Behavior on scale {
            NumberAnimation { 
                duration: AppStyles.shortAnimation 
            }

        }
        
        scale: root.pressed ? 1.2 : 1.0
    }
    
    // 自定义背景轨道
    background: Rectangle {
        x: 0
        y: (root.height - height) / 2
        width: root.width
        height: root.trackHeight
        radius: root.trackHeight / 2
        color: root.backgroundColor
        
        // 进度填充
        Rectangle {
            width: root.visualPosition * parent.width
            height: parent.height
            color: root.sliderColor
            radius: parent.radius
        }
        
        // 修复可点击区域 - 移除对isDragging的依赖
        MouseArea {
            anchors.fill: parent
            enabled: root.showClickableArea && root.enabled
            // 只有在不是通过handle拖拽时才处理点击
            acceptedButtons: Qt.LeftButton
            
            onClicked: function(mouse) {
                let handleX = root.visualPosition * root.width
                let handleArea = root.handleSize
                
                if (Math.abs(mouse.x - handleX) > handleArea / 2) {
                    let clickPosition = Math.max(0, Math.min(1, mouse.x / width))
                    root.clicked(clickPosition)
                }
            }
        }
    }
} 
