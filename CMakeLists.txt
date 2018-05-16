cmake_minimum_required(VERSION 3.8.2)
project(Sorting_Control_PC)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release") 

# Add user source files
file(GLOB SOURCES "src/*")
# Add user header files
file(GLOB HEADERS "include/*")

set(CMAKE_PREFIX_PATH "" CACHE PATH "Where is QT cmake?" ${CMAKE_PREFIX_PATH})

# Serial
set(SERIAL_ROOT_DIR "" CACHE PATH "Path to serial root directory")
find_library(SERIAL_LIBRARIES_RELEASE NAMES "serial" PATHS "${SERIAL_ROOT_DIR}/*" NO_DEFAULT_PATH)
find_library(SERIAL_LIBRARIES_DEBUG NAMES "serial-d" PATHS "${SERIAL_ROOT_DIR}/*" NO_DEFAULT_PATH)
set(SERIAL_LIBRARIES debug ${SERIAL_LIBRARIES_DEBUG} optimized ${SERIAL_LIBRARIES_RELEASE})
find_path(SERIAL_INCLUDE_DIR NAME "serial/serial.h" PATHS "${SERIAL_ROOT_DIR}/*" NO_DEFAULT_PATH)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#set(QT_USE_QTOPENGL TRUE)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Charts REQUIRED)
#find_package(Qt5Designer REQUIRED)
#find_package(Qt5PrintSupport REQUIRED)

link_directories(${SERIAL_LIB_DIR})

# Tell CMake to create the executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE include ${SERIAL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::Charts ${SERIAL_LIBRARIES})