/*
 Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-25
 This file defines the global application theme, including colors, fonts, and layout constants.
 To be used as a singleton throughout the application for consistent styling.
*/
pragma Singleton
import QtQuick

QtObject {
    id: root

    // Color system - extracted from existing code for unified management
    readonly property color primaryColor: "#5F8575"
    readonly property color backgroundColor: "#1e1e1e"
    readonly property color surfaceColor: "#333333"
    readonly property color textPrimary: "white"
    readonly property color textSecondary: "gray"
    readonly property color transparentWhite: "#80FFFFFF"

    // Size system
    readonly property int smallIcon: 16
    readonly property int mediumIcon: 32
    readonly property int largeIcon: 50
    readonly property int playButton: 50
    readonly property int controlButton: 30
    readonly property int sliderHandle: 12
    readonly property int sliderTrack: 4

    // Spacing system
    readonly property int smallSpacing: 5
    readonly property int mediumSpacing: 10
    readonly property int largeSpacing: 20
    readonly property int panelMargin: 20

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

    readonly property string playIcon: "/qt/qml/MySongPlayer/assets/icons/play_icon.png"
    readonly property string pauseIcon: "/qt/qml/MySongPlayer/assets/icons/pause_icon.png"
    readonly property string previousIcon: "/qt/qml/MySongPlayer/assets/icons/previous_icon.png"
    readonly property string nextIcon: "/qt/qml/MySongPlayer/assets/icons/next_icon.png"

    readonly property string listCycleIcon: "/qt/qml/MySongPlayer/assets/icons/list_cycle_icon.png"
    readonly property string randomIcon: "/qt/qml/MySongPlayer/assets/icons/random_icon.png"
    readonly property string repeatOneIcon: "/qt/qml/MySongPlayer/assets/icons/repeat_icon.png"

    readonly property string volumeHighIcon: "/qt/qml/MySongPlayer/assets/icons/high_icon.png"
    readonly property string volumeLowIcon: "/qt/qml/MySongPlayer/assets/icons/low_icon.png"
    readonly property string volumeMuteIcon: "/qt/qml/MySongPlayer/assets/icons/mute_icon.png"
    readonly property string volumeMedium: "/qt/qml/MySongPlayer/assets/icons/medium_icon.png"

    readonly property string searchIcon: "/qt/qml/MySongPlayer/assets/icons/search_icon.png"
    readonly property string menuIcon: "/qt/qml/MySongPlayer/assets/icons/menu_icon.png"
    readonly property string addIcon: "/qt/qml/MySongPlayer/assets/icons/add_icon.png"
    readonly property string closeIcon: "/qt/qml/MySongPlayer/assets/icons/close_icon.png"
    readonly property string trashIcon: "/qt/qml/MySongPlayer/assets/icons/trash_icon.png"

    readonly property string appIcon: ":/qt/qml/MySongPlayer/assets/icons/app_icon.png"

    // Animation configuration
    readonly property int shortAnimation: 100
    readonly property int mediumAnimation: 200
    readonly property int longAnimation: 300

    // Layout configuration
    readonly property int topBarHeight: 50
    readonly property int bottomBarHeight: 80
    readonly property int playlistPanelWidth: 250
    readonly property int playlistPanelHeight: 400

    // Window dimensions
    readonly property int mainWindowWidth: 480
    readonly property int mainWindowHeight: 640

    // Component heights
    readonly property int listItemHeight: 50
    readonly property int controlBarHeight: 30
    readonly property int progressTimeWidth: 45
    readonly property int progressBarHeight: 20
    readonly property int audioInfoBoxSize: 150

    // Border and radius system
    readonly property int standardBorderWidth: 1
    readonly property int cardRadius: 8
    readonly property int buttonRadius: 6
    readonly property int progressRadius: 4

    // Additional colors
    readonly property color transparentColor: "transparent"
    readonly property color shadowColor: "#000000"
    readonly property color primaryColorLight: Qt.lighter(primaryColor)
    readonly property color hoverColor: Qt.lighter(primaryColor, 1.2)

    // Icon sizes
    readonly property int searchIconSize: 16
    readonly property int audioImageSize: 32
}

