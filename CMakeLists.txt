# Download CPM.cmake if not already present
cmake_minimum_required(VERSION 3.14)

# Create project
project(mu-app)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set OS architecture
set(CMAKE_OSX_ARCHITECTURES "arm64")

# Find the DataProducts package
find_package(DataProducts REQUIRED)

# Create installation directories
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install")
endif()

# Set the path for where to look for the libraries of the installed target
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# include(FetchContent)
# FetchContent_Declare(
#   CPM
#   GIT_REPOSITORY https://github.com/cpm-cmake/CPM.cmake.git
#   GIT_TAG v0.42.0
# )
# FetchContent_MakeAvailable(CPM)

# # Add nlohmann_json
# CPMAddPackage(
#   NAME mu-data-products
#   SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mu-data-products"
# )

# Add the executable
add_executable(mu_app src/main.cpp)

# Include directories and link libraries
target_link_libraries(mu_app PRIVATE DataProducts::data_products)

# Install the executable
install(TARGETS mu_app
        DESTINATION bin
)