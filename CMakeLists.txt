cmake_minimum_required(VERSION 3.8.2)
project(Sorting_Control_PC)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release") 

# Find SFML
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# Tell CMake to create the executable
add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE 
	src/main.cpp
	src/Chart.cpp
	src/Communication.cpp
	src/Window.cpp
	)
	
target_sources(${PROJECT_NAME} PRIVATE 
	include/Chart.hpp
	include/Communication.hpp
	include/Helpers.hpp
	include/Window.hpp
	)
	
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE sfml-graphics sfml-window sfml-system)