cmake_minimum_required(VERSION 3.16)
project(Otso VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use static runtime for MSVC
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(Otso_WERROR "Treat warnings as errors" OFF)
option(Otso_BUILD_TESTS "Build lightweight logic tests" ON)

if(MSVC)
    add_compile_options(/W4 /WX /permissive- /utf-8)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup")
else()
    add_compile_options(-Wall -Wextra -municode -Wno-error=missing-field-initializers)
    if(Otso_WERROR)
        add_compile_options(-Werror)
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++ -mwindows")
    else()
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
    endif()
endif()

add_executable(Otso WIN32
    src/main.cpp
    src/core/crash_diagnostics.cpp
    src/core/file_dialog_filters.cpp
    src/core/globals.cpp
    src/core/list_continuation.cpp
    src/core/session_io.cpp
    src/core/text_codec.cpp
    src/core/versioning.cpp
    src/lang/lang.cpp
    src/modules/theme.cpp
    src/modules/editor.cpp
    src/modules/file.cpp
    src/modules/ui.cpp
    src/modules/background.cpp
    src/modules/command_routing.cpp
    src/modules/dialog.cpp
    src/modules/commands.cpp
    src/modules/icon_actions.cpp
    src/modules/perf_benchmark.cpp
    src/modules/tab_layout.cpp
    src/modules/tab_model_ops.cpp
    src/modules/tab_session_io.cpp
    src/modules/tab_spin_chrome.cpp
    src/modules/update_check.cpp
    src/modules/menu.cpp
    src/modules/graphics_engine.cpp
    src/modules/premium_header.cpp
    src/modules/premium_orchestrator.cpp
    src/modules/custom_scrollbar.cpp
    src/modules/selection_aura.cpp
    src/modules/command_palette.cpp
    src/modules/settings.cpp
    src/notepad.rc
)

target_include_directories(Otso PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/modules)
target_compile_definitions(Otso PRIVATE UNICODE _UNICODE NOMINMAX Otso_SPLIT_COMMAND_MODULES=1)

target_link_libraries(Otso PRIVATE
    comctl32
    shlwapi
    comdlg32
    shell32
    gdiplus
    msimg32
    user32
    gdi32
    kernel32
    dwmapi
    uxtheme
    winhttp
    d2d1
    dwrite
)

set(Otso_FONT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets/fonts")
set(Otso_BUNDLED_FONT_FILES
    "GeneralSans-Regular.otf"
    "GeneralSans-Medium.otf"
    "GeneralSans-Semibold.otf"
    "GeneralSans-Bold.otf"
    "GeneralSans-Italic.otf"
    "GeneralSans-MediumItalic.otf"
    "GeneralSans-SemiboldItalic.otf"
    "GeneralSans-BoldItalic.otf"
    "GeneralSans-Light.otf"
    "GeneralSans-LightItalic.otf"
)
set(Otso_BUNDLED_FONT_PATHS "")
foreach(font_file IN LISTS Otso_BUNDLED_FONT_FILES)
    set(font_path "${Otso_FONT_SOURCE_DIR}/${font_file}")
    if(EXISTS "${font_path}")
        list(APPEND Otso_BUNDLED_FONT_PATHS "${font_path}")
    endif()
endforeach()
if(Otso_BUNDLED_FONT_PATHS)
    add_custom_command(TARGET Otso POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Otso>/fonts"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${Otso_BUNDLED_FONT_PATHS} "$<TARGET_FILE_DIR:Otso>/fonts"
    )
endif()

if(MSVC)
    # Manifest already embedded via src/notepad.rc.
    # Disable linker auto-manifest to avoid duplicate MANIFEST resource (type 1).
    target_link_options(Otso PRIVATE /MANIFEST:NO)
endif()

if(NOT MSVC)
    target_link_options(Otso PRIVATE -mwindows -municode)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_options(Otso PRIVATE -static)
        target_link_libraries(Otso PRIVATE -static-libgcc -static-libstdc++)
    endif()
    if(CMAKE_STRIP)
        add_custom_command(TARGET Otso POST_BUILD COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:Otso>)
    endif()
endif()

if(Otso_BUILD_TESTS)
    enable_testing()
    add_executable(Otso-versioning-tests
        tests/versioning_tests.cpp
        src/core/versioning.cpp
    )
    target_include_directories(Otso-versioning-tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_compile_definitions(Otso-versioning-tests PRIVATE UNICODE _UNICODE NOMINMAX)
    add_test(NAME Otso-versioning-tests COMMAND Otso-versioning-tests)

    add_executable(Otso-encoding-tests
        tests/encoding_tests.cpp
        src/core/text_codec.cpp
    )
    target_include_directories(Otso-encoding-tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_compile_definitions(Otso-encoding-tests PRIVATE UNICODE _UNICODE NOMINMAX)
    add_test(NAME Otso-encoding-tests COMMAND Otso-encoding-tests)

    add_executable(Otso-session-io-tests
        tests/session_io_tests.cpp
        src/core/session_io.cpp
    )
    target_include_directories(Otso-session-io-tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_compile_definitions(Otso-session-io-tests PRIVATE UNICODE _UNICODE NOMINMAX)
    add_test(NAME Otso-session-io-tests COMMAND Otso-session-io-tests)

    add_executable(Otso-tab-session-io-tests
        tests/tab_session_io_tests.cpp
        src/core/session_io.cpp
        src/core/text_codec.cpp
        src/modules/tab_session_io.cpp
    )
    target_include_directories(Otso-tab-session-io-tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/modules)
    target_compile_definitions(Otso-tab-session-io-tests PRIVATE UNICODE _UNICODE NOMINMAX)
    target_link_libraries(Otso-tab-session-io-tests PRIVATE shlwapi)
    add_test(NAME Otso-tab-session-io-tests COMMAND Otso-tab-session-io-tests)

    add_executable(Otso-tab-model-ops-tests
        tests/tab_model_ops_tests.cpp
        src/modules/tab_model_ops.cpp
    )
    target_include_directories(Otso-tab-model-ops-tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/modules)
    target_compile_definitions(Otso-tab-model-ops-tests PRIVATE UNICODE _UNICODE NOMINMAX)
    add_test(NAME Otso-tab-model-ops-tests COMMAND Otso-tab-model-ops-tests)

    add_executable(Otso-list-continuation-tests
        tests/list_continuation_tests.cpp
        src/core/list_continuation.cpp
    )
    target_include_directories(Otso-list-continuation-tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_compile_definitions(Otso-list-continuation-tests PRIVATE UNICODE _UNICODE NOMINMAX)
    add_test(NAME Otso-list-continuation-tests COMMAND Otso-list-continuation-tests)
endif()

# Installation
install(TARGETS Otso DESTINATION .)
install(FILES LICENSE README.md DESTINATION .)
if(Otso_BUNDLED_FONT_PATHS)
    install(FILES ${Otso_BUNDLED_FONT_PATHS} DESTINATION fonts)
endif()

# Installer / packaging (CPack with NSIS)
set(CPACK_GENERATOR "NSIS")
set(CPACK_PACKAGE_NAME "Otso")
set(CPACK_PACKAGE_VENDOR "Technical Standard")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
if(CMAKE_GENERATOR_PLATFORM)
    set(CPACK_PACKAGE_FILE_NAME "Otso-${PROJECT_VERSION}-${CMAKE_GENERATOR_PLATFORM}")
else()
    set(CPACK_PACKAGE_FILE_NAME "Otso-${PROJECT_VERSION}")
endif()
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_NSIS_DISPLAY_NAME "Otso")
set(CPACK_NSIS_PACKAGE_NAME "Otso")
set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/wisesakarta/solum")
set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/src/icon.ico")
set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/src/icon.ico")
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
set(CPACK_PACKAGE_EXECUTABLES "Otso.exe" "Otso")
set(CPACK_NSIS_INSTALLED_ICON_NAME "Otso.exe")
set(CPACK_NSIS_MUI_FINISHPAGE_RUN "Otso.exe")
set(CPACK_NSIS_MENU_LINKS
    "Otso.exe" "Otso"
)

include(CPack)
