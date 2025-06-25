cmake_minimum_required(VERSION 3.28)

project(MySongPlayer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APP_NAME ${PROJECT_NAME})

find_package(Qt6 REQUIRED COMPONENTS Quick Multimedia Sql)

qt_standard_project_setup(REQUIRES 6.8)

find_package(PkgConfig REQUIRED)

pkg_check_modules(TAGLIB REQUIRED taglib)

add_library(TAGLIB::taglib INTERFACE IMPORTED)
target_include_directories(TAGLIB::taglib INTERFACE ${TAGLIB_INCLUDE_DIRS})
target_link_libraries(TAGLIB::taglib INTERFACE ${TAGLIB_LIBRARIES})

qt_add_executable(appMySongPlayer
    src/main.cpp
)

target_include_directories(appMySongPlayer PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/controllers
        ${CMAKE_CURRENT_SOURCE_DIR}/src/models
        ${CMAKE_CURRENT_SOURCE_DIR}/src/coordinators
        ${CMAKE_CURRENT_SOURCE_DIR}/src/services
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
        qml/components/complex/AudioProgressBar.qml
        qml/components/complex/VolumeSlider.qml
        qml/components/complex/PlayModeButton.qml
        qml/components/complex/AddSongOptionsPopup.qml
        qml/components/complex/LyricsDisplay.qml

        qml/panels/PlayListPanel.qml
        qml/panels/SearchPanel.qml

        qml/styles/AppStyles.qml

        qml/utils/TimeUtils.js

    RESOURCES
        qml/qmldir

        assets/icons/app_icon.ico
        assets/icons/app_icon.png
        assets/icons/add_icon.png
        assets/icons/menu_icon.png
        assets/icons/trash_icon.png
        assets/icons/next_icon.png
        assets/icons/previous_icon.png
        assets/icons/play_icon.png
        assets/icons/pause_icon.png
        assets/icons/close_icon.png
        assets/icons/search_icon.png
        assets/icons/import_icon.png

        assets/icons/repeat_icon.png
        assets/icons/random_icon.png
        assets/icons/list_cycle_icon.png

        assets/icons/high_icon.png
        assets/icons/low_icon.png
        assets/icons/mute_icon.png
        assets/icons/medium_icon.png

        README.md
)

target_sources(appMySongPlayer PRIVATE
    src/controllers/PlayerController.cpp src/controllers/PlayerController.h
    src/controllers/AudioPlayer.cpp src/controllers/AudioPlayer.h

    src/models/AudioInfo.cpp src/models/AudioInfo.h
    src/models/AudioSearchModel.cpp src/models/AudioSearchModel.h
    src/models/PlaylistModel.h src/models/PlaylistModel.cpp
    src/models/PlaylistSearchModel.h src/models/PlaylistSearchModel.cpp
    src/models/PlaylistSearchModel.h src/models/PlaylistSearchModel.cpp
    src/models/LyricsModel.h src/models/LyricsModel.cpp
    src/models/PlaylistSearchModel.h src/models/PlaylistSearchModel.cpp

    src/services/AudioImporter.h src/services/AudioImporter.cpp
    src/services/PlaylistStorageService.h src/services/PlaylistStorageService.cpp
    src/services/LyricsService.h src/services/LyricsService.cpp
    src/services/LyricsService.h src/services/LyricsService.cpp

    src/interfaces/ICurrentSongManager.h
    src/interfaces/IPlaylistOperations.h
    src/interfaces/IPlaylistPersistence.h

    src/coordinators/PlaylistCoordinator.h src/coordinators/PlaylistCoordinator.cpp

    src/storage/PlaylistDatabase.h src/storage/PlaylistDatabase.cpp
)


set_target_properties(appMySongPlayer PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.myawesomeapp
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_ICON_FILE "app_icon.icns"
    WIN32_EXECUTABLE TRUE
    RESOURCE assets/icons/app_icon.ico
)

target_link_libraries(appMySongPlayer PRIVATE
    Qt6::Quick
    Qt6::Multimedia
    Qt6::Sql
    TAGLIB::taglib
)


include(GNUInstallDirs)


if(WIN32)
    set(INSTALL_DESTDIR ".")
elseif(APPLE)
    set(INSTALL_DESTDIR ".")
else()
    set(INSTALL_DESTDIR ".")
endif()


install(TARGETS appMySongPlayer
    BUNDLE DESTINATION ${INSTALL_DESTDIR}
    RUNTIME DESTINATION ${INSTALL_DESTDIR}/${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${INSTALL_DESTDIR}/${CMAKE_INSTALL_LIBDIR}
)


if(WIN32)
  set_property(TARGET ${APP_NAME} APPEND PROPERTY 
               VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${APP_NAME}>)
endif()


if(APPLE)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/app_icon.png"
            DESTINATION "${CMAKE_INSTALL_PREFIX}/Resources"
            RENAME "app_icon.icns")
endif()
