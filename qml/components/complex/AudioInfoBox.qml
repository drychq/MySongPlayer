// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-19
import QtQuick
import QtQuick.Layouts
import QtMultimedia
import MySongPlayer

Item {
    id: root

    visible: !!PlayerController.currentSong

    RowLayout {
        id: audioInfoLayout
        anchors.centerIn: parent
        spacing: AppStyles.largeSpacing

        Item {
            Layout.preferredWidth: AppStyles.audioInfoBoxSize
            Layout.preferredHeight: AppStyles.audioInfoBoxSize
            Layout.alignment: Qt.AlignVCenter

            Image {
                id: albumImage
                anchors.fill: parent
                source: !!PlayerController.currentSong ? PlayerController.currentSong.imageSource : ""
            }

            Video {
                id: albumVideo
                anchors.fill: parent
                loops: MediaPlayer.Infinite
                volume: 0
                source: !!PlayerController.currentSong ? PlayerController.currentSong.videoSource : ""

                onSourceChanged: {
                    if (source !== "") {
                        play()
                    } else {
                        stop()
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignVCenter
            spacing: AppStyles.smallSpacing

            Text {
                id: titleText
                Layout.fillWidth: true
                color: AppStyles.textPrimary
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                text: !!PlayerController.currentSong ? PlayerController.currentSong.title : ""
                font: AppStyles.titleFont
            }

            Text {
                id: authorText
                Layout.fillWidth: true
                color: AppStyles.textSecondary
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                text: !!PlayerController.currentSong ? PlayerController.currentSong.authorName : ""
                font: AppStyles.subtitleFont
            }
        }
    }

    onVisibleChanged: {
        if (visible) {
            albumVideo.play()
        } else {
            albumVideo.seek(0)
            albumVideo.stop()
        }
    }
} 
