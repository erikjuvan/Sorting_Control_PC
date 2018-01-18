cmake_minimum_required(VERSION 3.0)
project(Qt_test)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release") 

# Add user source files
file(GLOB SOURCES "src/*.cpp")
# Add user header files
file(GLOB HEADERS "include/*.h")

set(CMAKE_PREFIX_PATH "" CACHE PATH "Where is QT cmake?" ${CMAKE_PREFIX_PATH})

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(QT_USE_QTOPENGL TRUE)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Charts REQUIRED)
find_package(Qt5Designer REQUIRED)
find_package(Qt5PrintSupport REQUIRED)

# Tell CMake to create the helloworld executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

# Use the Widgets module from Qt 5.
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets)