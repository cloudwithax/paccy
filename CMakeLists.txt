cmake_minimum_required(VERSION 3.16)
project(paccy VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
add_executable(paccy
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
)

target_link_libraries(paccy PRIVATE Qt6::Widgets)

install(TARGETS paccy DESTINATION bin)
install(FILES paccy.desktop DESTINATION share/applications)
install(FILES paccy.xml DESTINATION share/mime/packages)
