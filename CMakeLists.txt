# Download CPM.cmake if not already present
cmake_minimum_required(VERSION 3.14)

# Create project
project(mu-app)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set OS architecture
set(CMAKE_OSX_ARCHITECTURES "arm64")

# Custom dependencies
find_package(DataProducts CONFIG REQUIRED)
find_package(Unpackers CONFIG REQUIRED)

# Create installation directories
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install")
endif()

# Set the path for where to look for the libraries of the installed target
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# Add the executable
add_executable(mu_app src/main.cpp)

# Include directories and link libraries
target_link_libraries(mu_app PRIVATE 
    DataProducts::data_products
    Unpackers::unpackers
    )

# Install the executable
install(TARGETS mu_app
        DESTINATION bin
)