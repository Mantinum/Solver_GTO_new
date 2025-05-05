cmake_minimum_required(VERSION 3.16)

project(GTOSolver VERSION 0.1.0 LANGUAGES C CXX)

# Fetch Catch2 using FetchContent (requires CMake 3.14+)
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4 # Use a specific stable tag
)
# This makes the targets Catch2::Catch2 and Catch2::Catch2WithMain available
FetchContent_MakeAvailable(Catch2)

# Fetch spdlog using FetchContent
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1 # Utiliser un tag stable spécifique
)
# Ceci rendra la cible spdlog::spdlog disponible
FetchContent_MakeAvailable(spdlog)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Option for CUDA support (disabled by default for now)
option(GTS_CUDA "Enable CUDA support" OFF)

if(GTS_CUDA)
  message(STATUS "CUDA support enabled")
  # Add CUDA specific configurations later
else()
  message(STATUS "CUDA support disabled")
endif()

# Add source directory
add_subdirectory(external/2p2)
add_subdirectory(src)

# Enable testing with CTest
enable_testing()

# Add tests directory
add_subdirectory(tests)

message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "CMake Version: ${CMAKE_VERSION}")
message(STATUS "CXX Standard: ${CMAKE_CXX_STANDARD}")

# Supprimer les blocs add_library et add_executable ajoutés par erreur ici
# La définition de la cible gto_core se trouve dans src/CMakeLists.txt

# \... Fin du fichier ... 