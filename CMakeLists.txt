cmake_minimum_required(VERSION 3.8.2)
project(Sorting_Control_PC)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release") 

# Add user source files
file(GLOB SOURCES "src/*")
# Add user header files
file(GLOB HEADERS "include/*")

set(CMAKE_PREFIX_PATH "" CACHE PATH "Where is QT cmake?" ${CMAKE_PREFIX_PATH})

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#set(QT_USE_QTOPENGL TRUE)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Charts REQUIRED)
find_package(Qt5SerialPort REQUIRED)

# Tell CMake to create the executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::Charts Qt5::SerialPort)