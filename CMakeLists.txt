cmake_minimum_required(VERSION 3.28)

if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()

project(MySongPlayer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# The project does not use C++ modules. Disabling dependency scanning also avoids
# compiler-specific dyndep failures for source trees whose paths contain Unicode.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(MYSONGPLAYER_WARNINGS_AS_ERRORS "Treat project warnings as build errors" OFF)
option(MYSONGPLAYER_BUILD_UI "Build the Qt UI and platform adapters" ON)

function(mysongplayer_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    if(MYSONGPLAYER_WARNINGS_AS_ERRORS)
        set_property(TARGET ${target} PROPERTY COMPILE_WARNING_AS_ERROR ON)
    endif()
endfunction()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(QT_REQUIRED_VERSION 6.11 CACHE STRING "Minimum Qt 6 version required by this project")

set(APP_NAME appMySongPlayer)
set(CORE_TARGET MySongPlayerCore)
set(METADATA_TARGET MySongPlayerMetadata)
set(APP_CORE_TARGET MySongPlayerAppCore)

set(CORE_HEADERS
    src/include/core/AudioImport.h
    src/include/core/AudioTrack.h
    src/include/core/Lyrics.h
    src/include/core/PlayMode.h
    src/include/core/Playlist.h
)

set(CORE_SOURCES
    src/core/AudioImport.cpp
    src/core/Lyrics.cpp
    src/core/Playlist.cpp
)

# This glob is intentionally used only by the architecture check, never as the
# target source list. New core files are therefore checked but must still be
# reviewed and added to the explicit build manifest.
file(GLOB_RECURSE CORE_ARCHITECTURE_FILES CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "src/core/*.cpp"
    "src/core/*.h"
    "src/include/core/*.h"
    "src/include/core/*.hpp"
)

# Architecture fitness check: the portable core must never include framework
# headers or reach back into an outer application layer.
foreach(core_file IN LISTS CORE_ARCHITECTURE_FILES)
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/${core_file}" framework_includes
        REGEX "^[ \t]*#[ \t]*include[ \t]*[<\"](Q[^>\"]*|Qt[^>\"]*|taglib/[^>\"]*|(adapters|controllers|coordinators|infrastructure|interfaces|models|services|storage)/[^>\"]*)[>\"]")
    if(framework_includes)
        message(FATAL_ERROR
            "Framework dependency found in portable core: ${core_file}: ${framework_includes}")
    endif()
endforeach()

set(APP_CORE_HEADERS
    src/include/adapters/QtAudioTrackAdapter.h
    src/include/controllers/AudioPlayer.h
    src/include/controllers/PlayerController.h
    src/include/coordinators/AudioCoordinator.h
    src/include/coordinators/PlaylistCoordinator.h
    src/include/interfaces/ICurrentSongManager.h
    src/include/interfaces/IPlaylistOperations.h
    src/include/interfaces/IPlaylistPersistence.h
    src/include/models/AudioInfo.h
    src/include/models/AudioSearchModel.h
    src/include/models/LyricsModel.h
    src/include/models/PlaylistModel.h
    src/include/models/PlaylistSearchModel.h
    src/include/services/AudioImporter.h
    src/include/services/LyricsService.h
    src/include/services/PlaylistStorageService.h
    src/include/storage/PlaylistDatabase.h
)

set(METADATA_HEADERS
    src/include/infrastructure/TagLibAudioMetadataReader.h
)

set(METADATA_SOURCES
    src/infrastructure/TagLibAudioMetadataReader.cpp
)

set(APP_CORE_SOURCES
    src/adapters/QtAudioTrackAdapter.cpp
    src/controllers/AudioPlayer.cpp
    src/controllers/PlayerController.cpp
    src/coordinators/AudioCoordinator.cpp
    src/coordinators/PlaylistCoordinator.cpp
    src/models/AudioInfo.cpp
    src/models/AudioSearchModel.cpp
    src/models/LyricsModel.cpp
    src/models/PlaylistModel.cpp
    src/models/PlaylistSearchModel.cpp
    src/services/AudioImporter.cpp
    src/services/LyricsService.cpp
    src/services/PlaylistStorageService.cpp
    src/storage/PlaylistDatabase.cpp
)

set(QML_TYPE_HEADERS
    src/include/controllers/PlayerController.h
    src/include/models/AudioInfo.h
    src/include/models/AudioSearchModel.h
    src/include/models/LyricsModel.h
    src/include/models/PlaylistModel.h
    src/include/models/PlaylistSearchModel.h
)

set_source_files_properties(qml/styles/AppStyles.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

add_library(${CORE_TARGET} STATIC
    ${CORE_HEADERS}
    ${CORE_SOURCES}
)

set_target_properties(${CORE_TARGET} PROPERTIES
    AUTOMOC OFF
    AUTORCC OFF
    AUTOUIC OFF
)

target_include_directories(${CORE_TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

target_compile_features(${CORE_TARGET} PUBLIC cxx_std_23)
mysongplayer_enable_warnings(${CORE_TARGET})

include(CTest)

if(BUILD_TESTING)
    add_executable(MySongPlayerCoreTests
        tests/core/PlaylistCoreTest.cpp
    )
    set_target_properties(MySongPlayerCoreTests PROPERTIES
        AUTOMOC OFF
        AUTORCC OFF
        AUTOUIC OFF
    )
    target_link_libraries(MySongPlayerCoreTests PRIVATE ${CORE_TARGET})
    target_compile_features(MySongPlayerCoreTests PRIVATE cxx_std_23)
    mysongplayer_enable_warnings(MySongPlayerCoreTests)
    add_test(NAME MySongPlayerCoreTests COMMAND MySongPlayerCoreTests)
    set_tests_properties(MySongPlayerCoreTests PROPERTIES TIMEOUT 30)
endif()

if(MYSONGPLAYER_BUILD_UI)
find_package(Qt6 ${QT_REQUIRED_VERSION} REQUIRED COMPONENTS
    Concurrent
    Core
    Gui
    Multimedia
    Network
    Qml
    Quick
    Sql
)
find_package(TagLib 2.0 REQUIRED)

qt_standard_project_setup(REQUIRES ${QT_REQUIRED_VERSION})

add_library(${METADATA_TARGET} STATIC
    ${METADATA_HEADERS}
    ${METADATA_SOURCES}
)

set_target_properties(${METADATA_TARGET} PROPERTIES
    AUTOMOC OFF
    AUTORCC OFF
    AUTOUIC OFF
)

target_include_directories(${METADATA_TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

target_link_libraries(${METADATA_TARGET}
    PUBLIC
        ${CORE_TARGET}
    PRIVATE
        TagLib::TagLib
)

target_compile_features(${METADATA_TARGET} PUBLIC cxx_std_23)
mysongplayer_enable_warnings(${METADATA_TARGET})

add_library(${APP_CORE_TARGET} STATIC
    ${APP_CORE_HEADERS}
    ${APP_CORE_SOURCES}
)

target_include_directories(${APP_CORE_TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/include/controllers
        ${CMAKE_CURRENT_SOURCE_DIR}/src/include/models
)

target_link_libraries(${APP_CORE_TARGET}
    PUBLIC
        ${CORE_TARGET}
        Qt6::Core
        Qt6::Multimedia
        Qt6::Network
        Qt6::Qml
        Qt6::Sql
    PRIVATE
        ${METADATA_TARGET}
        Qt6::Concurrent
)

target_compile_features(${APP_CORE_TARGET} PUBLIC cxx_std_23)
mysongplayer_enable_warnings(${APP_CORE_TARGET})

qt_add_qml_module(${APP_CORE_TARGET}
    URI MySongPlayer
    VERSION 1.0
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/MySongPlayer
    SOURCES
        ${QML_TYPE_HEADERS}
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

qt_add_executable(${APP_NAME}
    src/main.cpp
)

target_link_libraries(${APP_NAME}
    PRIVATE
        ${APP_CORE_TARGET}
        Qt6::Gui
        Qt6::Quick
)

target_compile_features(${APP_NAME} PRIVATE cxx_std_23)
mysongplayer_enable_warnings(${APP_NAME})

if(TARGET ${APP_CORE_TARGET}plugin)
    target_link_libraries(${APP_NAME}
        PRIVATE
            ${APP_CORE_TARGET}plugin
    )
endif()

qt_import_qml_plugins(${APP_NAME})

set_target_properties(${APP_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.myawesomeapp
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)

install(TARGETS ${APP_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET ${APP_NAME}
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
    EXCLUDE_PLUGIN_TYPES imageformats qmltooling sqldrivers
    EXCLUDE_PLUGINS KDEPlasmaPlatformTheme6 fcitx5platforminputcontextplugin layer-shell
    INCLUDE_PLUGINS qgif qico qjpeg qsqlite qsvg
)
install(SCRIPT ${deploy_script})

if(MSVC)
    set_property(TARGET ${APP_NAME} PROPERTY
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${APP_NAME}>"
    )
endif()

if(BUILD_TESTING)
    add_executable(MySongPlayerMetadataTests
        tests/integration/TagLibAudioMetadataReaderTest.cpp
    )
    set_target_properties(MySongPlayerMetadataTests PROPERTIES
        AUTOMOC OFF
        AUTORCC OFF
        AUTOUIC OFF
    )
    target_link_libraries(MySongPlayerMetadataTests PRIVATE ${METADATA_TARGET})
    target_compile_features(MySongPlayerMetadataTests PRIVATE cxx_std_23)
    mysongplayer_enable_warnings(MySongPlayerMetadataTests)
    add_test(NAME MySongPlayerMetadataTests COMMAND MySongPlayerMetadataTests)
    set_tests_properties(MySongPlayerMetadataTests PROPERTIES TIMEOUT 10)

    add_executable(MySongPlayerStorageTests
        tests/integration/PlaylistStorageTest.cpp
    )
    target_link_libraries(MySongPlayerStorageTests PRIVATE ${APP_CORE_TARGET})
    target_compile_features(MySongPlayerStorageTests PRIVATE cxx_std_23)
    mysongplayer_enable_warnings(MySongPlayerStorageTests)
    add_test(NAME MySongPlayerStorageTests COMMAND MySongPlayerStorageTests)
    set_tests_properties(MySongPlayerStorageTests PROPERTIES
        ENVIRONMENT "XDG_DATA_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-data"
        TIMEOUT 30
    )

    add_executable(MySongPlayerAudioImportTests
        tests/integration/AudioImporterAsyncTest.cpp
    )
    target_link_libraries(MySongPlayerAudioImportTests PRIVATE ${APP_CORE_TARGET})
    target_compile_features(MySongPlayerAudioImportTests PRIVATE cxx_std_23)
    mysongplayer_enable_warnings(MySongPlayerAudioImportTests)
    add_test(NAME MySongPlayerAudioImportTests COMMAND MySongPlayerAudioImportTests)
    set_tests_properties(MySongPlayerAudioImportTests PROPERTIES TIMEOUT 10)

    add_test(
        NAME MySongPlayerQmlLint
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all_qmllint
    )
    set_tests_properties(MySongPlayerQmlLint PROPERTIES TIMEOUT 60)
endif()
endif()
