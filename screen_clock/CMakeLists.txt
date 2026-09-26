cmake_minimum_required(VERSION 3.20)
project(FedoraClock VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 auto tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Sources
set(SOURCES
    src/main.cpp
    src/ClockWindow.cpp
)

set(HEADERS
    src/ClockWindow.h
)

add_executable(fedora_clock ${SOURCES} ${HEADERS})

target_link_libraries(fedora_clock PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Compiler flags for performance and warnings
if(MSVC)
    target_compile_options(fedora_clock PRIVATE
        /O2
        /W4
    )
else()
    target_compile_options(fedora_clock PRIVATE
        -O2
        -Wall
        -Wextra
    )
endif()

# Install target
install(TARGETS fedora_clock DESTINATION bin)

message(STATUS "Qt6 version: ${Qt6_VERSION}")
message(STATUS "Build type:  ${CMAKE_BUILD_TYPE}")

