cmake_minimum_required(VERSION 3.16)

project(pdfmergegs
    VERSION 1.0.0
    DESCRIPTION "Merge PDF documents with Ghostscript"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(GNUInstallDirs)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets Pdf LinguistTools)

if(COMMAND qt_standard_project_setup)
    qt_standard_project_setup()
endif()

set(PROJECT_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/PdfListWidget.cpp
    src/PdfListWidget.h
    src/PdfDocumentInfo.cpp
    src/PdfDocumentInfo.h
    src/GhostscriptRunner.cpp
    src/GhostscriptRunner.h
    src/SettingsDialog.cpp
    src/SettingsDialog.h
    src/AppSettings.cpp
    src/AppSettings.h
    src/LanguageManager.cpp
    src/LanguageManager.h
)

qt_add_executable(pdfmergegs ${PROJECT_SOURCES})

target_link_libraries(pdfmergegs PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Pdf
)

target_include_directories(pdfmergegs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(MSVC)
    target_compile_options(pdfmergegs PRIVATE /W4)
else()
    target_compile_options(pdfmergegs PRIVATE -Wall -Wextra)
endif()

# ---------------------------------------------------------------------------
# Translations: .ts -> .qm, embedded into the binary through Qt resources
# ---------------------------------------------------------------------------
set(TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/translations/pdfmergegs_hr.ts")
set(QM_FILE "${CMAKE_CURRENT_BINARY_DIR}/pdfmergegs_hr.qm")

# Qt 6.5+ compiles the .ts files and embeds them into the binary in one step.
# Older Qt releases need the two steps spelled out, and the .qm file must be
# produced before the resource compiler runs.
if(QT_VERSION VERSION_GREATER_EQUAL 6.5)
    qt_add_translations(pdfmergegs
        TS_FILES ${TS_FILES}
        RESOURCE_PREFIX "/i18n"
        LUPDATE_OPTIONS -no-obsolete
    )
else()
    add_custom_target(update_translations
        COMMAND Qt6::lupdate -no-obsolete -locations relative
                ${CMAKE_CURRENT_SOURCE_DIR}/src -ts ${TS_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Updating Croatian translation (lupdate)"
        VERBATIM
    )

    add_custom_command(
        OUTPUT ${QM_FILE}
        COMMAND Qt6::lrelease ${TS_FILES} -qm ${QM_FILE}
        DEPENDS ${TS_FILES}
        COMMENT "Compiling Croatian translation (lrelease)"
        VERBATIM
    )
    add_custom_target(pdfmergegs_qm DEPENDS ${QM_FILE})
    add_dependencies(pdfmergegs pdfmergegs_qm)

    qt_add_resources(pdfmergegs "translations"
        PREFIX "/i18n"
        BASE "${CMAKE_CURRENT_BINARY_DIR}"
        FILES "${QM_FILE}"
    )
endif()

# ---------------------------------------------------------------------------
# Application icon and Qt resource bundle
# ---------------------------------------------------------------------------
qt_add_resources(pdfmergegs "app_resources"
    PREFIX "/icons"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/deploy"
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/deploy/pdfmergegs.svg"
)

# ---------------------------------------------------------------------------
# .desktop file and icon: generated into the build directory next to the binary
# ---------------------------------------------------------------------------
set(DESKTOP_IN "${CMAKE_CURRENT_SOURCE_DIR}/deploy/pdfmergegs.desktop.in")
set(DESKTOP_OUT "${CMAKE_CURRENT_BINARY_DIR}/pdfmergegs.desktop")

configure_file(${DESKTOP_IN} ${DESKTOP_OUT} @ONLY)

add_custom_command(TARGET pdfmergegs POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/deploy/pdfmergegs.svg"
            "${CMAKE_CURRENT_BINARY_DIR}/pdfmergegs.svg"
    COMMENT "Copying application icon next to the binary"
)

# ---------------------------------------------------------------------------
# Installation
# ---------------------------------------------------------------------------
install(TARGETS pdfmergegs
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${DESKTOP_OUT}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/deploy/pdfmergegs.svg"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)

message(STATUS "pdfmergegs ${PROJECT_VERSION} with Qt ${Qt6_VERSION}")
