cmake_minimum_required(VERSION 3.11.3)
project(Sorting_Control_PC)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo") 

# Find SFML
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# Tell CMake to create the executable
add_executable(${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

target_sources(${PROJECT_NAME} PRIVATE 
	src/main.cpp
	src/Communication.cpp
	src/Window.cpp
	src/Gui.cpp
	)
	
target_sources(${PROJECT_NAME} PRIVATE 
	include/Communication.hpp
	include/Helpers.hpp
	include/Window.hpp
	include/Gui.hpp
	)

#set(MYGUI_DIR "" CACHE PATH "Path to mygui root dir")
#find_library(MYGUI_RELEASE NAMES "mygui" PATHS "${MYGUI_DIR}/Build/*" NO_DEFAULT_PATH)
#find_library(MYGUI_DEBUG NAMES "mygui-d" PATHS "${MYGUI_DIR}/Build/*" NO_DEFAULT_PATH)
#set(MYGUI_LIBRARIES debug ${MYGUI_DEBUG} optimized ${MYGUI_RELEASE})
#find_path(MYGUI_INCLUDE_DIR NAME "mygui/Config.hpp" PATHS "${MYGUI_DIR}/*" NO_DEFAULT_PATH)

target_include_directories(${PROJECT_NAME} PRIVATE include) # ${MYGUI_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE sfml-graphics sfml-window sfml-system) # ${MYGUI_LIBRARIES})