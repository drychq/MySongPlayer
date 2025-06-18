/*
 Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-17
 This file defines the global application theme, including colors, fonts, and layout constants.
 To be used as a singleton throughout the application for consistent styling.
*/
pragma Singleton
import QtQuick
import MySongPlayer

QtObject {
    id: root

    // Color
    readonly property color primaryColor: "#5F8575"
    readonly property color backgroundColor: "#1e1e1e"
    readonly property color surfaceColor: "#333333"
    readonly property color textPrimary: "white"
    readonly property color textSecondary: "gray"
    readonly property color transparentWhite: "#80FFFFFF"

    // Icon size
    readonly property int smallIcon: 16
    readonly property int mediumIcon: 32
    readonly property int largeIcon: 50
    readonly property int playButton: 50
    readonly property int controlButton: 30
    readonly property int sliderHandle: 12
    readonly property int sliderTrack: 4

    // Spacing and margin
    readonly property int smallSpacing: 5
    readonly property int mediumSpacing: 10
    readonly property int largeSpacing: 20
    readonly property int panelMargin: 20

    // Font size
    readonly property font titleFont: Qt.font({
        pixelSize: 20,
        bold: true
    })

    readonly property font subtitleFont: Qt.font({
        pixelSize: 16,
        bold: true
    })

    readonly property font bodyFont: Qt.font({
        pixelSize: 14
    })

    readonly property font smallFont: Qt.font({
        pixelSize: 12
    })

    readonly property font tinyFont: Qt.font({
        pixelSize: 10
    })

    readonly property string playIcon: "/MySongPlayer/assets/icons/play_icon.png"
    readonly property string pauseIcon: "/MySongPlayer/assets/icons/pause_icon.png"
    readonly property string previousIcon: "/MySongPlayer/assets/icons/previous_icon.png"
    readonly property string nextIcon: "/MySongPlayer/assets/icons/next_icon.png"

    readonly property string listCycleIcon: "/MySongPlayer/assets/icons/list_cycle_icon.png"
    readonly property string randomIcon: "/MySongPlayer/assets/icons/random_icon.png"
    readonly property string repeatOneIcon: "/MySongPlayer/assets/icons/repeat_icon.png"

    readonly property string volumeHighIcon: "/MySongPlayer/assets/icons/high_icon.png"
    readonly property string volumeLowIcon: "/MySongPlayer/assets/icons/low_icon.png"
    readonly property string volumeMuteIcon: "/MySongPlayer/assets/icons/mute_icon.png"
    readonly property string volumeMedium: "/MySongPlayer/assets/icons/medium_icon.png"

    readonly property string searchIcon: "/MySongPlayer/assets/icons/search_icon.png"
    readonly property string menuIcon: "/MySongPlayer/assets/icons/menu_icon.png"
    readonly property string addIcon: "/MySongPlayer/assets/icons/add_icon.png"
    readonly property string closeIcon: "/MySongPlayer/assets/icons/close_icon.png"
    readonly property string trashIcon: "/MySongPlayer/assets/icons/trash_icon.png"

    // Animation
    readonly property int shortAnimation: 100
    readonly property int mediumAnimation: 200
    readonly property int longAnimation: 300

    // height and width
    readonly property int topBarHeight: 50
    readonly property int bottomBarHeight: 80
    readonly property int playlistPanelWidth: 250
    readonly property int playlistPanelHeight: 400
}
