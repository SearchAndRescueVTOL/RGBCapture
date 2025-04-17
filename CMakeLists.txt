cmake_minimum_required(VERSION 3.10)
project(Grab_Strategies)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directory for headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add executable using your actual source files
add_executable(Grab_Strategies
    src/RGBCapture.cpp
    src/image_utils.cpp
)

# Link with Pylon SDK
find_package(Pylon REQUIRED)
target_link_libraries(Grab_Strategies PRIVATE Pylon::Pylon)

# Optional: install to /usr/local/bin
install(TARGETS Grab_Strategies DESTINATION bin)
