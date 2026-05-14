#  @file CMakeLists.txt
#  @author Gaspard Kirira
#
#  Copyright 2026, Gaspard Kirira.
#  All rights reserved.
#  https://github.com/vixcpp/vixc
#
#  Use of this source code is governed by a MIT license
#  that can be found in the License file.
#
# ====================================================================
# Vix++ - Thin language layer for Vix-powered C++ applications
# ====================================================================

cmake_minimum_required(VERSION 3.20)

project(vixc VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CheckCXXCompilerFlag)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VIXC_ENABLE_LTO "Enable LTO in Release builds" OFF)
option(VIXC_BUILD_TESTS "Build Vix++ tests" ON)

# ----------------------------------------------------
# Sources
# ----------------------------------------------------
set(VIXC_CORE_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/Diagnostic.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/ImportResolver.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/SourceFile.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/Transpiler.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/Runner.cpp"
)

add_library(vixc_core STATIC ${VIXC_CORE_SOURCES})
add_library(vixc::core ALIAS vixc_core)

target_include_directories(vixc_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_executable(vixc_cli
  "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
)

add_executable(vixc::cli ALIAS vixc_cli)

target_link_libraries(vixc_cli PRIVATE
  vixc::core
)

# ----------------------------------------------------
# Compiler warnings
# ----------------------------------------------------
function(vixc_add_flag_if_supported target flag)
  string(REGEX REPLACE "[^A-Za-z0-9]" "_" flag_var "${flag}")
  set(test_var "HAVE_VIXC_${target}_${flag_var}")

  check_cxx_compiler_flag("${flag}" ${test_var})

  if (${test_var})
    target_compile_options(${target} PRIVATE "${flag}")
  endif()
endfunction()

foreach(_vixc_target IN ITEMS vixc_core vixc_cli)
  if (MSVC)
    vixc_add_flag_if_supported(${_vixc_target} /W4)
    vixc_add_flag_if_supported(${_vixc_target} /permissive-)
  else()
    vixc_add_flag_if_supported(${_vixc_target} -Wall)
    vixc_add_flag_if_supported(${_vixc_target} -Wextra)
    vixc_add_flag_if_supported(${_vixc_target} -Wshadow)
    vixc_add_flag_if_supported(${_vixc_target} -Wconversion)
    vixc_add_flag_if_supported(${_vixc_target} -Wformat=2)
    vixc_add_flag_if_supported(${_vixc_target} -Wpedantic)
  endif()
endforeach()

# ----------------------------------------------------
# Version string
# ----------------------------------------------------
set(_VIXC_VERSION "dev")

if (DEFINED VIXC_VERSION AND NOT "${VIXC_VERSION}" STREQUAL "")
  set(_VIXC_VERSION "${VIXC_VERSION}")

elseif (DEFINED ENV{VIXC_VERSION} AND NOT "$ENV{VIXC_VERSION}" STREQUAL "")
  set(_VIXC_VERSION "$ENV{VIXC_VERSION}")

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
      set(_VIXC_VERSION "${_GIT_DESCRIBE}")
    endif()
  endif()

  if ("${_VIXC_VERSION}" STREQUAL "dev" AND DEFINED PROJECT_VERSION)
    set(_VIXC_VERSION "v${PROJECT_VERSION}")
  endif()
endif()

message(STATUS "Vix++ version: ${_VIXC_VERSION}")

target_compile_definitions(vixc_core PUBLIC
  VIXC_VERSION="${_VIXC_VERSION}"
)

# ----------------------------------------------------
# LTO
# ----------------------------------------------------
if (VIXC_ENABLE_LTO AND CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)

  check_ipo_supported(RESULT _ipo_ok OUTPUT _ipo_msg)

  if (_ipo_ok)
    set_property(TARGET vixc_core PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET vixc_cli PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(WARNING "[vixc] IPO/LTO not supported: ${_ipo_msg}")
  endif()
endif()

# ----------------------------------------------------
# Output name
# ----------------------------------------------------
set_target_properties(vixc_cli PROPERTIES
  OUTPUT_NAME "vixc"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
)

# ----------------------------------------------------
# Tests
# ----------------------------------------------------
if (VIXC_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
  enable_testing()
  add_subdirectory(tests)
endif()

# ----------------------------------------------------
# Install
# ----------------------------------------------------
install(TARGETS vixc_core vixc_cli
  EXPORT vixcTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT vixcTargets
  FILE vixcTargets.cmake
  NAMESPACE vixc::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vixc
)

# ----------------------------------------------------
# CMake package config
# ----------------------------------------------------
include(CMakePackageConfigHelpers)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/vixcConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vixcConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vixc
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/vixcConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/vixcConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/vixcConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vixc
)

message(STATUS "Vix++ configured.")
message(STATUS "Executable will be: ${CMAKE_BINARY_DIR}/vixc")
