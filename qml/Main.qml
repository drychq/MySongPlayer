// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-18
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtCore
import MySongPlayer
import "layouts"
import "panels"
import "components/complex"

ApplicationWindow {
    id: root
    width: 480
    height: 640
    visible: true
    title: "My Song Player"

    Item {
        id: topContainer
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: AppStyles.topBarHeight
        z: 1

        TopBar {
            id: topBar
            anchors.fill: parent

            onPlaylistToggleRequested: {
                playListPanel.hidden = !playListPanel.hidden
            }

            onShowAddOptionsRequested: {
                addSongOptionsPopup.open()
            }

            onCloseSearchRequested: {
                searchPanel.hidden = true
                topBar.searchPanelHidden = true
                // Clear search results and reset search mode
                PlaylistSearchModel.clearSearch()
                // Reset to local search mode
                topBar.useNetworkSearch = false
                searchPanel.searchMode = "local"
            }

            onSearchResultsRequested: {
                searchPanel.hidden = false
                topBar.searchPanelHidden = false
            }
        }

        PlayListPanel {
            id: playListPanel
            anchors {
                top: topBar.bottom
            }
            x: hidden ? parent.width : parent.width - width
            onPlaylistSearchRequested: {
                topBar.searchPanelHidden = false
                searchPanel.hidden = false
            }
        }
    }

    ColumnLayout {
        id: mainLayout
        anchors.top: topContainer.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        spacing: 0

        MainArea {
            id: mainArea
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        BottomBar {
            id: bottomBar
            Layout.fillWidth: true
            Layout.preferredHeight: AppStyles.bottomBarHeight
        }
    }

    SearchPanel {
        id: searchPanel
        anchors {
            left: parent.left
            right: parent.right
        }
        height: mainArea.height + bottomBar.height
        y: hidden ? parent.height : topContainer.height

        onHiddenChanged: {
            topBar.searchPanelHidden = hidden
        }
    }

    AddSongOptionsPopup {
        id: addSongOptionsPopup

        onLocalImportRequested: {
            close()
            audioFileDialog.open()
        }

        onNetworkImportRequested: {
            close()
            topBar.useNetworkSearch = true
            searchPanel.searchMode = "network"
            searchPanel.hidden = false
            topBar.searchPanelHidden = false
            playListPanel.hidden = true
        }
    }

    FileDialog {
        id: audioFileDialog
        title: "Select Audio Files"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Audio Files (*.mp3 *.wav *.ogg *.flac *.m4a)"]
        currentFolder: StandardPaths.standardLocations(StandardPaths.MusicLocation)[0]
        onAccepted: {
            PlayerController.importLocalAudio(audioFileDialog.selectedFiles)
        }
    }
}
