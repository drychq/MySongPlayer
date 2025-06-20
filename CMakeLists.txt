cmake_minimum_required(VERSION 4.0.0)

project(MySongPlayer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 COMPONENTS Quick Multimedia REQUIRED)

qt_add_executable(appMySongPlayer
        src/main.cpp
)

qt_add_qml_module(appMySongPlayer
        URI MySongPlayer
        VERSION 1.0
        QML_FILES
        qml/Main.qml

        qml/layouts/TopBar.qml
        qml/layouts/MainArea.qml
        qml/layouts/BottomBar.qml

        qml/components/base/AudioListItem.qml
        qml/components/base/CustomSlider.qml
        qml/components/complex/AudioInfoBox.qml
        qml/components/complex/ImageButton.qml
        qml/components/complex/SearchField.qml
        qml/components/complex/VolumeSlider.qml
        qml/components/complex/LyricsDisplay.qml
        qml/components/complex/PlayModeButton.qml
        qml/components/complex/AudioProgressBar.qml
        qml/components/complex/AddSongOptionsPopup.qml

        qml/panels/SearchPanel.qml
        qml/panels/PlayListPanel.qml

        qml/styles/AppStyles.qml

        qml/utils/TimeUtils.js

        SOURCES
        src/controllers/PlayerController.cpp
        src/controllers/PlayerController.h
        src/models/AudioInfo.cpp
        src/models/AudioInfo.h
        src/models/AudioSearchModel.cpp
        src/models/AudioSearchModel.h

        RESOURCES
        qml/qmldir

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

        README.md
)

set_target_properties(appMySongPlayer PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
)

target_include_directories(appMySongPlayer PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/models
        ${CMAKE_CURRENT_SOURCE_DIR}/src/controllers
)

target_link_libraries(appMySongPlayer
        PRIVATE Qt6::Quick Qt6::Multimedia)

install(TARGETS appMySongPlayer
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
