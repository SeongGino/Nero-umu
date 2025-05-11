cmake_minimum_required(VERSION 3.16)

project(Nero-UMU VERSION 0.9.10 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(NERO_VERSION "Sets Nero version number" OFF)
option(NERO_GITHASH "Sets Nero git hash" OFF)
# for statically linking QuaZip specifically
set(BUILD_SHARED_LIBS OFF)

if(NERO_VERSION)
    add_compile_definitions(NERO_VERSION="${NERO_VERSION}")
endif()

if(NERO_GITHASH)
    add_compile_definitions(NERO_GITHASH="${NERO_GITHASH}")
endif()

if(NOT NERO_QT_VERSION)
    find_package(QT NAMES Qt6 Qt5 REQUIRED)
else()
    find_package(QT NAMES ${NERO_QT_VERSION} REQUIRED)
endif()

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

set(TS_FILES translations/Nero-Launcher_en_US.ts)

set(PROJECT_SOURCES
        src/main.cpp
        src/neromanager.cpp
        src/neromanager.h
        src/neromanager.ui
        src/nerorunner.cpp
        src/nerorunner.h
        src/nerofs.cpp
        src/nerofs.h
        src/nerotricks.cpp
        src/nerotricks.h
        src/nerotricks.ui
        src/nerowizard.cpp
        src/nerowizard.h
        src/nerowizard.ui
        src/neroshortcut.cpp
        src/neroshortcut.h
        src/neroshortcut.ui
        src/neroico.cpp
        src/neroico.h
        src/neropreferences.h
        src/neropreferences.cpp
        src/neropreferences.ui
        src/neroprefixsettings.h
        src/neroprefixsettings.cpp
        src/neroprefixsettings.ui
        src/neroconstants.h
        src/neroconstants.cpp
        src/nerodrives.h
        src/nerodrives.cpp
        src/nerodrives.ui
        src/nerorunnerdialog.h
        src/nerorunnerdialog.cpp
        src/nerorunnerdialog.ui
        src/neroonetimedialog.h
        src/neroonetimedialog.cpp
        src/neroonetimedialog.ui
        ${TS_FILES}
        img/pics.qrc
)

include(FindPkgConfig)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core5Compat)
    qt_add_executable(nero-umu
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    add_executable(nero-umu
        ${PROJECT_SOURCES}
    )
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

add_subdirectory(lib/quazip)
target_link_libraries(nero-umu PRIVATE Qt${QT_VERSION_MAJOR}::Widgets QuaZip::QuaZip)

include(GNUInstallDirs)
install(TARGETS nero-umu
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(nero-umu)
endif()
