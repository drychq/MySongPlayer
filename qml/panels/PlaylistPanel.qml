// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-17
import QtQuick
import QtQuick.Layouts
import MySongPlayer
import "../components/base"
import "../components/complex"

Rectangle {
    id: root

    property bool hidden: true

        signal playlistSearchRequested
        signal importRequested

    width: AppStyles.playlistPanelWidth
    height: AppStyles.playlistPanelHeight
    color: AppStyles.surfaceColor

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: AppStyles.mediumSpacing
        spacing: AppStyles.mediumSpacing


        RowLayout {

            Text {
                id: playlistText
                Layout.fillWidth: true
                text: "Playlist"
                color: AppStyles.textPrimary
                font: AppStyles.titleFont
            }

            ImageButton {
                id: clearButton
                Layout.alignment: Qt.AlignRight
                Layout.preferredWidth: AppStyles.mediumIcon
                Layout.preferredHeight: AppStyles.mediumIcon
                source: AppStyles.trashIcon
                onClicked: {
                    PlayerController.clearPlaylist()
                }
            }
        }

        ListView {
            id: listview
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: AppStyles.mediumSpacing
            clip: true
            //model: PlayerController.playlistModel()
            spacing: AppStyles.mediumSpacing

            delegate: AudioListItem {
                audioTitle: model.audioTitle
                audioAuthor: model.audioAuthorName
                audioImageSource: model.audioImageSource
                audioSource: model.audioSource
                audioIndex: model.index

                itemColor: AppStyles.backgroundColor
                showImage: true
                showActionButton: true
                actionButtonIcon: AppStyles.trashIcon

                onClicked: {
                    PlayerController.switchToAudioByIndex(audioIndex)
                }

                onActionClicked: {
                    PlayerController.removeAudio(audioIndex)
                }
            }
        }

        ImageButton {
            id: searchButton
            Layout.alignment: Qt.AlignLeft
            Layout.preferredWidth: AppStyles.mediumIcon
            Layout.preferredHeight: AppStyles.mediumIcon
            source: AppStyles.searchIcon
            onClicked: {
                root.playlistSearchRequested()
                root.hidden = true
            }
        }
    }

    Behavior on x {
        PropertyAnimation {
            easing.type: Easing.InOutQuad
            duration: AppStyles.mediumAnimation
        }
    }
}
