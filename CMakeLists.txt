cmake_minimum_required(VERSION 4.0.0)

project(MySongPlayer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 COMPONENTS Quick Multimedia REQUIRED)


qt_add_executable(appMySongPlayer
    main.cpp
)

qt_add_qml_module(appMySongPlayer
    URI MySongPlayer
    VERSION 1.0
    QML_FILES
    qml/Main.qml
    qml/AudioInfoBox.qml
    qml/ImageButton.qml
    qml/PlaylistPanel.qml
    qml/SearchField.qml
    qml/SearchPanel.qml

    SOURCES
    AudioInfo.cpp AudioInfo.h
    AudioSearchModel.cpp AudioSearchModel.h
    PlayerController.h PlayerController.cpp

    RESOURCES
    assets/images/song_1.jpg
    assets/images/song_2.jpg
    assets/images/song_3.jpg

    assets/icons/trash_icon.png
    assets/icons/search_icon.png
    assets/icons/random_icon.png
    assets/icons/repeat_icon.png
    assets/icons/previous_icon.png
    assets/icons/play_icon.png
    assets/icons/pause_icon.png
    assets/icons/next_icon.png
    assets/icons/mute_icon.png
    assets/icons/menu_icon.png
    assets/icons/medium_icon.png
    assets/icons/low_icon.png
    assets/icons/list_cycle_icon.png
    assets/icons/import_icon.png
    assets/icons/high_icon.png
    assets/icons/close_icon.png
    assets/icons/app_icon.png
    assets/icons/app_icon.ico
    assets/icons/add_icon.png


    assets/videos/video_1.avi

    assets/audio/symphony_no_5.mp3
    assets/audio/air_on_the_g_string.mp3
    assets/audio/eine_kleine_nachtmusik.mp3
    assets/audio/reprezentuje_biede.mp3

    README.md
)

set_target_properties(appMySongPlayer PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appMySongPlayer
    PRIVATE Qt6::Quick Qt6::Multimedia)

install(TARGETS appMySongPlayer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
