// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-17
import QtQuick
import QtQuick.Layouts

import MySongPlayer

Rectangle {
    id: root

    property bool hidden: true
    property string searchMode: "local"  // "local" or "network"

    color: "#333333"

    ListView {
        id: listView

        anchors {
            fill: parent
            margins: 20
        }

        spacing: 10
        clip: true
        //model: root.searchMode === "network" ? AudioSearchModel : PlaylistSearchModel
        //visible: root.searchMode === "network" ? !AudioSearchModel.isSearching : !PlaylistSearchModel.isSearching

        delegate: Rectangle {
            id: delegate

            required property int index
            required property var model

            // Adapt to different model property names
            property string audioTitle: root.searchMode === "network" ? (model.audioName || "") : (model.audioTitle || "")
            property string audioAuthorName: root.searchMode === "network" ? (model.audioAuthor || "") : (model.audioAuthorName || "")
            property url audioImageSource: model.audioImageSource || ""
            property url audioSource: model.audioSource || ""
            property int originalIndex: root.searchMode === "local" ? (model.originalIndex || -1) : -1

            width: listView.width
            height: 50

            color: "#1e1e1e"

            Image {
                id: audioImage

                anchors {
                    left: parent.left
                    leftMargin: 5
                    verticalCenter: parent.verticalCenter
                }

                width: 32
                height: 32

                source: delegate.audioImageSource
            }

            ColumnLayout {
                anchors {
                    left: audioImage.right
                    right: parent.right
                    top: parent.top

                    margins: 5
                }

                spacing: 5

                Text {
                    width: parent.width

                    text: delegate.audioTitle
                    color: "white"

                    fontSizeMode: Text.Fit
                    minimumPixelSize: 12
                    elide: Text.ElideRight

                    font {
                        pixelSize: 16
                        bold: true
                    }
                }

                Text {
                    width: parent.width

                    text: delegate.audioAuthorName
                    color: "gray"

                    fontSizeMode: Text.Fit
                    minimumPixelSize: 8
                    elide: Text.ElideRight

                    font {
                        pixelSize: 12
                    }
                }
            }

            TapHandler {
                onTapped: {
                    root.hidden = true

                    if (root.searchMode === "network") {
                        // Network search: add to playlist
                        PlayerController.addNetworkAudio(delegate.audioTitle, delegate.audioAuthorName,
                            delegate.audioSource, delegate.audioImageSource)
                    } else {
                        // Local search: play song directly
                        PlayerController.switchToAudioByIndex(delegate.originalIndex)
                    }
                }
            }
        }
    }

    Text {
        anchors.centerIn: parent

        color: "gray"
        /*visible: (root.searchMode === "network" ? AudioSearchModel.isSearching : PlaylistSearchModel.isSearching) || listView.count === 0
        text: {
            if (root.searchMode === "network" ? AudioSearchModel.isSearching : PlaylistSearchModel.isSearching) {
                return root.searchMode === "network" ? "Searching online music..." : "Searching..."
            } else if (listView.count === 0) {
                return root.searchMode === "network" ? "No music found, try other keywords" : "No results"
            } else {
                return ""
            }
        }*/

        font {
            pixelSize: 24
            bold: true
        }
    }

    Behavior on y {
        PropertyAnimation {
            easing.type: Easing.InOutQuad
            duration: 200
        }
    }
}
