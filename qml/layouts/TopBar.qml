// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-17
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SongPlayer

Item {
    id: root
    
    property alias searchPanelHidden: root._searchPanelHidden
    property alias playlistPanelHidden: root._playlistPanelHidden
    property bool useNetworkSearch: false
    
    property bool _searchPanelHidden: true
    property bool _playlistPanelHidden: true
    
    signal playlistToggleRequested()
    signal showAddOptionsRequested()
    signal closeSearchRequested()
    signal searchResultsRequested()
    
    height: AppStyles.topBarHeight
    
    Rectangle {
        id: topbar
        anchors.fill: parent
        color: AppStyles.primaryColor

        RowLayout {
            anchors.fill: parent
            spacing: AppStyles.mediumSpacing

            Item {
                Layout.preferredWidth: AppStyles.mediumSpacing
                Layout.fillHeight: true
            }

            SearchField {
                Layout.fillWidth: true
                Layout.preferredHeight: AppStyles.controlBarHeight
                visible: !root._searchPanelHidden

                onAccepted: value => {
                                if (root.useNetworkSearch) {
                                    console.log("Network search triggered:", value)
                                    AudioSearchModel.searchSong(value)
                                } else {
                                    var playlistData = PlayerController.getPlaylistAudioInfoList()
                                    console.log("Local search triggered:", value, "playlist data count:", playlistData.length)
                                    PlaylistSearchModel.performSearch(playlistData, value)
                                }
                                root.searchResultsRequested()
                                topbar.forceActiveFocus()
                            }
            }

            Item {
                Layout.fillWidth: true
                visible: root._searchPanelHidden
            }

            ImageButton {
                id: playlistIcon
                Layout.preferredWidth: AppStyles.mediumIcon
                Layout.preferredHeight: AppStyles.mediumIcon
                source: AppStyles.menuIcon
                visible: root._searchPanelHidden
                onClicked: {
                    root.playlistToggleRequested()
                }
            }

            ImageButton {
                id: addButton
                Layout.preferredWidth: AppStyles.mediumIcon
                Layout.preferredHeight: AppStyles.mediumIcon
                source: AppStyles.addIcon
                visible: root._searchPanelHidden

                Shortcut {
                    sequence: StandardKey.Open
                    onActivated: root.showAddOptionsRequested()
                }

                onClicked: {
                    root.showAddOptionsRequested()
                }
            }

            ImageButton {
                id: closeSearchButton
                Layout.preferredWidth: AppStyles.mediumIcon
                Layout.preferredHeight: AppStyles.mediumIcon
                source: AppStyles.closeIcon
                visible: !root._searchPanelHidden
                onClicked: {
                    root.closeSearchRequested()
                }
            }
        }
    }
} 
