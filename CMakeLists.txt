cmake_minimum_required(VERSION 3.27)
project(thinsys LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS Off)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

enable_testing()

option(CMAKE_COMPILE_WARNING_AS_ERROR OFF "Treat warnings as errors")
option(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE On "Enable LTO in release builds")

include(cmake/canonical_project.cmake)

add_subdirectory(thinsys)

add_custom_target(project_conf_files SOURCES
  .gitignore
  .clang-format
  .clangd
  conanfile.txt
)
