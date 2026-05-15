cmake_minimum_required(VERSION 3.16)
project(StegoLab VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── Find Qt (Qt6 first, fallback Qt5) ──
find_package(Qt6 QUIET COMPONENTS Widgets Core Gui Sql)
if(Qt6_FOUND)
    set(QT_PREFIX Qt6)
    message(STATUS "Using Qt6")
else()
    find_package(Qt5 REQUIRED COMPONENTS Widgets Core Gui Sql)
    set(QT_PREFIX Qt5)
    message(STATUS "Using Qt5")
endif()

# ── Core library (shared between app and tests) ──
add_library(steganolib STATIC
    src/steganography.cpp
    src/database.cpp
    include/steganography.h
    include/database.h
)
target_include_directories(steganolib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(steganolib PUBLIC ${QT_PREFIX}::Core ${QT_PREFIX}::Sql)

# ── Application ──
add_executable(StegoLab
    src/main.cpp
    src/mainwindow.cpp
    include/mainwindow.h
)
target_link_libraries(StegoLab PRIVATE steganolib ${QT_PREFIX}::Widgets ${QT_PREFIX}::Gui)

if(MINGW)
    target_link_options(StegoLab PRIVATE -mwindows)
    target_link_libraries(StegoLab PRIVATE
        ws2_32 winmm imm32 ole32 oleaut32 uuid gdi32 comdlg32 winspool
    )
elseif(WIN32)
    set_target_properties(StegoLab PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

install(TARGETS StegoLab DESTINATION bin)

# ── Tests ──
find_package(${QT_PREFIX} QUIET COMPONENTS Test)
if(${QT_PREFIX}Test_FOUND)
    enable_testing()
    add_executable(StegoLab_tests
        tests/test_steganography.cpp
    )
    target_link_libraries(StegoLab_tests PRIVATE
        steganolib ${QT_PREFIX}::Test ${QT_PREFIX}::Core ${QT_PREFIX}::Gui
    )
    add_test(NAME StegoLabTests COMMAND StegoLab_tests)
    message(STATUS "Tests enabled (${QT_PREFIX}Test found)")
else()
    message(WARNING "${QT_PREFIX}Test not found — tests disabled")
endif()
