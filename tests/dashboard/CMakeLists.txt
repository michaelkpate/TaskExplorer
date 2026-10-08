cmake_minimum_required(VERSION 3.21)
project(DashboardCompileCheck LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent)
# Compile the production dashboard independently of the legacy Windows backend.
# This is an object-only check; it cannot establish live telemetry correctness.
add_library(DashboardCompileCheck OBJECT
    ../../TaskExplorer/GUI/SystemInfo/PerformanceDashboard.cpp)
target_include_directories(DashboardCompileCheck PRIVATE ../../TaskExplorer)
target_compile_definitions(DashboardCompileCheck PRIVATE TE_WITH_WIDGETS BUILD_STATIC)
target_link_libraries(DashboardCompileCheck PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Concurrent)
