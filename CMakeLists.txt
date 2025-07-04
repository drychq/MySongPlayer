cmake_minimum_required(VERSION 3.28)

project(MySongPlayer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Multimedia Sql)

qt_standard_project_setup(REQUIRES 6.8)

find_package(TagLib REQUIRED)

file(GLOB_RECURSE APP_SOURCES "src/*.h" "src/*.cpp")

set(APP_NAME appMySongPlayer)
qt_add_executable(${APP_NAME}
    src/main.cpp
    ${APP_SOURCES}
)

target_include_directories(${APP_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/controllers
        ${CMAKE_CURRENT_SOURCE_DIR}/src/models
        ${CMAKE_CURRENT_SOURCE_DIR}/src/coordinators
        ${CMAKE_CURRENT_SOURCE_DIR}/src/services
)

qt_add_qml_module(${APP_NAME}
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
)

set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/app_icon.icns")
set(APP_ICON_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/app_icon.ico")

set_target_properties(${APP_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.myawesomeapp
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/app_icon.png"
    RESOURCE "${APP_ICON_WINDOWS}"
)

target_link_libraries(${APP_NAME} PRIVATE
    Qt6::Quick
    Qt6::Multimedia
    Qt6::Sql
    TagLib::TagLib
)

include(GNUInstallDirs)

if(WIN32)
    set(INSTALL_DESTDIR ".")
elseif(APPLE)
    set(INSTALL_DESTDIR ".")
else()
    set(INSTALL_DESTDIR ".")
endif()

install(TARGETS ${APP_NAME}
    BUNDLE DESTINATION . COMPONENT Devel
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Devel
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Devel
)

if(MSVC)
  set_property(TARGET ${APP_NAME} PROPERTY
               VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${APP_NAME}>")
endif()
