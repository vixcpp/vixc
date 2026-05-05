#  @file CMakeLists.txt
#  @author Gaspard Kirira
#
#  Copyright 2026, Gaspard Kirira.
#  All rights reserved.
#  https://github.com/vixcpp/vixpp
#
#  Use of this source code is governed by a MIT license
#  that can be found in the License file.
#
# ====================================================================
# Vix++ - Thin language layer for Vix-powered C++ applications
# ====================================================================

cmake_minimum_required(VERSION 3.20)

project(vixpp VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CheckCXXCompilerFlag)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VIXPP_ENABLE_LTO "Enable LTO in Release builds" OFF)
option(VIXPP_BUILD_TESTS "Build Vix++ tests" ON)

# ----------------------------------------------------
# Sources
# ----------------------------------------------------
set(VIXPP_CORE_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/Diagnostic.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/ImportResolver.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/SourceFile.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/Transpiler.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/Runner.cpp"
)

add_library(vixpp_core STATIC ${VIXPP_CORE_SOURCES})
add_library(vixpp::core ALIAS vixpp_core)

target_include_directories(vixpp_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_executable(vixpp_cli
  "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
)

add_executable(vixpp::cli ALIAS vixpp_cli)

target_link_libraries(vixpp_cli PRIVATE
  vixpp::core
)

# ----------------------------------------------------
# Compiler warnings
# ----------------------------------------------------
function(vixpp_add_flag_if_supported target flag)
  string(REGEX REPLACE "[^A-Za-z0-9]" "_" flag_var "${flag}")
  set(test_var "HAVE_VIXPP_${target}_${flag_var}")

  check_cxx_compiler_flag("${flag}" ${test_var})

  if (${test_var})
    target_compile_options(${target} PRIVATE "${flag}")
  endif()
endfunction()

foreach(_vixpp_target IN ITEMS vixpp_core vixpp_cli)
  if (MSVC)
    vixpp_add_flag_if_supported(${_vixpp_target} /W4)
    vixpp_add_flag_if_supported(${_vixpp_target} /permissive-)
  else()
    vixpp_add_flag_if_supported(${_vixpp_target} -Wall)
    vixpp_add_flag_if_supported(${_vixpp_target} -Wextra)
    vixpp_add_flag_if_supported(${_vixpp_target} -Wshadow)
    vixpp_add_flag_if_supported(${_vixpp_target} -Wconversion)
    vixpp_add_flag_if_supported(${_vixpp_target} -Wformat=2)
    vixpp_add_flag_if_supported(${_vixpp_target} -Wpedantic)
  endif()
endforeach()

# ----------------------------------------------------
# Version string
# ----------------------------------------------------
set(_VIXPP_VERSION "dev")

if (DEFINED VIXPP_VERSION AND NOT "${VIXPP_VERSION}" STREQUAL "")
  set(_VIXPP_VERSION "${VIXPP_VERSION}")

elseif (DEFINED ENV{VIXPP_VERSION} AND NOT "$ENV{VIXPP_VERSION}" STREQUAL "")
  set(_VIXPP_VERSION "$ENV{VIXPP_VERSION}")

else()
  find_package(Git QUIET)

  if (Git_FOUND)
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" describe --tags --always
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE _GIT_DESCRIBE
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )

    if (_GIT_DESCRIBE)
      set(_VIXPP_VERSION "${_GIT_DESCRIBE}")
    endif()
  endif()

  if ("${_VIXPP_VERSION}" STREQUAL "dev" AND DEFINED PROJECT_VERSION)
    set(_VIXPP_VERSION "v${PROJECT_VERSION}")
  endif()
endif()

message(STATUS "Vix++ version: ${_VIXPP_VERSION}")

target_compile_definitions(vixpp_core PUBLIC
  VIXPP_VERSION="${_VIXPP_VERSION}"
)

# ----------------------------------------------------
# LTO
# ----------------------------------------------------
if (VIXPP_ENABLE_LTO AND CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)

  check_ipo_supported(RESULT _ipo_ok OUTPUT _ipo_msg)

  if (_ipo_ok)
    set_property(TARGET vixpp_core PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET vixpp_cli PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(WARNING "[vixpp] IPO/LTO not supported: ${_ipo_msg}")
  endif()
endif()

# ----------------------------------------------------
# Output name
# ----------------------------------------------------
set_target_properties(vixpp_cli PROPERTIES
  OUTPUT_NAME "vix++"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
)

# ----------------------------------------------------
# Tests
# ----------------------------------------------------
if (VIXPP_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
  enable_testing()
  add_subdirectory(tests)
endif()

# ----------------------------------------------------
# Install
# ----------------------------------------------------
install(TARGETS vixpp_core vixpp_cli
  EXPORT vixppTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT vixppTargets
  FILE vixppTargets.cmake
  NAMESPACE vixpp::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vixpp
)

# ----------------------------------------------------
# CMake package config
# ----------------------------------------------------
include(CMakePackageConfigHelpers)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/vixppConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vixppConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vixpp
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/vixppConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/vixppConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/vixppConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vixpp
)

message(STATUS "Vix++ configured.")
message(STATUS "Executable will be: ${CMAKE_BINARY_DIR}/vix++")
