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
    set(QT_LIBS Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Sql)
    message(STATUS "Using Qt6")
else()
    find_package(Qt5 REQUIRED COMPONENTS Widgets Core Gui Sql)
    set(QT_LIBS Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Sql)
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
target_link_libraries(steganolib PUBLIC Qt6::Core Qt6::Sql)

# ── Application ──
add_executable(StegoLab
    src/main.cpp
    src/mainwindow.cpp
    include/mainwindow.h
)
target_link_libraries(StegoLab PRIVATE steganolib Qt6::Widgets Qt6::Gui)

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
find_package(Qt6 QUIET COMPONENTS Test)
if(Qt6Test_FOUND)
    enable_testing()
    add_executable(StegoLab_tests
        tests/test_steganography.cpp
    )
    target_link_libraries(StegoLab_tests PRIVATE
        steganolib Qt6::Test Qt6::Core Qt6::Gui
    )
    add_test(NAME StegoLabTests COMMAND StegoLab_tests)
    message(STATUS "Tests enabled (Qt6Test found)")
else()
    message(WARNING "Qt6Test not found — tests disabled")
endif()
