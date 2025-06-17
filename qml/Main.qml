import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts

import MySongPlayer

ApplicationWindow {
    id: root

    width: 480
    height: 640
    visible: true
    title: "My Song Player"

    ColumnLayout {
        anchors.fill: parent

        Rectangle {
            id: topbar

            Layout.preferredHeight: 50
            Layout.fillWidth: true
            color: "#5F8575"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10

                SearchField {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    Layout.alignment: Qt.AlignVCenter

                    visible: !searchPanel.hidden
                    enabled: !AudioSearchModel.isSearching

                    onAccepted: value => {
                        AudioSearchModel.searchSong(value)
                        topbar.forceActiveFocus()
                    }
                }

                RowLayout {
                    Layout.alignment: Qt.AlignRight

                    ImageButton {
                        id: playlistIcon
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 32
                        source: "../assets/icons/menu_icon.png"

                        visible: searchPanel.hidden

                        onClicked: {
                            playlistPanel.hidden = !playlistPanel.hidden
                        }
                    }

                    ImageButton {
                        id: addAudioButton
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 32
                        source: "../assets/icons/add_icon.png"
                    }
                }
                ImageButton {
                    id: closeSearchButton

                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32

                    source: "../assets/icons/close_icon.png"
                    visible: !searchPanel.hidden

                    onClicked: {
                        searchPanel.hidden = true
                    }
                }
            }
        }

        Rectangle {
            id: mainSection
            Layout.fillHeight: true
            Layout.fillWidth: true

            color: "#1e1e1e"

            AudioInfoBox {
                id: firstSong
                anchors.centerIn: parent
                width: parent.width - 40
            }
        }

        Rectangle {
            id: bottomBar

            Layout.fillWidth: true
            Layout.preferredHeight: 100
            color: "#333333"

            RowLayout {
                spacing: 20

                enabled: !!PlayerController.currentSong
                opacity: enabled ? 1 : 0.3

                ImageButton {
                    id: playModeButton

                    Layout.alignment: Qt.AlignLeft
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30

                    source: "../assets/icons/list_cycle_icon.png"
                }

                RowLayout {


                    ImageButton {
                        id: previousButton

                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 30

                        source: "../assets/icons/previous_icon.png"

                        onClicked: PlayerController.switchToPreviousSong()
                    }

                    ImageButton {
                        id: playPauseButton

                        Layout.preferredWidth: 50
                        Layout.preferredHeight: 50

                        source: PlayerController.playing ? "../assets/icons/pause_icon.png" : "../assets/icons/play_icon.png"
                        onClicked: PlayerController.playPause()
                    }

                    ImageButton {
                        id: nextButton

                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 30

                        source: "../assets/icons/next_icon.png"

                        onClicked: PlayerController.switchToNextSong()
                    }
                }

                ImageButton {
                    id: volumeButton

                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30
                    source: "../assets/icons/medium_icon.png"
                }
            }
        }
    }

    PlaylistPanel {
        id: playlistPanel
        x: hidden ? parent.width : parent.width - width
        onSearchRequested: {
            searchPanel.hidden = false
        }
    }

    SearchPanel {
        id: searchPanel
        anchors.left: parent.left
        anchors.right: parent.right
        height: mainSection.height + bottomBar.height
        y: hidden ? parent.height : topbar.height
    }
}
