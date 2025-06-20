// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-17
import QtQuick
import QtQuick.Layouts
import MySongPlayer

Rectangle {
    id: root
    
    height: AppStyles.bottomBarHeight
    color: AppStyles.surfaceColor

    ColumnLayout {
        anchors.fill: parent
        spacing: AppStyles.mediumSpacing

        AudioProgressBar {
            id: progressBar
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: AppStyles.mediumSpacing
            spacing: AppStyles.mediumSpacing

            Item {
                Layout.preferredWidth: AppStyles.controlBarHeight * 4
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignVCenter

                PlayModeButton {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: AppStyles.controlButton
                    height: AppStyles.controlButton
                    opacity: PlayerController.currentSong ? 1 : 0.3
                }
            }

            // 中间容器 - 播放按钮组居中显示
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignVCenter
                
                RowLayout {
                    anchors.centerIn: parent
                    spacing: 15
                    enabled: !!PlayerController.currentSong
                    opacity: enabled ? 1 : 0.3

                    ImageButton {
                        id: previousButton
                        Layout.preferredWidth: AppStyles.controlButton
                        Layout.preferredHeight: AppStyles.controlButton
                        source: AppStyles.previousIcon
                        onClicked: PlayerController.switchToPreviousSong()
                    }

                    ImageButton {
                        id: playPauseButton
                        Layout.preferredWidth: AppStyles.playButton
                        Layout.preferredHeight: AppStyles.playButton
                        source: PlayerController.playing ? AppStyles.pauseIcon : AppStyles.playIcon
                        onClicked: PlayerController.playPause()
                    }

                    ImageButton {
                        id: nextButton
                        Layout.preferredWidth: AppStyles.controlButton
                        Layout.preferredHeight: AppStyles.controlButton
                        source: AppStyles.nextIcon
                        onClicked: PlayerController.switchToNextSong()
                    }
                }
            }

            VolumeSlider {
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: AppStyles.controlBarHeight * 4
                Layout.fillHeight: true
                opacity: PlayerController.currentSong ? 1 : 0.3
            }
        }
    }
}
